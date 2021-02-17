#include "XeThruRadar.h"
#include "SoftwareSerial.h"

#ifndef SERIAL_BUFFER_SIZE
#define SERIAL_BUFFER_SIZE SERIAL_RX_BUFFER_SIZE
#endif


SoftwareSerial serialRadar(2,3);

XeThruRadar::XeThruRadar() {}

void XeThruRadar::idle_module() 
{
  debug_print("stop module");
  _send_buf[0] = _xts_spc_mod_setmode;
  _send_buf[1] = _xts_sm_stop;
  send_command(_send_buf, 2);
  radar_ack();
}

void XeThruRadar::output_message(unsigned char message, unsigned char enable){
  _send_buf[0] = _xts_spc_output;
  _send_buf[1] = _xts_spco_setcontrol;
  _send_buf[2] = message & 0xff;
  memcpy(_send_buf + 3, &message, 4);
  _send_buf[7] = enable;

  send_command(_send_buf, 8);
}

void XeThruRadar::init()
{
  debug_print("Init sequence starting...");

  serialRadar.begin(115200);
  while (!serialRadar){
    ;
  }
  empty_serial_buffer();

  debug_print("Init sequence complete!");
}

void XeThruRadar::enableDebug()
{
  enable_debug_port = true;
  Serial.begin(9600);
}

/**
* Reads one character from the serial RX buffer.
* Blocks until a character is available.
*/
unsigned char XeThruRadar::serial_read_blocking()
{
  while (serialRadar.available() < 1)
  {
    delay(1);
  }
  return serialRadar.read();
}

/**
* Checks if the RX buffer is overflowing.
* The Arduino RX buffer is only 64 bytes,
* so this happens a lot with fast data rates
*/
bool XeThruRadar::check_overflow()
{
  if (serialRadar.available() >= SERIAL_BUFFER_SIZE - 1)
  {
    debug_print("Buffer overflowed");
    return true;
  }

  return false;
}

// Empties the Serial RX buffer
void XeThruRadar::empty_serial_buffer()
{
  debug_print("Emptying serial buffer... ");

  while (serialRadar.available() > 0)
    serialRadar.read(); // Remove one byte from the buffer

  debug_print("Done!");
}

void XeThruRadar::debug_print(String msg)
{
  if (enable_debug_port == true)
    Serial.println(msg);
}

/*****************
* Sends a command
******************/
void XeThruRadar::send_command(const unsigned char *cmd, int len)
{
  // Calculate CRC
  debug_print("Send command");
  char crc = _xt_start;
  for (int i = 0; i < len; i++)
    crc ^= cmd[i];

  // Send _xt_start + command + crc_string + _xt_stop
  serialRadar.write(_xt_start);
  serialRadar.write(cmd, len);
  serialRadar.write(crc);
  serialRadar.write(_xt_stop);
  serialRadar.flush();
  
  if (enable_debug_port)
  {
    Serial.print("-Data send: ");
    Serial.print(_xt_start, HEX);
    for (int i = 0; i < len; i++)
    {
      Serial.print(" ");
      Serial.print(cmd[i], HEX);  
    }
    Serial.print(" ");
    Serial.print(crc, HEX);
    Serial.print(" ");
    Serial.println(_xt_stop, HEX);
  }
}

