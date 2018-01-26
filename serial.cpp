#include "serial.h"
#include <iostream> //provides std out for error logging (cerr)
#include <string.h> //provides error messages
#include <fcntl.h> //provides tty control
#include <sys/ioctl.h> //provides low-level control of serial port
#include <unistd.h> //provides open() (eventually)

Serial::Serial() :
    m_bVerbose(false),
    m_nFd(-1),
    m_nBaud(115200),
    m_nWordLength(8),
    m_sPort("/dev/ttyUSB0"),
    m_sParity("n"),
    m_nStopBits(1)
{
    PopulateBaud();
}

Serial::~Serial()
{

}

bool Serial::Open(string sPort, unsigned int nBaud, string sParity, unsigned int nBits, unsigned int nStop)
{
    if(sPort.compare("") != 0)
        m_sPort = sPort;
    if(nBaud != 0)
        m_nBaud = nBaud;
    if(sParity.compare("") != 0)
        m_sParity = sParity;
    if(nBits != 0)
        m_nWordLength = nBits;
    if(nStop != 99)
        m_nStopBits = nStop;
    if(m_nFd >= 0 && !Close())
    {
        //Error - cannot close port
        return false;
    }
    m_nFd = open(m_sPort.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if(m_nFd < 0)
    {
        if(m_bVerbose)
            cerr << "Failed to open port " << m_sPort << endl;
        return false;
    }
    if(!GetAttributes())
    {
        Close();
        if(m_bVerbose)
            cerr << "Failed to open serial port " << m_sPort << endl;
        return false;
    }
    m_tty.c_cflag |= (CLOCAL | CREAD); //Do not take ownership and enable read
    m_tty.c_cflag &= ~CSIZE; //Mask word length (set later)
    m_tty.c_cflag &= ~CRTSCTS; //Disable output hardware flow control
//    m_tty.c_cflag &= ~CRTSXOFF; //Disable input hardware flow control

    //!@todo review following tty configuration
    // Non-canonical mode
    m_tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    m_tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    m_tty.c_oflag &= ~OPOST;

    // Set low values for timing to recieve data straight away
    m_tty.c_cc[VMIN] = 1;
    m_tty.c_cc[VTIME] = 1;

    SetBaud(m_nBaud);
    SetWord(m_nWordLength);
    SetParity(m_sParity);
    SetStopBits(m_nStopBits);
    if(SetAttributes())
        return true;

    //Something isn't right so fail gracefully
    Close();
    if(m_bVerbose)
        cerr << "Failed to open serial port " << m_sPort << endl;
    return false;
}

bool Serial::Close()
{
    if(m_nFd <0)
        return true; //Aready closed
    close(m_nFd); //!@todo Does close provide return value?
    m_nFd = -1;
    return true;
}

void Serial::SetPort(string sPort)
{
    m_sPort = sPort;
}

bool Serial::SetBaud(unsigned int nBaud)
{
    if(m_nFd < 0)
        return false;
    auto it = m_mBaud.find(nBaud);
    if(it == m_mBaud.end())
    {
        if(m_bVerbose) cerr << "Invalid baud " << nBaud << endl;
        return false;
    }
    cfsetospeed(&m_tty, it->second);
    cfsetispeed(&m_tty, it->second);
    return SetAttributes();
}

bool Serial::SetWord(unsigned int nBits)
{
    if(m_nFd < 0)
        return false;
    switch(nBits)
    {
        case 5:
            m_tty.c_cflag &= ~CSIZE;
            m_tty.c_cflag |= CS5;
            break;
        case 6:
            m_tty.c_cflag &= ~CSIZE;
            m_tty.c_cflag |= CS6;
            break;
        case 7:
            m_tty.c_cflag &= ~CSIZE;
            m_tty.c_cflag |= CS7;
            break;
        case 8:
            m_tty.c_cflag &= ~CSIZE;
            m_tty.c_cflag |= CS8;
            break;
        default:
            if(m_bVerbose) cerr << "Invalid word length. Valid values: 5,6,7,8." << endl;
            return false;
    }
    return SetAttributes();
}

bool Serial::SetParity(string m_sParity)
{
    if(m_nFd < 0)
        return false;
    char cParity = '0';
    if(m_sParity.length() > 0)
        cParity = m_sParity[0];
    switch(cParity)
    {
        case 'e':
            m_tty.c_cflag |= PARENB;
            m_tty.c_cflag &= ~PARODD;
            break;
        case 'o':
            m_tty.c_cflag |= PARENB;
            m_tty.c_cflag |= PARODD;
            break;
        case 'n':
            m_tty.c_cflag &= ~PARENB;
            break;
        default:
            if(m_bVerbose) cerr << "Invalid parity. Valid values: e,even,o,odd,n,none." << endl;
            return false;
    }
    return SetAttributes();
}

bool Serial::SetStopBits(unsigned int nBits)
{
    if(m_nFd < 0)
        return false;
    switch(nBits)
    {
        case 1:
            m_tty.c_cflag &= ~CSTOPB;
            break;
        case 2:
            m_tty.c_cflag |= CSTOPB;
            break;
        default:
            if(m_bVerbose) cerr << "Invalid quantity of stop bits. Valid values: 1,2." << endl;
            return false;
    }
    return SetAttributes();

}

int Serial::Read(unsigned char *pBuffer, unsigned int nSize)
{
    if(m_nFd < 0 || nSize == 0)
        return 0;
    return read(m_nFd, pBuffer, nSize);
}

int Serial::Read(vector<unsigned char>& vBuffer, unsigned int nSize)
{
    if(!IsOpen())
        return 0; //!@todo Should we return -1 if port closed?
    unsigned char c;
    if(nSize == 0)
        while(read(m_nFd, &c, 1) == 1)
            vBuffer.push_back(c);
    return vBuffer.size();
}

int Serial::Read(string *pString)
{
    return 0; //!@todo Implement Read(string)
    //Need to figure out whether to block and how much to rad before we find a string terminator
}

bool Serial::Write(const char *pBuffer, unsigned int nSize)
{
    if(m_nFd < 0 || nSize == 0)
        return 0;
    return (write(m_nFd, pBuffer, nSize) > 0);
}

bool Serial::Write(vector<unsigned char>& vBuffer)
{
    for(vector<unsigned char>::iterator it = vBuffer.begin(); it != vBuffer.end(); ++it)
    {
        unsigned char c = *it;
        write(m_nFd, &c, 1);
    }
    return true;
}

bool Serial::Write(string sData)
{
    return Write(sData.c_str(), sData.length());
}

bool Serial::Write(const char& cData)
{
    return Write(&cData, 1);
}

void Serial::SetRts(bool bValue)
{
    if(m_nFd < 0)
        return;
    int nFlag = TIOCM_RTS;
    ioctl(m_nFd, bValue?TIOCMBIS:TIOCMBIC, &nFlag);
}

void Serial::SetDtr(bool bValue)
{
    if(m_nFd < 0)
        return;
    int nFlag = TIOCM_DTR;
    ioctl(m_nFd, bValue?TIOCMBIS:TIOCMBIC, &nFlag);
}

void Serial::SetVerbose(bool bVerbose)
{
    m_bVerbose = bVerbose;
}

bool Serial::IsOpen()
{
    return (m_nFd >= 0);
}

void Serial::Flush(unsigned int nDirection)
{
    if(!IsOpen())
        return;
    switch(nDirection)
    {
    case SERIAL_INPUT:
        tcflush(m_nFd, TCIFLUSH);
        break;
    case SERIAL_OUTPUT:
        tcflush(m_nFd, TCOFLUSH);
        break;
    default:
        tcflush(m_nFd, TCIOFLUSH);
        break;
    }
}

bool Serial::GetAttributes()
{
    if(m_nFd < 0 || tcgetattr(m_nFd, &m_tty) < 0)
    {
        if(m_bVerbose) cerr << "Failed to get port attributes - " << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool Serial::SetAttributes()
{
    if(m_nFd < 0 || tcsetattr(m_nFd, TCSANOW, &m_tty) != 0)
    {
        if(m_bVerbose) cerr << "Failed to set port attributes - " << strerror(errno) << endl;
        return false;
    }
    return true;
}

void Serial::PopulateBaud()
{
    #ifdef B50
    m_mBaud[50] = B50;
    #endif // B50
    #ifdef B75
    m_mBaud[75] = B75;
    #endif // B75
    #ifdef B110
    m_mBaud[110] = B110;
    #endif // B110
    #ifdef B134
    m_mBaud[134] = B134;
    #endif // B134
    #ifdef B150
    m_mBaud[150] = B150;
    #endif // B150
    #ifdef B200
    m_mBaud[200] = B200;
    #endif // B200
    #ifdef B300
    m_mBaud[300] = B300;
    #endif // B300
    #ifdef B600
    m_mBaud[600] = B600;
    #endif // B600
    #ifdef B1200
    m_mBaud[1200] = B1200;
    #endif // B1200
    #ifdef B1800
    m_mBaud[1800] = B1800;
    #endif // B1800
    #ifdef B2400
    m_mBaud[2400] = B2400;
    #endif // B2400
    #ifdef B4800
    m_mBaud[4800] = B4800;
    #endif // B4800
    #ifdef B9600
    m_mBaud[9600] = B9600;
    #endif // 9600
    #ifdef B19200
    m_mBaud[19200] = B19200;
    #endif // 19200
    #ifdef B38400
    m_mBaud[38400] = B38400;
    #endif // B38400
    #ifdef B57600
    m_mBaud[57600] = B57600;
    #endif // 57600
    #ifdef B115200
    m_mBaud[115200] = B115200;
    #endif // 115200
    #ifdef B128000
    m_mBaud[128000] = B128000;
    #endif // 128000
    #ifdef B230400
    m_mBaud[230400] = B230400;
    #endif // 230400
    #ifdef B256000
    m_mBaud[256000] = B256000;
    #endif // 256000
    #ifdef B460800
    m_mBaud[460800] = B460800;
    #endif // 460800
    #ifdef B9600
    m_mBaud[500000] = B500000;
    #endif // 500000
    #ifdef B576000
    m_mBaud[576000] = B576000;
    #endif // 576000
    #ifdef B921600
    m_mBaud[921600] = B921600;
    #endif // B921600
    #ifdef B1000000
    m_mBaud[1000000] = B1000000;
    #endif // B1000000
    #ifdef B1152000
    m_mBaud[1152000] = B1152000;
    #endif // B1152000
    #ifdef B1500000
    m_mBaud[1500000] = B1500000;
    #endif // B1500000
    #ifdef B2000000
    m_mBaud[2000000] = B2000000;
    #endif // B2000000
    #ifdef B2500000
    m_mBaud[2500000] = B2500000;
    #endif // B2500000
    #ifdef B3000000
    m_mBaud[3000000] = B3000000;
    #endif // B3000000
}
