/** Serial port interface class
*   Author: Brian Walton
*   Date: 2018-01-20
*   License: LGPL
*/

#pragma once
#include <string> //provides std::string class
#include <map> //provides std::map
#include <sys/termios.h> //provides terminal constants
#include <vector>

const static unsigned int SERIAL_INPUT = 1;
const static unsigned int SERIAL_OUTPUT = 2;

using namespace std;

class Serial
{
    public:
        Serial();
        virtual ~Serial();

        /** @brief  Open serial port
        *   @param  sName Port name
        *   @param  nBaud Baud rate
        *   @param  nBits Quantity of bits per word
        *   @param  sParity Parity (Default: none)
        *   @param  nStop Quantity of stop bits
        *   @retval bool True on success
        *   @note   Omit later parameters to use default or preset values
        *   @note   Set any value to empty string or zero to use default or preset values (except nStop which should be ommitted)
        */
        bool Open(string sName = "", unsigned int nBaud = 0, string m_sParity = "", unsigned int nBits = 0, unsigned int nStop = 99);

        /** @brief  Close serial port
        *   @retval bool True on success
        */
        bool Close();

        /** @brief  Set serial port device name, e.g. /dev/ttyS4
        *   @param  sPort Name of serial port device
        */
        void SetPort(string sPort);

        /** @brief  Set the baud
        *   @param  nBaud Baud rate
        *   @retval bool True on success
        */
        bool SetBaud(speed_t nBaud);

        /** @brief  Set the word length
        *   @param  nBits Quantity of bits in each word
        *   @retval bool True on success
        */
        bool SetWord(unsigned int nBits);

        /** @brief  Set parity
        *   @param  sParity (e|even|o|odd|n|none)
        *   @retval bool True on success
        */
        bool SetParity(string m_sParity);

        /** @brief  Set the stop bits
        *   @param  nBits Quantity of stop bits
        *   @retval bool True on success
        */
        bool SetStopBits(unsigned int nBits);

        /** @brief  Read data from the serial port
        *   @param  pBuffer Pointer to a buffer to populate with data
        *   @param  nSize Maximum quantity of characters / bytes to read from port (Default: 1)
        *   @retval int Quantity of characters / bytes actually read from port
        *   @note   Can use Read(&myChar) to read a single char
        */
        int Read(unsigned char *pBuffer, unsigned int nSize = 1);

        /** @brief  REad data from serial port
        *   @param  vBuffer Vector to hold data
        *   @param nSize Maximum quantity of characters / bytes to read from port. Zero to read all available data (Default: 0)
        *   @retval int Quantity of characters / bytes read
        */
        int Read(vector<unsigned char>& vBuffer, unsigned int nSize = 0);

        /** @brief  Read string from serial port
        *   @param  pString Pointer to a string to populate
        *   @retval int Length of string.
        */
        int Read(string *pString);

        /** @brief  Write data to the serial port
        *   @param  pBuffer Pointer to a buffer containing data to write
        *   @param  nSize Quantity of characters / bytes to write
        *   @retval bool True on success
        *   @note   Ensure the buffer at least nSize chars in size
        */
        bool Write(const char *pBuffer, unsigned int nSize = 1);

        /** @brief  Write data to the serial port
        *   @param  vBuffer Vector holding data to write
        *   @retval bool True on success
        */
        bool Write(vector<unsigned char>& vBuffer);

        /** @brief  Write a string to the serial port
        *   @param  sData
        *   @retval bool True on success
        */
        bool Write(string sData);

        /** @brief  Write single byte / character to serial port
        *   @param  cData
        *   @retval bool True on success
        */
        bool Write(const char& cData);

        /** @brief  Set RTS line
        *   @param  bValue Set true to assert RTS
        */
        void SetRts(bool bValue);

        /** @brief  Set DTR line
        *   @param  bValue Set true to assert DTR
        */
        void SetDtr(bool bValue);

        /** @brief  Set verbosity of output
        *   @param  bVerbose True to output info. False for silent operation
        */
        void SetVerbose(bool bVerbose = true);

        /** @brief  Report if serial port is open
        *   @retval bool True if serial port is open
        */
        bool IsOpen();

        /** @brief  Empty the recieve buffer
        *   @param  nDirection Which buffer to flush (default: SERIAL_INPUT | SERIAL_OUTPUT)
        */
        void Flush(unsigned int nDirecton = (SERIAL_INPUT | SERIAL_OUTPUT));

    protected:

    private:
        bool GetAttributes(); // Populate m_tty with port attibutes. Returns true on succes
        bool SetAttributes(); // Sets port attibutes from m_tty. Returns true on succes
        void PopulateBaud(); // Populates map of valid baud rates
        bool m_bVerbose; //True for verbose output
        int m_nFd; //File descriptor for serial port
        speed_t m_nBaud; //Baud rate
        unsigned int m_nWordLength; //Word length
        string m_sPort; //Name of serial port
        string m_sParity; //Parity
        unsigned int m_nStopBits; //Quantity of stop bits
        termios m_tty; //Port attributes
        map<unsigned int,speed_t> m_mBaud; //Map of real baud to tty baud value
};
