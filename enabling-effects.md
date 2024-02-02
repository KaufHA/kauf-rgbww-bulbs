# Enabling and Developing Custom Effects

> NOTE: You could brick or otherwise damage your Kauf bulbs.
> Only attempt this if you accept the risks.

This guide will walk through enabling the built-in ESPHome
light effects, as well as provide a sample custom lambda effect.
This requires building custom firmware
and flashing your bulb and as such you should be familiar
with code development.

This guide assumes your build environment is Linux.

> NOTE: If you already have the ESPHome dashboard setup, you can
> skip directly to the "Adding Stock Effects" section
> which describes using !extend and use said dashboard
> to add effects. (Using the ESPHome dashboard is outside
> the scope of this guide.)

## Overview

- Setup Build Environment
- Validate Build Environment
- Send un-altered firmware over the air (OTA)
- Enable built-in ESPHome effects.
- Create simple lambda effect.

## Setup Build

### Setup ESPHome Build

Follow the directions on the ESPHome web site to get a build
environment configured. You will end up with a python
virtual environment. We'll call ours venv-esphome-kauf.

Link: [Getting Started with the ESPHome Command Line](https://esphome.io/guides/getting_started_command_line)

In Linux, this essentially works out to be:

```shell
mkdir -pv ~/src
cd ~/src
python3 -m venv venv-esphome-kauf
source venv-esphome-kauf/bin/activate
pip3 install esphome
```

### Clone the Kauf Repository

> Of course you can branch the repository in github and check
> out that branch so you can save your work back to your
> own github repository.

The easiest place to checkout to is either within or
next to the ESPHome virtual environment. In our example,
we will checkout the code in the same `src` directory.

```shell
cd ~/src
git clone https://github.com/KaufHA/kauf-rgbww-bulbs.git
```

Now we have:

```plain
~/src
  venv-esphome-kauf
  kauf-rgbww-bulbs
```

Make a branch for your custom alterations and switch
to that branch.

```shell
cd ~/src/kauf-rgbww-bulbs
git branch with-custom-effects
git checkout with-custom-effects
```

### Do a Test Build

Before making changes, ensure you have a working build
environment and that the build succeeds.

```shell
esphome run kauf-bulb-factory.yaml
```

The first build will take a while and will end with asking
how you want to upload the firmware. Exit out with CTRL+c.

> Every time you open a new shell, remember to source
> the virtual environment otherwise you will get a
> "command not found" error.

### Configure Upload

Get a Kauf bulb you want to experiment with. Power
it up and get the IP address of it. Add a line
to your `/etc/hosts` file replacing the IP in the
example below with the IP of your Kauf bulb:

```plain
192.168.0.34    kauf-bulb.local
```

### Create a Custom Config

Copy the `kauf-bulb-update.yaml` to a new file. This will
allow you to keep pulling updates from github while you
play with your effects. It will also enable you to create
a .bin.gz which you can update your bulbs with via the
web interface.

```shell
cp kauf-bulb-update.yaml kauf-bulb-with-custom-effects.yaml
```

You may *optionally* alter the wifi section in your
copied file to have your access
point information. This should not be needed with the
"forced_hash" but acts as extra insurance that you will
not need to re-add your bulb.

```yaml
wifi:
  ssid: SomeSID
  password: secret
```

### Build and Upload

With your bulb powered on, run:

```shell
esphome run kauf-bulb-with-custom-effects.yaml
```

and select the "Over The Air" option
to upload the new firmware. After it has uploaded
and the bulb has rebooted, it should behave as
before.

> After the reboot, the console will show the debug log.
> You may safely CTRL+c to exit out of viewing the log.

## Adding Stock Effects

Edit your `kauf-bulb-with-custom-effects.yaml` and add the following to the end:

```yaml
light:
  - id: !extend  kauf_light
    #https://esphome.io/components/light/#light-effects
    effects:
      - flicker:
          name: Flicker 12%
          alpha: 94%
          intensity: 12%
      - strobe:
      - pulse:
      - random:
          name: Random Colors
```

Build and upload the new firmware (again using the esphome command from above).
Now when you visit the built-in page for the bulb, the top "Kauf Bulb" row
of the table will have a drop-down to select your effect.
From Home Assistant you can also activate the effects by name.

## Sample Lambda

Lambda effects are c++ code, stored in YAML files,
which gets called at the rate specified
by the lambda configuration. A couple points:

- Do not loop indefinitely in the lambda. ESPHome is essentially a cooperative
  multitasking system so you must yield the thread.
- Set the `update_interval` to 1ms, or write a native c++ method if
  you need updates as fast as possible.

We will create a simple "on-off" red light alert. Add the following
after the "-random" effect:

```yaml
      - lambda:
          name: "Red Alert"
          update_interval: 1000ms
          lambda: |-
            // static to maintain the value from run-to-run.
            static bool is_light_on = false;

            // Example of how to write to the debug log and do something only the first time.
            static ColorMode start_color_mode = ColorMode::UNKNOWN;
            if (initial_run) {
              // ESP_LOGD("lambda", "Initial Run");
              start_color_mode = id(kauf_light).current_values.get_color_mode();
            }

            auto light = id(kauf_light);
            if (light->transformer_active) {
              // Still in the middle of a transition, so do nothing.
              // Also of note: If a new transition is started before
              // the running one finishes, it will cause an off-on
              // blink of the light.
              return;
            }

            auto call = id(kauf_light).make_call();
            call.set_color_mode(ColorMode::RGB);
            call.set_transition_length(0);

            if (is_light_on) {
              call.set_brightness(0.0);
              call.set_state(true);
              is_light_on = false;
            } else {
              call.set_brightness(1.0);
              call.set_rgbw(1.0, 0.0, 0.0, 0.0);
              call.set_state(true);
              is_light_on = true;
            }

            call.perform();
```

## Direct c++ Development

If you want to write your effects directly, a couple of points.

The core effects are in `components/light/base_light_effects.h` which is part of
the ESPHome source tree.

You will need to modify your custom config file to not pull the c++ code
every time a build is done. This is done by overriding the
`external_components` section to reference a local
components tree. The override is done by adding this to your
custom configuration:

```yaml
external_components:
  - source:
      type: local
      path: ./components
```

## END OF LINE
