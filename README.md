# kauf-rgbww-bulbs

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


## Recommended Tasmota Template

```
{"NAME":"Kauf Bulb", "GPIO":[0,0,0,0,416,419,0,0,417,420,418,0,0,0], "FLAG":0, "BASE":18, "CMND":"SO105 1|RGBWWTable 204,204,122,153,153"}
```
