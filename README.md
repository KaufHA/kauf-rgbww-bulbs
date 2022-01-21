# kauf-rgbww-bulbs

The recommended way to import a bulb into your ESPHome dashboard is through the dashboard import feature.  The bulb will need to be updated to 1.72 to be discovered by the ESPHome dashboard.  You can accomplish the same thing without updating first by using the following yaml modified as desired.

The friendly_name substitution is recommended and will need to be created in either case.

```
substitutions:
  name: bedroom-lamp
  friendly_name: Bedroom Lamp
  
packages:
  kauf.rgbww: github://KaufHA/kauf-rgbww-bulbs/kauf-bulb.yaml

esphome:
  name_add_mac_suffix: false

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
```


This repo contains files for the KAUF RGBWW Smart Bulbs.

***components* directory** - Custom components needed to compile the KAUF RGBWW bulb firmware.  These don't need to be downloaded, the yaml files automatically grab them by reference to this github repo.

***config-update* directory** - Files needed in the ESPHome config directory to compile the update bin file for the KAUF RGBWW bulbs.

***kauf-bulb.yaml* file** - yaml file needed to import a bulb into your ESPHome dashboard and keep basically all the stock KAUF functionality.

***kauf-bulb-minimal.yaml* file** - yaml file to import a bulb into your ESPHome dashboard using only stock ESPHome functionality.