/**
* Receives one data package from Serial
* buffer and stores it in recv_buf
*
* Returns the length of the data received.
*/
int XeThruRadar::receive_data(bool print_data)
{
  int recv_len = 0; //Number of bytes received
  unsigned char cur_char;
  
  //Wait for start character
  while (1)
  {
    // Check if input buffer is overflowed
    if (check_overflow()){
      empty_serial_buffer();
    }
    // Get one byte from radar
    cur_char = serial_read_blocking();
    if (cur_char == _xt_escape)
    {
      // Check if input buffer is overflowed
      if (check_overflow())
      {
        debug_print("Overflow while receiving. Aborting receive_data()");
        return -1;
      }

      // If it's an escape character –
      // ...ignore next character in buffer
      serial_read_blocking();
    }
    else if (cur_char == _xt_start)
    {
      // If it's the start character –
      // ...we fill the first character of the buffer and move on
      _recv_buf[0] = _xt_start;
      recv_len = 1;
      break;
    }
  }
  // Start receiving the rest of the bytes
  while (1)
  {
    // Check if input buffer is overflowed
    if (check_overflow())
    {
      debug_print("Overflow while receiving. Aborting receive_data()");
      return -1;
    }

    // Get one byte from radar
    cur_char = serial_read_blocking();

    if (cur_char == _xt_escape)
    {
      // Check if input buffer is overflowed
      if (check_overflow())
      {
        debug_print("Overflow while receiving. Aborting receive_data()");
        return -1;
      }

      // If it's an escape character –
      // fetch the next byte from serial
      cur_char = serial_read_blocking();

      // Make sure to not overwrite receive buffer
      if (recv_len >= RX_BUF_LENGTH)
      {
        debug_print("Received more than rx buffer size. Aborting receive_data()");
        return -1;
      }

      // Fill response buffer, and increase counter
      _recv_buf[recv_len] = cur_char;
      recv_len++;
    }

    else if (cur_char == _xt_start)
    {
      // If it's the start character, something is wrong
      debug_print("Start character received in the middle of message. Restart receive_data()");
      _recv_buf[0] = _xt_start;
      recv_len = 1;
      continue;
    }

    else
    {
      // Make sure not overwrite receive buffer
      if (recv_len >= RX_BUF_LENGTH)
        break;

      // Fill response buffer, and increase counter
      _recv_buf[recv_len] = cur_char;
      recv_len++;

      // is it the stop byte?
      if (cur_char == _xt_stop)
      {
        break; //Exit this loop
      }
    }
  }

  // Calculate CRC
  char crc = 0;

  // CRC is calculated without the crc itself and the stop byte, hence the -2 in the counter
  for (int i = 0; i < recv_len - 2; i++)
  {
    crc ^= _recv_buf[i];
  }

  // Print the received data
  if (print_data)
  {
    Serial.print("-Data receive: ");
    for (int i = 0; i < recv_len; i++)
    {
      Serial.print(_recv_buf[i], HEX);
      Serial.print(" ");
    }
    Serial.println(" ");
  }

  // Check if calculated CRC matches the recieved
  if (crc == _recv_buf[recv_len - 2])
  {
    return recv_len; // Return length of data packet upon success
  }
  else
  {
    debug_print("[Error]: CRC check failed!");
    return -1; // Return -1 upon crc failure
  }
}


/**
* Waiting for radar to return the ack
*/
bool XeThruRadar::radar_ack()
{
  while(true){
    receive_data(enable_debug_port);
    if (_recv_buf[1] == _xts_spr_ack || _recv_buf[2] == _xts_spr_ack)
    {
      debug_print("Radar ack");
      return true;
    }
    

    delay(500);
  }
}

/**
* Waiting for radar to become ready in the bootup sequence
*/
bool XeThruRadar::radar_ready()
{
  while(true){
    receive_data(enable_debug_port);
      if ( _recv_buf[1] == _xts_sprs_ready || _recv_buf[2] == _xts_sprs_ready)
      {
        debug_print("Radar ready");
        return true;
      }
    delay(500);
  }
}

void XeThruRadar::reset_module()
{
  debug_print("Resetting module...");
  send_command(&_xts_spc_mod_reset, 1);


  debug_print("Waiting for system status messages...");
  radar_ready();

  debug_print("Module reset");
  
}

