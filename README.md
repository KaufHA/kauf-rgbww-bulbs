# KAUF RGBWW Smart Bulbs (BLF*/BRF30)

The recommended way to import a bulb into your ESPHome dashboard is through the dashboard import feature.  Bulbs should be detected by the ESPHome dashboard automatically, and be importable by clicking "Adopt".  You can accomplish the same thing without the dashboard detecting the bulb by using the following yaml.  You will probably need to add the [`use_address`](https://esphome.io/components/wifi.html?highlight=use_address#configuration-variables) option under `wifi:` to set the bulb's IP address and initially flash the bulb.  `use_address` can typically be removed after initial flashing.

The friendly_name substitution is recommended and will not be automatically created by the ESPHome dashboard import.

```
substitutions:
  name: bedroom-lamp
  friendly_name: Bedroom Lamp

packages:
  kauf.rgbww: github://KaufHA/kauf-rgbww-bulbs/kauf-bulb.yaml

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
```

## Repo Contents
This repo contains files for the KAUF RGBWW Smart Bulbs.


***components* directory** - Custom components needed to compile the KAUF RGBWW bulb firmware.  These don't need to be downloaded.  The yaml files automatically grab them by reference to this GitHub repo.  Every subfolder within the components directory that does not start with kauf_* is copied from stock ESPHome and edited for our products.


### ESPHome YAML Config Files

***kauf-bulb.yaml*** - The yaml file recommended to import a bulb into your ESPHome dashboard. This is the yaml file incorporated automatically when the dashboard import feature is used.

***kauf-bulb-plus.yaml*** - A yaml file that adds in additional config entities from the precompiled binaries.

***kauf-bulb-lite.yaml*** - A yaml file without any Kauf custom components but otherwise keeping as much functionality from kauf-bulb.yaml as possible.

***kauf-bulb-minimal.yaml*** - A yaml file to import a bulb into your ESPHome dashboard with only basic RGBWW Bulb functionality.  Copy and paste the contents of this to your local yaml file as a template if you are looking to do your own thing.

***kauf-bulb-update.yaml*** - The yaml file to build the update bin file. Generally not useful to end users.

***kauf-bulb-factory.yaml*** - The yaml file to build the factory bin file. Generally not useful to end users.


### Tasmota Files

***tasmota-kauf-bulb-v13.0.0.1.bin.gz*** - Our custom Tasmota firmware file that has been gzipped. This is the file recommended for flashing our bulbs to Tasmota.  This firmware is stock tasmota.bin with a bunch of unnecessary code removed, DDP enabled, the proper template for our bulbs enabled by default, and default Wi-Fi credentials of initial_ap / asdfasdfasdfasdf.

***tasmota-kauf-bulb-v13.0.0.1.bin*** - Tasmota bin file corresponding to the bin.gz file.

***user_config_override.h*** - config file used to build the Tasmota binaries in this repo.


### Help / Text Files

***README.md*** - This file.

***RGBWW Bulb Manual.pdf*** - A PDF version of the manual that comes with our bulbs.

***enabling-effects.md*** - An explanation on various ways to compile ESPHome and add effects to the bulbs.


## Entities

The following entities will be created by the bulbs, and available from the web interface, the HTTP API, and within Home Assistant.

### Control Entities

***Kauf Bulb*** light entity - The main light entity that controls the bulb.  It will be renamed per the `friendly_name` substitution.  All other entities will have this entity's name prepended to the below indicated name.

### Configuration Entities

***Cold RGB*** light entity - Disabled by default.  Allows the definition of an RGB value that will be mixed in with the cold white LEDs when the bulb is in color temperature mode.  This entity is not meant to directly control the bulb.

***Warm RGB*** light entity - Disabled by default.  Allows the definition of an RGB value that will be mixed in with the warm white LEDs when the bulb is in color temperature mode.  This entity is not meant to directly control the bulb.

***Effect*** select entity - Used to enable and disable DDP.

### Configuration Entities - Precompiled Binaries Only

The precompiled update files add the following configuration entities that were removed in the yaml package to save space.  Those using the yaml packages can configure all of these aspects using [substitutions](#replacements-for-configuration-entities-that-only-exist-in-precompiled-binaries).

***Power On State*** select entity - Sets the behavior of the bulb on boot / restoration of power.  Defaults to `Always On - Last Value`.  Note that the yaml packages default to `RESTORE_DEFAULT_ON`, which is different from the precompiled binaries.
- `Restore Power Off State`: Same as ESPHome's `RESTORE_DEFAULT_ON`.  The bulb will attempt to restore the most recent value of the main light entity.  If a previous value cannot be restored, the bulb will turn on to bright white.
- `Always On - Last Value`: Same as ESPHome's `RESTORE_AND_ON`.  The bulb will always turn on at boot.  A previous value will be restored if possible.  If a previous value can't be restored, the bulb will still turn on but to the default bright white.
- `Always On - Bright White`: Same as ESPHome's `ALWAYS_ON`.  The bulb will always turn on and always to a bright white due to not even attempting to restore any previous state.
- `Always Off`: Same as ESPHome's `RESTORE_AND_OFF`.  The bulb will always be off when power is restored.  However, the bulb will remember its last state for when the bulb is eventually turned on.
- `Invert State`: Same as ESPHome's `RESTORE_INVERTED_DEFAULT_ON`.  The bulb will restore to on if it was last off, and restore to off if it was last on.  The most recent on-value will be remembered and restored when the bulb turns on.

***Max Power*** number entity - Sets the maximum PWM percentage for all LED channels.  Defaults to 80%.

***Default Fade*** number entity - Sets the default transition length in milliseconds for the main light entity.  Defaults to 250.

***No HASS*** switch entity - When turned on, the reboot timeout is disabled so that the bulb can be used without being connected to Home Assistant.  On by default.


### Diagnostic Entities

***Uptime*** sensor entity - Disabled by default.  Gives the bulb's uptime.

***Restart Firmware*** button entity - Disabled by default.  When pressed, reboots the bulb's firmware.

***DDP Debug*** select entity - Disabled by default.  Use to output additional debug statements for DDP functionality.

***IP Address*** sensor entity - Disabled by default.  Provides the bulb's IP address.


## DDP Functionality for WLED, xLights, etc.
DDP functionality needs to be enabled by changing the "Effect" select entity to "WLED / DDP".  Once enabled, the bulb will listen for DDP packets and change color as indicated by any received DDP packet.  The DDP packet should have 3 channels: Red, Green Blue.

**Interaction with Home Assistant:** With DDP enabled, the bulbs will always be listening for commands both from Home Assistant and DDP.  Any command from either will change the color of the bulb until another command from either is received.  If you are seeing glitches in your DDP effects, you might have a Home Assistant automation messing with the light while you are trying to control with DDP.  You might also have two different DDP sources controlling the same bulb.

**DDP Brightness:** If the corresponding light entity in Home Assistant is on, then received DDP packets will be scaled to the brightness of the Home Assistant light entity.  If the corresponding light entity is off in Home Assistant, then the DDP packet will be displayed as-is without brightness scaling.

**Chaining:** If a DDP packet has enough channel data for more than one bulb, the bulb will take the first three channels (R,G,B) for itself and send the remaining data to the next higher IP address.  Each bulb will split up excess DDP packets into two new DDP packets, allowing the DDP chain to tree out much faster than linear propagation.

**Tasmota:** For Tasmota, the command `scheme 5` enables DDP and `scheme 0` disables DDP.

## Advanced Settings
When using kauf-bulb.yaml as a package in the ESPHome dashboard, you can configure the following aspects by adding substitutions to your local yaml config. The substitutions section of kauf-bulb.yaml has comments with more explanation as well.

***name*** - Used to name the device for mDNS, the ESPHome dashboard, and in Home Assistant.

***friendly_name*** - The friendly name will be used to name every entity in Home Assistant. Add a substitution to change this to something descriptive for each device.

***disable_entities*** - Adding a substitution to redefine this to "false" will result in all entities being automatically enabled in Home Assistant.

***disable_webserver*** - Defining this to "true" will result in the web server not listening on port 80.  The web server code will still be compiled in, but the web server will not be accessible.

***wifi_ap_timeout*** - Defines the amount of time after a Wi-Fi connection is lost that the bulb will put up its Wi-Fi AP to allow reconfiguration of credentials.  This defaults to 15 seconds on the precompiled binaries and 2 minutes on the yaml package.

***sub_on_turn_on*** - define an ESPHome script to execute when the light turns on.

***sub_on_turn_off*** - define an ESPHome script to execute when the light turns off.

***sub_warm_white_temp*** - Define a new warm white color temperature.  Defaults to 2800 Kelvin.  Can be in mireds or Kelvin, but the units must be given in either case.

***sub_cold_white_temp*** - Define a new cold white color temperature.  Defaults to 6600 Kelvin.  Can be in mireds or Kelvin, but the units must be given in either case.

- **Note:** The sub_warm_white_temp and sub_cold_white_temp substitutions do not have any impact on what the bulb looks like at min/max temp settings, only how the min/max settings are expressed in Home Assistant.

***sub_reboot_req*** - The number of reboots in a row, each occurring after less than 10 seconds or without the bulb connecting to Wi-Fi, required to reset the stored Wi-Fi credentials and force-enable the Wi-Fi AP.  Defaults to 9.  If this is increased, you need to also increase the `sub_ota_num_attempts` substitution.

***sub_ota_num_attempts*** - modifies the number of reboots required for the bulb to go into safe mode.  Defaults to 15.  Needs to be at least 1.5-2.0 times the `sub_reboot_req` value to ensure that the bulb can reset its wifi credentials before going into safe mode.



### Replacements for Configuration Entities That Only Exist in Precompiled Binaries

The following substitutions are used to configure aspects that have configuration entities in the precompiled binaries but removed in the yaml package to save space.  Use the following substitutions instead of configuration entities.

***sub_max_power*** - Max PWM for the LED channels.  Needs to be a float between 0.0 and 1.0.  Defaults to 0.8 (80%).

***sub_default_transition_length*** - Default transition length for the bulb.  Defaults to 250ms.

***sub_api_reboot_timeout*** - API reboot timeout.  Defaults to 0s so the bulbs will never automatically reboot due to the API not being connected.  ESPHome default is 15min, so we might recommend that value if you desire the reboot_timeout feature to be enabled.

***light_restore_mode*** - Defines the state of the bulb on boot-up.  For more information on the available options, see restore_mode under [Base Light Configuration](https://esphome.io/components/light/index.html?highlight=restore_mode#base-light-configuration).
- `RESTORE_DEFAULT_ON` is the default and equivalent to the select entity option `Restore Power Off State`.
- `RESTORE_AND_ON` is equivalent to the select entity option `Always On - Last Value`.
- `ALWAYS_ON` is equivalent to the select entity option `Always On - Bright White`.
- `RESTORE_AND_OFF` is equivalent to the select entity option `Always Off`.
- `RESTORE_INVERTED_DEFAULT_ON` is equivalent to the select entity option `Invert State`.

## Factory Reset
Going to the bulb's URL in a web browser and adding /reset will completely wipe all settings from flash memory.

## Clearing Wi-Fi Credentials to Get Wi-Fi AP Back
You can clear the bulb's Wi-Fi credentials and get the Wi-Fi AP back to configure new credentials by rebooting the bulb 9 times in a row, with each boot lasting less than 10 seconds.  Starting with the 3rd such boot, the bulb will flash yellow to let you know its working.  Eventually the bulb will turn red, reset the credentials, and reboot to put up its Wi-Fi AP.  The number of reboots required can be changed with the `sub_reboot_req` substitution.

## Adding Effects
Effects can be added to the main light entity while using yaml packages with the following yaml.  The effects listed below are just for example and can be added to, edited, or removed.

```
light:
  - id: !extend kauf_light
    effects:
      - flicker:
          alpha: 94%
          intensity: 12%
      - strobe:
      - pulse:
      - random:
```

See [enabling-effects.md](https://github.com/KaufHA/kauf-rgbww-bulbs/blob/main/enabling-effects.md) for more details.

## Troubleshooting
Some bulbs may begin color cycling when initially powered on.  This is due to a failure to clear the factory test routine when the bulbs were manufactured.  The color cycling will automatically end after 10 minutes.  The color cycling can also be terminated by flashing the bulb via an update bin file or the ESPHome dashboard.  Firmware version 1.86 adds a button in the web interface to stop the factory test routine.

General troubleshooting ideas applicable to all products are located in the [Common repo's readme](https://github.com/KaufHA/common/blob/main/README.md#troubleshooting).

## Recommended Tasmota Template

We recommend that use the Tasmota bin.gz file in this repository to flash the bulbs to Tasmota.  Our binary files have the template built in.  If you are flashing stock Tasmota you can use the following template.

```
{"NAME":"Kauf Bulb", "GPIO":[0,0,0,0,416,419,0,0,417,420,418,0,0,0], "FLAG":0, "BASE":18, "CMND":"SO105 1|RGBWWTable 204,204,122,153,153"}
```

## Links
- [Purchase A15 bulbs on Amazon](https://www.amazon.com/dp/B0B3SCKQNL)
- [Purchase A19 bulbs on Amazon](https://www.amazon.com/dp/B0B3SMC6TM)
- [Purchase A21 bulbs on Amazon](https://www.amazon.com/dp/B09GV9FD3X)
- [Purchase BR30 bulbs on Amazon](https://www.amazon.com/dp/B09L5P2MDD)
- [KAUF YouTube Channel](https://www.youtube.com/channel/UCjgziIA-lXmcqcMIm8HDnYg)
- [KaufHA Website](https://kaufha.com/blf10/)
