# kauf-rgbww-bulbs
Files for the KAUF RGBWW Smart Bulbs.

***components* directory** - Custom components needed to compile the KAUF RGBWW bulb firmware.  These are all stock ESPHome components that were modified by Kaufman Home Automation.  These don't need to be downloaded, the yaml files automatically grab them by reference to this github repo.

***config* directory** - Files needed in the ESPHome config directory to import a KAUF RGBWW bulb into your dashboard and keep basically all the stock KAUF functionality.

You need one copy of the yaml file per bulb, but only one total copy of each of the header files.  If you are importing multiple bulbs, download the yaml and two header files from this directory into your ESPHome config directory.  Copy and paste the yaml file once per bulb.  Rename the yaml files to something descriptive for each bulb, and edit the yaml files where indicated in the comments.

***config-minimal* directory** - Files needed in your config directory to import a KAUF bulb into your ESPHome dashboard using only stock ESPHome functionality.

You need one yaml file per bulb. If you are importing multiple bulbs, download the yaml file from this directory into your ESPHome config directory. Copy and paste the yaml file once per bulb. Rename the yaml files to something descriptive for each bulb, and edit the yaml files where indicated in the comments.

***config-update* directory** - Files needed in the ESPHome config directory to compile the update bin file for the KAUF RGBWW bulbs.
