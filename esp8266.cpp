#include "esp8266.h"
#include <iostream>
#include <unistd.h> //provides usleep

ESP8266::ESP8266(string sPort, unsigned int nBaud) :
    m_bConnected(false),
    m_bVerbose(false),
    m_bSilent(false)
{
    m_pSerial = new Serial();
    m_pSerial->SetPort(sPort);
    m_pSerial->SetBaud(nBaud);
}

ESP8266::~ESP8266()
{
    m_pSerial->Close();
    delete m_pSerial;
}

bool ESP8266::Open()
{
    return m_pSerial->Open();
}

bool ESP8266::Reset(bool bFlash)
{
    /* DTR & RTS are inverted by serial port so we use the inverted DRT / RTS here
        ___ ___
       |DTR|RTS||RST|GP0|ACTION|
       | 0 | 0 || 1 | 1 |RUN   |
       | 0 | 1 || 1 | 0 |FLASH |
       | 1 | 0 || 0 | 1 |RESET |
       | 1 | 1 || 1 | 1 |RUN   |
    */
    if(m_bVerbose)
        cout << "Reseting ESP" << endl;
    if(!(m_pSerial->IsOpen() || m_pSerial->Open()))
        return false;
    //Reset !DTR=0 !RTS=1
    m_pSerial->SetDtr(false);
    m_pSerial->SetRts(true);
    usleep(50000);
    //Flash mode !DTR=1 !RTS=0
    m_pSerial->SetDtr(bFlash);
    m_pSerial->SetRts(false);
    usleep(50000);
    //Run mode DTR=1 RTS=1 (or DTR=0 RTS=0)
    m_pSerial->SetDtr(false);
    return true;
}

bool ESP8266::Sync()
{
    bool bSuccess = true;
    vector<unsigned char> vBuffer;
    vBuffer.resize(36);
    vBuffer[0] = 0x07;
    vBuffer[1] = 0x07;
    vBuffer[2] = 0x12;
    vBuffer[3] = 0x20;
    for(int nPos = 0; nPos < 32; ++nPos)
        vBuffer[nPos + 4] = 0x55;
    SendCommand(ESP_OP_SYNC, vBuffer, 0);
    vBuffer.clear();
    for(int nCycle = 0; nCycle < 7; ++nCycle)
        bSuccess &= SendCommand(ESP_OP_NONE, vBuffer);
    return bSuccess;
}

bool ESP8266::Connect()
{
    m_bConnected = false;
    if(m_bVerbose)
        cout << "Connecting to ESP8266..." << endl;
    //!@todo Set appropriate number of reset and sync attempts
    for(int nAttempt = 0; nAttempt < 4; ++nAttempt)
    {
        Reset(true); //Hardware reset to flash mode
        // worst-case latency timer should be 255ms (probably <20ms)
        usleep(255000);
        for(int nTry = 0; nTry < 4; ++nTry)
        {
            m_pSerial->Flush();
            if(Sync())
            {
                m_bConnected = true;
                return true;
            }
        }
    }
    return false;
}

int ESP8266::Checksum(int *pData, unsigned int nSize, int nChecksum)
{
    int nCalcChecksum = nChecksum;
    for(unsigned int nIndex = 0; nIndex < nSize; ++nIndex)
        nChecksum ^= pData[nIndex];
    return nCalcChecksum;
}

bool ESP8266::SlipRead(vector<unsigned char>& vBuffer)
{
    /*
        Get data from serial
        Check for header 0xc0
            return false if not found
            remove header
        Iterate through data
            Check for start of escape sequence 0xdb
                set escape sequence flag
                remove start of escape sequence byte
            If within escape sequence
                replace 0xdc with 0xc0
                replace 0xdd with 0xdb
                else report invalid slip sequence and return false
                clear escape sequence flag
            Check for footer 0xc0
                return result
    */
    bool bInEscapeSeq = false;
    m_pSerial->Read(vBuffer); //!@todo ADD TIMEOUT - gets stuck here
    if(vBuffer.size() == 0 || vBuffer[0] != 0xc0)
        return false; //!@todo Should we search for header rather than expect it to be first data element?
    //!@todo add serial read timeout
    for(vector<unsigned char>::iterator it = vBuffer.begin(); it != vBuffer.end(); ++it)
    {
        if(bInEscapeSeq)
        {
            bInEscapeSeq = false;
            if(*it == 0xdc)
                *it = 0xc0;
            else if(*it == 0xdd)
                *it = 0xdb;
            else
            {
                if(!m_bSilent)
                    cerr << "Invalid SLIP escape 0xdb " << "0x" << hex << *it << endl;
                return false;
            }
        }
        else if(*it == 0xdb)
        {
            bInEscapeSeq = true;
            vBuffer.erase(it);
        }
        else if(*it == 0xc0)
        {
            vBuffer.erase(it, vBuffer.end());
            return true;
        }
    }
    return false; //Didn't sequence terminator
}

