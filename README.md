# ribanEspTool
ESP8266 Tool written in C++

**DO NOT USE THIS YET**
This code is currently under development and should not yet be considered useable. See [status](status table) below for current functionality.

## What is ribanEspTool?
ribanEspTool provides a set of tools to interface with an [Espressif ESP8266](https://www.espressif.com/en/products/hardware/esp8266ex/overview) microcontroller module.

Features:
* Flash firmware
* Erase flash memory
* Reset module
* Read information from module
* Convert object code to firmware image
* Simple terminal emulator

## How do I get ribanEspTool?
Currently ribanEspTool is only available as source code from [github](https://github.com/riban-bw/ribanEspTool). A Code::Blocks project is included which *may* build the project on a POSIX OS but is likely to require tweaks to make it work. Core development is currently using Cygwin on Microsoft Windows. A GNU Makefile based build is planned.

## How do I use ribanEspTool?
General usage of ribanEspTool is to provide a command as a paramter then further switches or parameters as required by that command.

ribanEspTool has built-in help. Use the -h or --help command line switch to show general usage.

`ribanEspTool --help`

Each command has usage description, e.g.

`ribanEspTool reset -h`

## Why create ribanEspTool?
This is a port of [esptool.py](https://github.com/espressif/esptool) to C++. The goal is to remove the dependency on Python which, although seemingly ubiquitous, adds a dependenacy that some users / projects may find undesirable. This project also aims to add functionality required by [SMING](https://github.com/SmingHub/Sming) not currently supported by esptool.py such as incorporating [Richard Burton's](http://richard.burtons.org/) esptool2 ROM image creation features and a simple terminal.

ribanEspTool is written in C++ for POSIX (style) operating systems including GNU/Linux, BSD, MacOS, Cygwin, etc. To run on Microsoft Windows, first install Cygwin and run from a Cygwin session, e.g. Cygwin Bash or Cygwin mingw32-make.

## Who is ribanEspTool created for and by?
Users will be ESP8266 firmware developers. This is a tool which compliments and may be used as part of a build environment such as [SMING](https://github.com/SmingHub/Sming).
ribanEspTool is written by Brian Walton developed from code originally written by Fredrik Ahlberg (@themadinventor).

## When was ribanEspTool written and when will it be updated or finished?
in September 2016 development of a Java GUI application to interface with the ESP8266 called [jesper](https://github.com/riban-bw/jesper/commits/master) began. The goal was to make a cross-platform, simple to use tool. In January 2018 development of ribanEspTool began, derived from the work done for jesper (which at this time remains incomplete but partially functional). Effort was focused on POSIX compatibility and low dependency, i.e. no Java / Python, just a single executable application.

Today (2018-01-26) the code is under active development in alpha stage, i.e. does not provide sufficient functionality to be considered a test candidate. There is no schedule for development and release and the code may never be completed. (No code ever is right?) It is hoped that a beta version may be available before the end of February 2018 but no guarantee.

### Status

|Function|Status|
|--------|------|
|Help|Functional|
|Version|Functional|
|Port|Functional|
|Baud|Functional|
|Verbose|Functional|
|Quiet|Functional|
|reset|Functional|
|write_flash|Not functional|
|Run|Not functional|
|elf2image|Not functional|
|read_mac|Not functional|
|chip_id|Not functional|
|flash_id|In progress|
|read_flash|Not functional|
|erase_flash|Not functional|

## Where can I find out more about ribanEspTool
ribanEspTool source code, issue tracker and wiki are hosted on [github](https://github.com/riban-bw/ribanEspTool). Please reporte issues and feature requests via the [issue tracker](https://github.com/riban-bw/ribanEspTool/issues). Enhancements and bug fixes may be submitted by means of git pull requests.

Core developer is Brian Walton (@riban-bw) who may be contacted via [gitter](https://gitter.im/).
