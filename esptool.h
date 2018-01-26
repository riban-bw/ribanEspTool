
#pragma once
#include <string>
#include <vector>
#include <map>
#include "esp8266.h"

enum COMMAND
{
    NONE,
    RESET,
    ERASE,
    FLASH,
    RUN,
    CHIP_ID,
    FLASH_ID,
    TERMINAL,
    ELF2IMAGE,
    MAC,
    READ_FLASH
};

using namespace std;

/** @brief  Parse the command line
*   @param  argc Quantity of command line parameters
*   @param  argv Pointer to each command line argument
*   @retval unsigned int Command to run
*/
COMMAND ParseCommandLine(int nCount, char** pArgs);

/** @brief Show software version
*/
void ShowVersion();

/** @brief  Show help for a command
*   @param  sCommand The command for which to show help
*/
void ShowHelp(COMMAND nCommand = COMMAND::NONE);

/** @brief  Hardware reset using RTS / DTR signals
*   @param  bFlash True to set to flash mode. False to set to run mode (Default: false)
*/
void Reset(bool bflash = false);

/** @brief  Create firmware image from compiled elf image
*   @param  sElf Filename of elf image
*   @param  sImage Filename of firmware image to create
*   @retval bool True on success
*/
bool Elf2Image(string sElf, string sImage);

/** @brief  Write a firmware image to ESP8266
*   @param  nOffset Address to write firmware
*   @param  sFilename Filename of the firmware image
*   @retval bool True on success
*/
bool WriteFlash(unsigned int nOffset, string sFilename);

/** @todo Implement functions:
*   load_ram
*   dump_mem
*   read_mem
*   write_mem
*   write_flash
*   run
*   image_info
*   make_image
*   elf2image
*   read_mac
*   chip_id
*   flash_id
*   read_flash
*   verify_flash
*   erase_flash
*   version - done
*/

//Global variables
bool g_bVerbose = false; //True for verbose output
bool g_bQuiet = false; //True to suppress all output
unsigned int g_nBaud = 115200; //Baud rate
string g_sPort = "/dev/ttyUSB0"; //Serial port device
string g_sAppName; //Application name
vector<string>g_vParameters; //Vector of command line parameters after command
map<unsigned int,string>g_mFirmwareMap; //Map of flash offset to firmware filenames
unsigned int g_nCpu = 40;
ESP8266* g_pEsp; //Pointer to serial port
