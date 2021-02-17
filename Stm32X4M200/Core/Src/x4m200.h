/*
 XeThruRadar.cpp - Library for using the XeThru Radar module
 Created by Oyvind N. Dahl, August 13, 2015.
 Updated by Oyvind N. Dahl, May 22, 2017.
 */

#ifndef x4m200_h
#define x4m200_h

typedef struct resp_data {
	bool valid;
	float movement;
	int rpm;
	char code;
} RespirationData;

// State codes for respiration
const unsigned char _xts_val_resp_state_breathing = 0; // Valid RPM detected Current RPM value
const unsigned char _xts_val_resp_state_movement = 1; // Detects motion, but can not identify breath 0
const unsigned char _xts_val_resp_state_movement_tracking = 2;// Detects motion, possible breathing 0
const unsigned char _xts_val_resp_state_no_movement = 3; // No movement detected 0
const unsigned char _xts_val_resp_state_initializing = 4; // No movement detected 0
const unsigned char _xts_val_resp_state_unknown = 6;

// Radar constants
const unsigned char _xt_start = 0x7D;
const unsigned char _xt_stop = 0x7E;
const unsigned char _xt_escape = 0x7F;

const unsigned char _xts_spc_appcommand = 0x10;
const unsigned char _xts_spc_mod_setmode = 0x20;
const unsigned char _xts_spc_mod_loadapp = 0x21;
const unsigned char _xts_spc_mod_reset = 0x22;
const unsigned char _xts_spc_mod_setledcontrol = 0x24;
const unsigned char _xts_spc_mod_noisemap = 0x25;

const unsigned char _xts_spr_appdata = 0x50;
const unsigned char _xts_spr_system = 0x30;
const unsigned char _xts_spr_ack = 0x10;

// PING command
const unsigned char _xts_spc_ping = 0x01;                   // Ping command code
const unsigned long _xts_def_pingval = 0xeeaaeaae;           // Ping seed value
const unsigned char _xts_spr_pong = 0x01;                  // Pong responce code
const unsigned long _xts_def_pongval_ready = 0xaaeeaeea;     // Module is ready
const unsigned long _xts_def_pongval_notready = 0xaeeaeeaa; // Module is not ready

const unsigned char _xts_spc_dir_command = 0x90;
const unsigned char _xts_sdc_app_setint = 0x71;
const unsigned char _xts_sdc_comm_setbaudrate = 0x80;

const unsigned long _xts_sacr_outputbaseband = 0x00000010;
const unsigned long _xts_sacr_id_baseband_output_off = 0x00000000;
const unsigned long _xts_sacr_id_baseband_output_amplitude_phase = 0x00000002;

const unsigned char _xts_spca_set = 0x10;

const unsigned long _xts_id_app_resp = 0x1423a2d6;
const unsigned long _xts_id_app_resp_adult = 0x064e57ad;
const unsigned long _xts_id_app_sleep = 0x00f17b17;
const unsigned long _xts_id_resp_status = 0x2375fe26;
const unsigned long _xts_id_sleep_status = 0x2375a16c;
const unsigned long _xts_id_detection_zone = 0x96a10a1c;
const unsigned long _xts_id_sensitivity = 0x10a5112b;

const unsigned char _xts_sm_run = 0x01;
const unsigned char _xts_sm_normal = 0x10;
const unsigned char _xts_sm_idle = 0x11;
const unsigned char _xts_sm_stop = 0x13;
const unsigned char _xts_sm_manua = 0x12;

const unsigned char _xts_sprs_booting = 0x10;
const unsigned char _xts_sprs_ready = 0x11;

const unsigned char _xt_ui_led_simple = 0x01;
const unsigned char _xt_ui_led_full = 0x02;
const unsigned char _xt_ui_led_off = 0x00;

const unsigned char _xts_spcn_setcontrol = 0x10;
const unsigned char _xts_spco_setcontrol = 0x10;
const unsigned char _xts_spc_output = 0x41;

const unsigned char _xtid_output_control_disable = 0x00;
const unsigned char _xtid_output_control_enable = 0x01;

#endif