bool ESP8266::SendCommand(int nOperation, vector<unsigned char>& vData, int nChecksum)
{
    vector<unsigned char> vBuffer = vData;
    /*Populate header
        byte message type
        byte operation code
        short length of payload
        int checksum
    */
    //Insert header in reverse order (keep pushing bytes at front)
    vBuffer.insert(vBuffer.begin(), nChecksum & 0xFF);
    vBuffer.insert(vBuffer.begin(), (nChecksum >> 8) & 0xFF);
    vBuffer.insert(vBuffer.begin(), (nChecksum >> 16) & 0xFF);
    vBuffer.insert(vBuffer.begin(), (nChecksum >> 24) & 0xFF);
    vBuffer.insert(vBuffer.begin(),(vData.size() >> 8) & 0xFF);
    vBuffer.insert(vBuffer.begin(), vData.size() & 0xFF);
    vBuffer.insert(vBuffer.begin(), nOperation); //request command operation
    vBuffer.insert(vBuffer.begin(), ESP_MSGTYPE_COMMAND); //Message type
    //!@todo check little / big endian
    //int[] checksum = toIntArray(checksum(data, 0));
    m_pSerial->Write(vBuffer);
    vBuffer.clear();
    //Try several times to get an appropriate header but not indefinitely
    for(int nCount = 0; nCount  < ESP_RESPONSE_RETRY; ++nCount)
    {
        if(!SlipRead(vBuffer))
           return false;
        if(vBuffer.size() < ESP_HEADER_SIZE)
            continue; //too short for a header
        if(vBuffer[ESP_HEADER_MSG_TYPE] != ESP_MSGTYPE_RESPONSE)
            continue; //not a response message
        if((nOperation == ESP_OP_NONE) || (vBuffer[ESP_HEADER_OP] == nOperation))
        {
            //Got the response we were looking for
            vBuffer.erase(vBuffer.begin(), vBuffer.begin() + 8);
            return true;
        }
    }
    return false;
}

int ESP8266::ReadReg(int nAddress)
{
    if(!m_bConnected && !Connect())
        return false;
    vector<unsigned char> vBuffer;
    FromInteger(nAddress, vBuffer, 0);
    bool bResult = SendCommand(ESP_OP_READ_REG, vBuffer, 0);
    if(!bResult)
    {
        if(m_bVerbose)
            cerr << "Failed to read register " << "0x" << hex << nAddress << endl;
        return 0; //!@todo This is an error state
    }
    return ToInteger(vBuffer, 0);
}

bool ESP8266::WriteReg(int nAddress, int nValue)
{
    if(!m_bConnected && !Connect())
        return false;
    vector<unsigned char> vBuffer;
    FromInteger(nValue, vBuffer);
    return SendCommand(ESP_OP_WRITE_REG, vBuffer, 0);
}

int ESP8266::ToInteger(vector<unsigned char>& vBuffer, unsigned int nStart)
{
    if(vBuffer.size() < nStart + 4)
        return 0; //!@todo This is an error state
    int nResult = vBuffer[nStart] << 24;
    nResult += vBuffer[nStart + 1] << 16;
    nResult += vBuffer[nStart + 2] << 8;
    nResult += vBuffer[nStart + 3] << 0;
    return nResult;
}

void ESP8266::FromInteger(int nValue, vector<unsigned char>& vBuffer, unsigned int nStart)
{
    while(vBuffer.size() < 4)
        vBuffer.insert(vBuffer.begin(), 0);
    vBuffer[nStart + 0] = (nValue >> 24) & 0xFF;
    vBuffer[nStart + 1] = (nValue>> 16) & 0xFF;
    vBuffer[nStart + 2] = (nValue>> 8) & 0xFF;
    vBuffer[nStart + 3] = (nValue>> 0) & 0xFF;
}

/*
string ESP8266::ReadMac()
{
    int nOui
    int nMac0 = ReadReg(ESP_OTP_MAC0)
    int nMac1 = ReadReg(ESP_OTP_MAC1)
    int nMac2 = ReadReg(ESP_OTP_MAC2)
    int nMac3 = ReadReg(ESP_OTP_MAC3)
    if(nMac3 != 0):
        long lMac = ((nMac3 >> 16) & 0xff, (mac3 >> 8) & 0xff, mac3 & 0xff)
    elif ((mac1 >> 16) & 0xff) == 0:
        oui = (0x18, 0xfe, 0x34)
    elif ((mac1 >> 16) & 0xff) == 1:
        oui = (0xac, 0xd0, 0x74)
    else:
        raise FatalError("Unknown OUI")
    return oui + ((mac1 >> 8) & 0xff, mac1 & 0xff, (mac0 >> 24) & 0xff)
}
*/
unsigned int ESP8266::ReadId()
{
    if(!m_bConnected && !Connect())
        return false;
    unsigned int nId0 = ReadReg(ESP_OTP_MAC0);
    unsigned int nId1 = ReadReg(ESP_OTP_MAC1);
    return (nId0 >> 24) | ((nId1 & 0xffffff) < 8);
}
