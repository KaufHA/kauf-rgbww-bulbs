# KAUF RGBWW Smart Bulbs (BLF10/BRF30)

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


## Troubleshooting
Any build errors can usually be resolved by upgrading the ESPHome dashboard to the latest version.  On the days when ESPHome updates are released, it may take us up to 24 hours to make necessary changes to our custom components during which time you may see build errors with the new version.  Please be patient.  
  
If you are still getting errors after upgrading the ESPHome dashboard to the latest version, try deleting the .esphome/packages and .esphome/external_components subfolders from within the ESPHome config directory.

If you are stuck on ESPHome version 2022.3.1, you need to uninstall the Home Assistant Add-On, then reinstall using the [Official Add-On Repo](https://github.com/esphome/home-assistant-addon).

## Recommended Tasmota Template

```
{"NAME":"Kauf Bulb", "GPIO":[0,0,0,0,416,419,0,0,417,420,418,0,0,0], "FLAG":0, "BASE":18, "CMND":"SO105 1|RGBWWTable 204,204,122,153,153"}
```

## Links
- [Purchase A21 bulbs on Amazon](https://www.amazon.com/dp/B09GV9FD3X)
- [Purchase BR30 bulbs on Amazon](https://www.amazon.com/dp/B09L5P2MDD)
- [KAUF YouTube Channel](https://www.youtube.com/channel/UCjgziIA-lXmcqcMIm8HDnYg)
- [KaufHA Website](https://kaufha.com/blf10/)
