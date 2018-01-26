/*  Defines ESP8266 class
*   Provides interface to ESP8266 via serial port
*/
#pragma once
#include "serial.h"

using namespace std;

	// Request types
	const static int ESP_MSGTYPE_COMMAND  = 0x00;
	const static int ESP_MSGTYPE_RESPONSE = 0x01;

    // These are the currently known commands supported by the ROM
	const static int ESP_OP_NONE        = 0x00;
	const static int ESP_OP_FLASH_BEGIN = 0x02;
	const static int ESP_OP_FLASH_DATA  = 0x03;
	const static int ESP_OP_FLASH_END   = 0x04;
	const static int ESP_OP_MEM_BEGIN   = 0x05;
	const static int ESP_OP_MEM_END     = 0x06;
	const static int ESP_OP_MEM_DATA    = 0x07;
	const static int ESP_OP_SYNC        = 0x08;
	const static int ESP_OP_WRITE_REG   = 0x09;
	const static int ESP_OP_READ_REG    = 0x0a;

    // Maximum block sized for RAM and Flash writes, respectively.
	const static int ESP_RAM_BLOCK   = 0x1800;
	const static int ESP_FLASH_BLOCK = 0x400;

    // Default baud rate. The ROM auto-bauds, so we can use more or less whatever we want.
	const static int ESP_ROM_BAUD    = 115200;

    // First byte of the application image
	const static int ESP_IMAGE_MAGIC = 0xe9;

    // Initial state for the checksum routine
	const static int ESP_CHECKSUM_MAGIC = 0xef;

    // OTP ROM addresses
	const static int ESP_OTP_MAC0    = 0x3ff00050;
	const static int ESP_OTP_MAC1    = 0x3ff00054;
	const static int ESP_OTP_MAC2    = 0x3ff00058; //By inference
    const static int ESP_OTP_MAC3    = 0x3ff0005c;

    // Flash sector size, minimum unit of erase.
    const static int ESP_FLASH_SECTOR = 0x1000;
    const static int ESP_FLASH_SECTOR_PER_BLOCK = 16;
    // Message header
    const static int ESP_HEADER_SIZE     = 8;
    const static int ESP_HEADER_MSG_TYPE = 0; //uint8 Defines the type of message
    const static int ESP_HEADER_OP       = 1; //uint8 Defines the operation to perform in a command message
    const static int ESP_HEADER_LEN      = 2; //uint16 Size of message payload
    const static int ESP_HEADER_CHECKSUM = 4; //uint32 Checksum of payload (command message)
    const static int ESP_HEADER_VALUE    = 4; //uint32 Value (response message)

    // Timeouts
    const static int ESP_RESPONSE_RETRY  = 100; //How many times we try to get a response
    const static int ESP_SLIP_TIMEOUT    = 500; //How many times we try to get a response

class ESP8266
{
    public:
        /** Instantiate an instance of the ESP8266 class
        *   @param  sPort Name of serial port, e.g. /dev/ttyS4
        *   @param  nBaud Baud rate of serial port, e.g. 115200
        */
        ESP8266(string sName, unsigned int nBaud);
        virtual ~ESP8266();

        /** @brief  Opens the ESP8266 connected serial port
        *   @retval bool True on success
        *   @note   ESP8266 serial functions will open port automatically. Only call this if application needs to respond early top port open failure.
        */
        bool Open();

        /** @brief  Hardware reset using RTS / DTR signals
        *   @param  bFlash True to set to flash mode. False to set to run mode (Default: false)
        *   @retval bool True on success
        */
        bool Reset(bool bflash = false);

        /** @brief  Set verbose mode
        *   @param  bVerbose True to output more verbose messages
        */
        void SetVerbose(bool bVerbose) {m_bVerbose = bVerbose;};

        /** @brief  Set silent mode
        *   @param  bSilent True to suppress all output (including error messages)
        */
        void SetSilent(bool bSilent) {m_bSilent = bSilent;};

        /** @brief  Report if ESP8266 connected serial port is open
        *   @retval True if serial port is open
        */
        bool IsOpen() {return m_pSerial->IsOpen();};

        /** @brief  Get the ESP8266 connected serial port
        *   @retval Serial Pointer to serial port
        *   @note   Allows direct control of serial port - treat with care
        */
        Serial* GetSerial() {return m_pSerial;};

        /** @brief  Send a command to the ESP8266
        *   @param  nCommand Command ID (See ESPCOMMAND)
        *   @param  vData Vector containing data to send or receive
        *   @param  nChecksum Checksum of data
        *   @retval bool True on success
        */
        bool SendCommand(int nCommand, vector<unsigned char>& vData, int nChecksum = 0);

        /** Read the MAC address of the ESP8266
        *   @retval string MAC address as colon separated string, e.g. 12:34:56:78:9A:BC
        */
        string ReadMac();

        /** Read the chip ID of the ESP8266
        *   @retval unsigned int Chip ID
        */
        unsigned int ReadId();

        /** @brief  Validate or calculate checsum of data block
        *   @param  pData Pointer to data block to check
        *   @param  nSize Quantity of elements in data block
        *   @param  nChecksum Checksum value to validate (set to zero to calculate checksum of data block)
        *   @retval int Calculated checksum or zero if calculated checksum is same as checksum parameter
        */
        static int Checksum(int *pData, unsigned int nSize, int nChecksum);

    protected:

    private:
        /** @brief  Synchronise ESP8266 serial port with current baud
        *   @retval bool True on success
        */
        bool Sync();

        /** @brief  Connect to ESP8266 in flash mode
        *   @retval bool True on success
        */
        bool Connect();

        /** @brief  Read a message from ESP8266, decoding using SLIP escaping
        *   @param  vBuffer Vector to hold received message
        *   @retval bool True on success
        */
        bool SlipRead(vector<unsigned char>& vBuffer);

        /** @brief  Reads from an ESP8266 register
        *   @param  nAddress Register address
        *   @retval int Value in register or zero on failure
        */
        int ReadReg(int nAddress);

        /** @brief  Write to an ESP8266 register
        *   @param  nAddress Register address
        *   @param  nValue Value to write to register
        *   @retval bool Ture on success
        */
        bool WriteReg(int nAddress, int nValue);

        /** @brief  Convert 4 consecutive bytes from a vector to an integer
        *   @param  vBuffer Vector containing the source data
        *   @param  nStart Position of first element in vector to convert
        *   @retval int Converted value
        */
        int ToInteger(vector<unsigned char>& vBuffer, unsigned int nStart);

        /** @brief  Convert an integer to big-endian bytes placing in a vector
        *   @param  nValue Integer to convert
        *   @param  vBuffer Vector in which to place result
        *   @param  nStart Position of first element at which to place results in vector (default: 0)
        */
        void FromInteger(int nValue, vector<unsigned char>& vBuffer, unsigned int nStart = 0);

        Serial* m_pSerial; // Pointer to serial port
        bool m_bConnected; //True if connected to ESP8266 in flash mode
        bool m_bVerbose; //True to provide verbose output
        bool m_bSilent; //True to supress all output
};
