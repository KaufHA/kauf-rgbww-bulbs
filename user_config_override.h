/*
  user_config_override.h - user configuration overrides my_user_config.h for Tasmota

  Copyright (C) 2021  Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _USER_CONFIG_OVERRIDE_H_
#define _USER_CONFIG_OVERRIDE_H_

/*****************************************************************************************************\
 * USAGE:
 *   To modify the stock configuration without changing the my_user_config.h file:
 *   (1) copy this file to "user_config_override.h" (It will be ignored by Git)
 *   (2) define your own settings below
 *
 ******************************************************************************************************
 * ATTENTION:
 *   - Changes to SECTION1 PARAMETER defines will only override flash settings if you change define CFG_HOLDER.
 *   - Expect compiler warnings when no ifdef/undef/endif sequence is used.
 *   - You still need to update my_user_config.h for major define USE_MQTT_TLS.
 *   - All parameters can be persistent changed online using commands via MQTT, WebConsole or Serial.
\*****************************************************************************************************/

#undef  CFG_HOLDER
#define CFG_HOLDER        1112                   // [Reset 1] Change this value to load SECTION1 configuration parameters to flash

// set template
#undef  MODULE
#define MODULE                 USER_MODULE      // [Module] Select default module from tasmota_template.h
#undef  FALLBACK_MODULE
#define FALLBACK_MODULE        USER_MODULE      // [Module2] Select default module on fast reboot where USER_MODULE is user template
#undef  USER_TEMPLATE
#define USER_TEMPLATE "{\"NAME\":\"Kauf Bulb\", \"GPIO\":[0,0,0,0,416,419,0,0,417,420,418,0,0,0], \"FLAG\":0, \"BASE\":18, \"CMND\":\"SO105 1|RGBWWTable 204,204,122,153,153\"}"

// configure wifi
#undef  STA_SSID1
#define STA_SSID1         "initial_ap"        // [Ssid1] Wifi SSID
#undef  STA_PASS1
#define STA_PASS1         "asdfasdfasdfasdf"  // [Password1] Wifi password
#undef  WIFI_CONFIG_TOOL
#define WIFI_CONFIG_TOOL       WIFI_MANAGER      // [WifiConfig] Default tool if Wi-Fi fails to connect (default option: 4 - WIFI_RETRY)
                                                 // (WIFI_RESTART, WIFI_MANAGER, WIFI_RETRY, WIFI_WAIT, WIFI_SERIAL, WIFI_MANAGER_RESET_ONLY)
                                                 // The configuration can be changed after first setup using WifiConfig 0, 2, 4, 5, 6 and 7.

// configure etc
#undef PROJECT
#define PROJECT "kauf-bulb"

#undef FRIENDLY_NAME
#define FRIENDLY_NAME "Kauf Bulb"

#ifndef USE_NETWORK_LIGHT_SCHEMES
#define USE_NETWORK_LIGHT_SCHEMES
#endif

#ifndef DEBUG_LIGHT
#define DEBUG_LIGHT
#endif


// disable unnecessary stuff
#undef USE_AC_ZERO_CROSS_DIMMER
#undef USE_IMPROV
#undef USE_SONOFF_RF
  #undef USE_RF_FLASH
#undef USE_SONOFF_SC
#undef USE_TUYA_MCU
  #undef USE_TUYA_TIME
#undef USE_ARMTRONIX_DIMMERS
#undef USE_PS_16_DZ
#undef USE_SONOFF_IFAN
#undef USE_BUZZER
#undef USE_ARILUX_RF
#undef USE_SHUTTER
#undef USE_EXS_DIMMER
#undef USE_SONOFF_D1
#undef USE_SHELLY_DIMMER
  #undef SHELLY_CMDS
  #undef SHELLY_FW_UPGRADE
#undef USE_WS2812
#undef USE_MY92X1
#undef USE_SM16716
#undef USE_SM2135
#undef USE_SM2335
#undef USE_BP1658CJ
#undef USE_BP5758D
#undef USE_SONOFF_L1
#undef USE_ELECTRIQ_MOODL
#undef USE_DS18x20
#undef USE_I2C
    #undef USE_VEML6070_SHOW_RAW
    #undef USE_APDS9960_GESTURE
    #undef USE_APDS9960_PROXIMITY
    #undef USE_APDS9960_COLOR
  #undef USE_ADE7953
    #undef USE_DISPLAY_MODES1TO5
    #undef USE_DISPLAY_LCD
    #undef USE_DISPLAY_SSD1306
    #undef USE_DISPLAY_MATRIX
    #undef USE_DISPLAY_SEVENSEG
#undef USE_ENERGY_SENSOR
#undef USE_ENERGY_MARGIN_DETECTION
  #undef USE_ENERGY_POWER_LIMIT
#undef USE_ENERGY_DUMMY
#undef USE_HLW8012
#undef USE_CSE7766
#undef USE_PZEM004T
#undef USE_PZEM_AC
#undef USE_PZEM_DC
#undef USE_MCP39F501
#undef USE_BL09XX
#undef USE_DHT
#undef USE_IR_REMOTE
  #undef USE_IR_SEND_NEC
  #undef USE_IR_SEND_RC5
  #undef USE_IR_SEND_RC6
  #undef USE_IR_RECEIVE
  #undef USE_ZIGBEE_ZNP
  #undef USE_ZBBRIDGE_TLS

#endif  // _USER_CONFIG_OVERRIDE_H_
