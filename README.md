# LVGL on ESP32 with MicroPython (as a USER_C_MODULE)

This repository is an attempt to create an out-of-the-box LVGL experience on ESP32 files using MicroPython.

## Rationale

While there are a lot of libraries, examples and information available on getting LVGL working on MicroPython there is a
huge amount of fragmented, outdated and non-working code out there.

After spending several days getting increasingly frustrated about the state of things I decided to fragment a bit more
and create my own module with the following requirements:

- Must compile as a USER_C_MODULE
- Works with latest release of LVGL
- Works with latest release of MicroPython
- Works with latest (supported by MicroPython) release of ESP-IDF 
- Uses the `esp_lcd` driver of ESP-IDF with an SPI bus
- Allows sharing SPI for example with SD card

There are currently no plans to support 
- non-ESP32 devices 
- other than the standard `esp_lcd` displays
- other buses than SPI
- other displays than ST7789 as I do not possess any to test

The final objective of this project is to support the camp badges for [Fri3d Camp](https://www.fri3d.be) and as such
certain functionality you'd expect from a display driver might be missing as they are not necessary for that project.

Don't hesitate to create a pull request to add the required functionality. 

## Board configuration

This wrapper requires that you activate the following options in your `sdkconfig.board`:

```
# lv_tick_inc is called from an esp_timer configured with interrupts
CONFIG_ESP_TIMER_SUPPORTS_ISR_DISPATCH_METHOD=y
```
## Building

First follow the MicroPython documentation to create a build for your device without this module and verify it is
working.

Once you are sure it does, clone the repository with recursive submodules, this makes sure 

```shell
git clone --recurse-submodules <repository-url>
```

Then you need to create or alter your partition table, app partition (where binary micropython.bin exist) size need 0x260000 at least(depend on your project), following is an example:
'''  - # Name,   Type, SubType, Offset,  Size, Flags
  - nvs,      data, nvs,     0x9000,  0x6000,
  - phy_init, data, phy,     0xf000,  0x1000,
  - factory,  app,  factory, 0x10000, 0x4F0000,
  - vfs,      data, fat,     0x500000, 0x500000,

  - Part 'factory' 0/0 @ 0x10000 size 0x1f0000 (overflow 0x62440)
'''
Compile adding the `USER_C_MODULES` parameter to the `make` command.

```shell
make USER_C_MODULES=/path/to/lvgl_esp32_mpy/micropython.cmake <other options>
```

## Broken things

* [Soft-reboots are not working](https://github.com/lvgl/lv_binding_micropython/issues/343) 

## Missing things

Things I'll probably still implement at some point:

- Rotation of displays

## Supported versions

- LVGL: 9.1
- MicroPython: 1.23.0
- ESP-IDF: 5.2.2

## Acknowledgements

A lot of ideas and/or work were borrowed from

- [kdschlosser](https://github.com/kdschlosser/lvgl_micropython)'s LVGL binding
- [russhughes](https://github.com/russhughes/s3lcd)'s s3lcd
- The original [lv_binding_micropython](https://github.com/lvgl/lv_binding_micropython)