void XeThruRadar::load_respiration_app()
{
  debug_print("Load respiration app...");
  //Fill send buffer
  _send_buf[0] = _xts_spc_mod_loadapp;
  memcpy(_send_buf + 1, &_xts_id_app_resp_adult, 4);

  //Send the command
  send_command(_send_buf, 5);
  radar_ack();

  debug_print("Respiration app load");

}

void XeThruRadar::configure_noisemap() {

  _send_buf[0] = _xts_spc_mod_noisemap;
  _send_buf[1] = _xts_spcn_setcontrol;
  _send_buf[2] = 0x06; // 0x06: Use default noisemap and adaptive noisemap
  _send_buf[3] = 0x00;
  _send_buf[4] = 0x00;
  _send_buf[5] = 0x00;

  send_command(_send_buf, 6);
  radar_ack();
}

void XeThruRadar::execute_app()
{
  debug_print("Execute app...");
  //Fill send buffer
  _send_buf[0] = _xts_spc_mod_setmode;
  _send_buf[1] = _xts_sm_run;

  // Send the command
  send_command(_send_buf, 2);
  // Get response
  receive_data(enable_debug_port);
  debug_print("Complete");
}

//TODO da controllare
void XeThruRadar::led_control_simple(){
  debug_print("Set led cotrol simple");

  _send_buf[0] = _xts_spc_mod_setledcontrol;
  _send_buf[1] = _xt_ui_led_simple;

  send_command(_send_buf, 2);
}

void XeThruRadar::setDetectionZone(float start_zone, float end_zone)
{
  debug_print("Set detection Zone");

  _send_buf[0] = _xts_spc_appcommand;
  _send_buf[1] = _xts_spca_set;

  memcpy(_send_buf + 2, &_xts_id_detection_zone, 4);
  memcpy(_send_buf + 6, &start_zone, 4);
  memcpy(_send_buf + 10, &end_zone, 4);

  //Send the command
  send_command(_send_buf, 14);
  radar_ack();
}

void XeThruRadar::setSensitivity(long sensitivity)
{
  debug_print("Set sensity");

  _send_buf[0] = _xts_spc_appcommand;
  _send_buf[1] = _xts_spca_set;

  memcpy(_send_buf + 2, &_xts_id_sensitivity, 4);
  memcpy(_send_buf + 6, &sensitivity, 4);

  //Send the command
  send_command(_send_buf, 10);
  radar_ack();

}

/**********************************************************
  This function retrieves a packet of respiration data, 
  extracts the content and returns it in a readable format
***********************************************************/
RespirationData XeThruRadar::get_respiration_data()
{
  RespirationData data; // For storing and returning respiration data
  String str;           //For debugging

  // receive_data() fills _recv_buf[] with valid data
  if (receive_data(enable_debug_port) < 0)
  {
    //Something went wrong!
    data.valid_data = false;
    return data;
  }

  // Check that it's app-data we've received
  if (_recv_buf[1] != _xts_spr_appdata)
  {
    //Something went wrong!
    data.valid_data = false;
    return data;
  }

  // Get state code
  //data.state_code = _recv_buf[10];
  memcpy(&data.state_code, &_recv_buf[10], 4);
  str = String((int)data.state_code);
  Serial.println("get_respiration_data: state_code= " + str);


  if(data.state_code == _xts_val_resp_state_breathing){
  // Get rpm value
  memcpy(&data.rpm, &_recv_buf[14], 4);
  //data.rpm = *(int*)&_recv_buf[14];
  str = String((data.rpm));
  Serial.println("get_respiration_data: rpm= " + str);
  //debug_print("get_respiration_data: rpm= " + str);

  // Get movement value
  memcpy(&data.movement, &_recv_buf[22], 4);
  //data.movement = *(float*)&_recv_buf[22];
  str = String(data.movement);
  Serial.println("get_respiration_data: movement= " + str);
  //debug_print("get_respiration_data: movement= " + str);
  }

  // Set state to indicate valid data
  data.valid_data = true;

  // Return the extracted data
  return data;
}
