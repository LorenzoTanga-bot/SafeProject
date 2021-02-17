#include <Arduino.h>

#include "XeThruRadar.h"

XeThruRadar radar;
String str;
int i = 0;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    ;
  }
  delay(5000);

  Serial.println("Setup");
  //radar.enableDebug();
  radar.init();
  radar.reset_module();
  radar.load_respiration_app();
  radar.configure_noisemap();
  //radar.led_control_simple();
  radar.setSensitivity(5);
  radar.setDetectionZone(0.4, 1.0);
  Serial.println("Execute app");
  radar.execute_app();
}

void loop()
{
  radar.get_respiration_data();
}