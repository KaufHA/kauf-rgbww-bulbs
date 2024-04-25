# Enabling and Developing Custom Effects

> NOTE: You could brick or otherwise damage your Kauf bulbs.
> Only attempt this if you accept the risks.

This guide will walk through enabling the built-in ESPHome
light effects, as well as creating a sample custom effect.
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
- Convert the lambda to c++ with configurable options.
- Custom power-on state.
- Deploy to other bulbs.

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
not need to re-add your bulb to the network.

```yaml
wifi:
  ssid: SomeSID
  password: secret
```

> Remember: Do not push your custom config to a public
> git repository if you put a password in the config file.

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
            if (light->is_transformer_active()) {
              // Still in the middle of a transition, so do nothing.
              //
              // NOTE: If a new transition is started before
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

We will re-implement the red alert in c++.

The core effects are in `components/light/base_light_effects.h`
which is originally from the ESPHome source tree.
This is a good place to get code hints.

The .h file contains full class definitions with methods to simplify
development by not needing to keep a .cpp file and .h file in sync.
We will follow that paradigm in our code as well for simplicity.
In other words: if you name our effects file with the .cpp extension, you will need
to create a .h file with class/method signatures for compilation
to succeed.

### Use Local Components

We need to modify the custom config file to not pull the c++ code
every time a build is done, and more importantly to use the local components directory.
This is accomplished by overriding the `external_components` section to reference a local
components tree. Add the following to your custom configuration:

```yaml
external_components:
  - source:
      type: local
      path: ./components
```

### Create an Effects C++ Source File

To allow us to continue to pull updates from upstream, we
will put our effects in a new file.
As mentioned above, to simplify our development,
we will follow the lead of ESPHome and put the full class
and method implementation in a .h file.
We will call it `custom_light_effects.h`.
The file should be placed in `./components/light`.

```c++
#include <utility>
#include <vector>

#include "esphome/core/automation.h"
#include "esphome/core/log.h"
#include "light_effect.h"
#include "color_mode.h"
namespace esphome
{
  namespace light
  {
    class RedAlertEffect : public LightEffect
    {
    public:
      explicit RedAlertEffect(const std::string &name) : LightEffect(name) {}

      void start() override
      {
        this->is_light_on_ = true;

        // We do not use this...this is here as an example.
        this->state_->current_values.get_color_mode();

        if (this->red_ > 0.0f || this->green_ > 0.0f || this->blue_ > 0.0f)
        {
          ESP_LOGD("RedAlertEffect", "Have a custom color  R: %.2f  G: %.2f  B: %.2f", this->red_, this->green_,
                   this->blue_);
        }
        else
        {
          ESP_LOGD("RedAlertEffect", "No custom color, using red.");
          this->red_ = 1.0f;
          this->green_ = 0.0f;
          this->blue_ = 0.0f;
        }
      }

      void stop() override
      {
        ESP_LOGD("RedAlertEffect", "stop");
      }

      void apply() override
      {
        if (this->state_->is_transformer_active())
        {
          // Still in the middle of a transition, so do nothing.
          //
          // NOTE: If a new transition is started before
          // the running one finishes, it will cause an off-on
          // blink of the light.
          return;
        }

        // Lifted from lambda to only change on the given frequency.
        const uint32_t now = millis();
        if (now - this->last_run_ < this->update_interval_)
        {
          return;
        }
        this->last_run_ = now;

        auto call = this->state_->make_call();
        call.set_color_mode(ColorMode::RGB);
        call.set_transition_length(0);

        if (this->is_light_on_)
        {
          call.set_brightness(0.0);
          call.set_state(true);
          this->is_light_on_ = false;
        }
        else
        {
          call.set_brightness(1.0);
          call.set_rgbw(this->red_, this->green_, this->blue_, 0.0f);
          call.set_state(true);
          this->is_light_on_ = true;
        }
        call.perform();
      }

      void set_red(float red) { this->red_ = red; }
      void set_green(float green) { this->green_ = green; }
      void set_blue(float blue) { this->blue_ = blue; }

    protected:
      // State
      bool is_light_on_ = true;
      ColorMode start_color_mode_ = ColorMode::UNKNOWN;
      uint32_t last_run_{0};

      // Config
      uint32_t update_interval_ = 1000;
      float red_;
      float green_;
      float blue_;
    };

  } // namespace light
} // namespace esphome
```

> It is left as an exercise to the reader to make the update interval configurable.

### Expose New Effect in Python

For this effect to become visible, types.py and effects.py need to be updated in a few places.

First, edit `types.py` and add a line in the '#Effects' section right after 'FlickerLightEffect'
for the RedAlert:

```python
RedAlertEffect = light_ns.class_("RedAlertEffect", LightEffect)
```

In `effects.py` find the `from .types import` section and add `RedLightEffect` to the list.

Next add a monochromatic effect in `effects.py`. This can be added anywhere in the file after
the variables are defined. It is suggested to add it right after the `FlickerLightEffect`
to keep it grouped with other monochromatic effects.

```python
@register_monochromatic_effect(
    "red_alert",
    RedAlertEffect,
    "RedAlert",
    {
        cv.Optional(CONF_RED, default=0.0):  cv.percentage,
        cv.Optional(CONF_GREEN, default=0.0): cv.percentage,
        cv.Optional(CONF_BLUE, default=0.0): cv.percentage,
    },
)
async def candle_effect_to_code(config, effect_id):
    var = cg.new_Pvariable(effect_id, config[CONF_NAME])
    cg.add(var.set_red(config[CONF_RED]))
    cg.add(var.set_green(config[CONF_GREEN]))
    cg.add(var.set_blue(config[CONF_BLUE]))
    return var
```

### Enable the Effect in the YAML File

Remove the lambda from your custom configuration file
and add the configurable c++ effect.

```yaml
light:
  - id: !extend kauf_light
    #https://esphome.io/components/light/#light-effects
    effects:
      - flicker:
          name: Flicker 12%
          alpha: 94%
          intensity: 12%
      - red_alert:
          name: Red Alert
      - red_alert:
          name: Green Alert
          green: 100%
```

Finally build, upload, and test.

## Custom Power-On State

If you want a custom color or state when the bulb is powered
on (i.e. by a physical switch), then you will need to create
a custom `on_boot` script.

First, copy and rename the existing `script_quick_boot` from
`kaulf-bulb.yaml` into your custom YAML file.
Then add the settings you want before the 10 second delay. Here is what
the script looks like with parts removed for brevity.

```yaml
script:
    # increment global_quick_boot_count if bulb stays on less than 10 seconds or never connects to wifi
    # reset wifi credentials if the counter gets to 5
  - id: script_quick_boot_custom
    then:
    # ...code skipped...
            - delay: 4s
            - light.turn_off: kauf_light

      # Custom boot code
      - lambda: |-
          auto call = id(kauf_light).turn_on();
          call.set_transition_length(500);
          call.set_brightness(0.5);
          call.set_rgb(1.0, 0.5, 0.0);
          call.set_save(false);
          call.perform();
      # End custom boot code, the next line is in the original script.
      # wait 10 seconds
      - delay: 10s
```

Now configure `on_boot` to use this new script in your custom YAML file
(this section already exists toward the top):

```yaml
esphome:
  name_add_mac_suffix: true

  on_boot:
    then:
      - script.execute: script_quick_boot_custom
```

Other useful methods are:

```yaml
  call.set_color_mode_if_supported(ColorMode::COLOR_TEMPERATURE);
  call.set_color_temperature_if_supported(454.0f); // Warm White
  call.set_effect("Red Alert");
```

## Deploy to Other Bulbs

Once you are happy with your firmware, you can do OTA
updates to other Kauf bulbs using a bulb's web interface.
Use the esphome 'compile' command to build the firmware without uploading it.

```shell
esphome compile kauf-bulb-with-custom-effects.yaml
gzip -9vf .esphome/build/kauf-bulb/.pioenvs/kauf-bulb/firmware.bin
cp .esphome/build/kauf-bulb/.pioenvs/kauf-bulb/firmware.bin.gz ~/kauf-firmware-with-effects-v01.bin.gz
```

## Closing Thoughts

You should now be enabled to create your own effects.
Please share any you create on the Home Assistant
or ESPHome forums and thank you for your support.

## END OF LINE
