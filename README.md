# KAUF RGBWW Smart Bulbs (BLF*/BRF30)

The recommended way to import a bulb into your ESPHome dashboard is through the dashboard import feature.  The bulb will need to be updated to version 1.72 or greater to be discovered by the ESPHome dashboard.  You can accomplish the same thing without updating first by using the following yaml modified as desired.

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

***components* directory** - Custom components needed to compile the KAUF RGBWW bulb firmware.  These don't need to be downloaded, the yaml files automatically grab them by reference to this github repo.  Every subfolder not starting with kauf_* is copied from stock ESPHome and edited for our products.

***kauf-bulb.yaml*** - The yaml file recommended to import a bulb into your ESPHome dashboard and keep all custom KAUF functionality. This is the yaml file incorporated automatically when the dashboard import feature is used.

***kauf-bulb-lite.yaml*** - A yaml file without any Kauf custom components but otherwise keeping as much functionality from kauf-bulb.yaml as possible.

***kauf-bulb-minimal.yaml*** - A yaml file to import a bulb into your ESPHome dashboard with only basic RGBWW Bulb functionality.

***kauf-bulb-update.yaml*** - The yaml file to build the update bin file. Generally not useful to end users.

***kauf-bulb-factory.yaml*** - The yaml file to build the factory bin file. Generally not useful to end users.

## DDP Functionality for WLED, xLights, etc.
DDP functionality needs to be enabled by changing the "Effect" select entity to "WLED / DDP".  Once enabled, the bulb will listen for DDP packets and change color as indicated by any received DDP packet.  The DDP packet should have 3 channels: Red, Green Blue.  

**Interaction with Home Assistant:** With DDP enabled, the bulbs will always be listening for commands both from Home Assistant and DDP.  Any command from either will change the color of the bulb until another command from either is received.  If you are seeing glitches in your DDP effects, you might have a Home Assistant automation messing with the light while you are trying to control with DDP.

**DDP Brightness:** If the corresponding light entity in Home Assistant is on, then received DDP packets will be scaled to the brightness of the Home Assistant light entity.  If the corresponding light entity is off in Home Assistant, then the DDP packet will be displayed as-is without brightness scaling.

**Chaining:** As of update v1.863, the bulbs will now "chain" DDP packets.  If a DDP packet has enough channel data for more than one bulb, the bulb will take the first three channels (R,G,B) for itself and send the remaining data to the next higher IP address.

## Advanced Settings
When using kauf-bulb.yaml as a package in the ESPHome dashboard, you can configure the following aspects by adding substitutions. The substitutions section of kauf-bulb.yaml has comments with more explanation as well.

***friendly_name*** - The friendly name will be used to name every entity in Home Assistant. Add a substitution to change this to something descriptive for each device.

***disable_entities*** - Adding a substitution to redefine this to "false" will result in all entities being automatically enabled in Home Assistant.

***light_restore_mode*** - Defines the state of the bulb on boot-up.  The select entity needs to remain in the "YAML Configured" setting for this substitution to be effective, otherwise the bootup automation will overwrite this setting.  For more information on the available options, see restore_mode under [Base Light Configuration](https://esphome.io/components/light/index.html#base-light-configuration).
- RESTORE_DEFAULT_OFF is the default and equivalent to the select entity option "Restore Power Off State"
- RESTORE_AND_ON is equivalent to the select entity option "Always On - Last Value"
- ALWAYS_ON is equivalent to the select entity option "Always On - Bright White"
- RESTORE_AND_OFF is equivalent to the select entity option "Always Off"

***default_power_on_state*** - defines the default option for the power on state select entity.

***sub_on_turn_on*** - define an ESPHome script to execute when the light turns on.

***sub_on_turn_off*** - define an ESPHome script to execute when the light turns off.


## Factory Reset
Going to the bulb's URL in a web browser and adding /reset will completely wipe all settings from flash memory.

## Clearing Wi-Fi Credentials to Get Wi-Fi AP Back
You can clear the bulb's Wi-Fi credentials and get the Wi-Fi AP back to configure new credentials by rebooting the bulb five times in a row, with each boot lasting less than 10 seconds.  On the 2nd, 3rd, and 4th such boot the bulb will flash yellow to let you know its working.  On the 5th such boot the bulb will turn red, reset the credentials, and reboot to put up its Wi-Fi AP


## Troubleshooting
Some bulbs may begin color cycling when initially powered on.  This is due to a failure to clear the factory test routine when the bulbs were manufactured.  The color cycling will automatically end after 10 minutes.  The color cycling can also be terminated by flashing the bulb via an update bin file or the ESPHome dashboard.  Firmware version 1.86 adds a button in the web interface to stop the factory test routine.

General troubleshooting ideas applicable to all products are located in the [Common repo's readme](https://github.com/KaufHA/common/blob/main/README.md#troubleshooting).

## Recommended Tasmota Template

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
