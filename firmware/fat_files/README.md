# Pico DRAM Tester USB Flash Storage

## Overview

This folder is backed by a small FAT12 partition in the PICO 2's in-built
flash storage. It's main goal is to allow users to configure
settings and timings that were hitherto hard coded.

You can easily restore the contents of this drive to factory defaults
by deleting SYSTEM.CFG and powercycling the board.

## Known Limitations

### THERE IS CURRENTLY NO WEAR LEVELING SUPPORT

So please don't use this drive for anything other than storing configuration
files, yet. And maybe avoid having any files open for too long on your host OS.

### Writes to the storage will wedge the Pico 2's Second Core

The aim is to fix this. But the simple workaround for now is to power
cycle your dram tester after making any config changes. Otherwise, any
rem test you select will hang indefinitely.

## USB Serial Logging Support

As we use the TinyUSB library to enable mass storage, it was considered useful
to also add serial out over USB. Check you host's device manager for connection
options. Using a terminal or serial monitor (such as VSCode's) with
auto-reconnect enabled will allow you to see potentially useful logging
of what your Pico 2 is getting up to under the...  I was gonner say
bonnet/hood, but I haven't seen a top cover for this project yet. :-).
