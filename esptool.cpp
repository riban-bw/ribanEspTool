#include "esptool.h"
#include "version.h"
#include <iostream>
#include <libgen.h> //provides file name manipulation
#include <getopt.h>
#include <unistd.h> //provides usleep
//#include <conio.h> //provides keyboard input

#include <sys/ioctl.h>
#include <stdio.h>
#include <sys/select.h>
#include <termios.h>
int _kbhit()
{
    static const int STDIN = 0;
    static bool bIninitialized = false;

    if(!bIninitialized)
    {
        cerr << "Initializing _kbhit" << endl;
        // Use termios to turn off line buffering
        termios term;
        tcgetattr(STDIN, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(STDIN, TCSANOW, &term);
        setbuf(stdin, NULL);
        bIninitialized = true;
    }
    int bytesWaiting;
    ioctl(STDIN, TIOCINQ, &bytesWaiting);
    return bytesWaiting;
}

int main(int argc, char** argv)
{
    g_sAppName = basename(argv[0]);
    if(g_bVerbose) cout << "Starting " << g_sAppName << endl;
    if(g_sAppName.compare("esptool") == 0)
    {
        if(g_bVerbose) cout << "esptool.py emulation mode" << endl;
        //!@todo emulate esptool
    }
    COMMAND nCommand = ParseCommandLine(argc, argv);
    //Handle commands that do not use serial port
    switch(nCommand)
    {
        case COMMAND::ELF2IMAGE:
            exit(Elf2Image("elf", "image")?0:-1);
            break;
        default:
            ; //carry on to open serial port
    }
        for(map<unsigned int,string>::iterator it = g_mFirmwareMap.begin(); it != g_mFirmwareMap.end(); ++it)
        {
            cout << "Write " << it->second << " to " << it->first << endl;
        }
    g_pEsp = new ESP8266(g_sPort, g_nBaud);
    g_pEsp->SetVerbose(g_bVerbose);
    if(g_pEsp->Open())
    {
        if(g_bVerbose) cout << "Opened serial port" << endl;
    }
    else
    {
        if(!g_bQuiet) cerr << "Failed to open serial port" << g_sPort << endl;
        return -1;
    }
    //Handle commands that use serial port
    switch(nCommand)
    {
    case COMMAND::RESET:
        g_pEsp->Reset();
        break;
    case COMMAND::TERMINAL:
        {
            Serial* pSerial = g_pEsp->GetSerial();
            while(pSerial->IsOpen())
            {
                unsigned char cData;
                if(pSerial->Read(&cData) > 0)
                {
                    cout << cData;
                }
                while(_kbhit())
                {
//                    cout << (char)getchar(); //!@todo Send to serial
                }
            }
        }
        break;
    case ERASE:
        break;
    case FLASH:
        for(map<unsigned int,string>::iterator it = g_mFirmwareMap.begin(); it != g_mFirmwareMap.end(); ++it)
        {
            if(g_bVerbose)
                cout << "Write " << it->second << " to " << it->first << endl;
            WriteFlash(it->first, it->second);
        }
        break;
    case RUN:
        break;
    case CHIP_ID:
        cout << g_pEsp->ReadId() << endl;
        break;
    case FLASH_ID:
        break;
    default:
        if(g_bVerbose) cout << "Unsupported command" << endl;
    }
    delete g_pEsp;
}

COMMAND ParseCommandLine(int nCount, char** pArgs)
{
    bool bMoreOptions = true;
    int nOptionIndex = 0;
    int nOffset = -1; //Holds firmware offset or -1 if aleady added to map
    COMMAND nCommand = COMMAND::NONE;
    static struct option options[] =
    {
        //{"long_name", "no_argument|required_argument|optional_argument", null, 'short_name'},
        {"version", no_argument, 0, 'v'},
        {"verbose", no_argument, 0, 'V'},
        {"quiet", no_argument, 0, 'q'},
        {"help", no_argument, 0, 'h'},
        {"port", required_argument, 0, 'p'},
        {"baud", required_argument, 0, 'b'},
        {"freq", required_argument, 0, 'f'},
        {"flash_mode", required_argument, 0, 'm'},
        {"flash_size", required_argument, 0, 's'},
        {0, 0, 0, 0} //terminate arguments
    };
    while(bMoreOptions)
    {
        switch(getopt_long(nCount, pArgs, "-b:p:f:m:s:hvVtq", options, &nOptionIndex))
        {
        case 'v':
            //show version
            ShowVersion();
            exit(0);
            break;
        case 'V':
            //set verbose mode
            g_bVerbose = true;
            g_bQuiet = false;
            break;
        case 'q':
            //quiet
            g_bQuiet = true;
            g_bVerbose = false;
        case 'h':
            //show help
            ShowVersion();
            ShowHelp(nCommand);
            exit(0);
            break;
        case 'b':
            //set baud rate
            {
                try
                {
                    g_nBaud = stoi(optarg);
                } catch(const std::exception& e)
                {
                    if(g_bVerbose) cerr << "Invalid baud rate: " << optarg << endl;
                    exit(-1);
                }
            }
            break;
        case 'p':
            //set serial port device
            g_sPort = optarg;
            break;
        case 'f':
            //cpu frequency
            if(nCommand == COMMAND::FLASH)
                try
                {
                    g_nCpu = stoi(optarg);
                }
                catch(const std::exception& e)
                {
                    if(g_bVerbose) cerr << "Invalid CPU frequency: " << optarg << endl;
                    exit(-1);
                }
            break;
        case 'm':
            //flash mode
            break;
        case 's':
            //flash size
            break;
        case 1:
        {
            //command line parameters
            string sArg = optarg;
            switch(nCommand)
            {
            case COMMAND::NONE:
                //This parameter should be a command
                if(sArg.compare("write_flash") == 0)
                    nCommand = COMMAND::FLASH;
                else if(sArg.compare("reset") == 0)
                    nCommand = COMMAND::RESET;
                else if(sArg.compare("erase") == 0)
                    nCommand = COMMAND::ERASE;
                else if(sArg.compare("run") == 0)
                    nCommand = COMMAND::RUN;
                else if(sArg.compare("chip_id") == 0)
                    nCommand = COMMAND::CHIP_ID;
                else if(sArg.compare("flash_id") == 0)
                    nCommand = COMMAND::FLASH_ID;
                else if(sArg.compare("terminal") == 0)
                    nCommand = COMMAND::TERMINAL;
                else if(sArg.compare("elf2image") == 0)
                    nCommand = COMMAND::ELF2IMAGE;
                break;
            case COMMAND::FLASH:
                if(nOffset == -1)
                {
                    string sOffset = optarg;
                    try
                    {
                        if(sOffset.compare(0, 2, "0x") == 0)
                            nOffset = stoi(sOffset.substr(2), 0, 16);
                        else
                            nOffset = stoi(sOffset);
                    }
                    catch(const std::exception& e)
                    {
                        if(!g_bQuiet)
                            cerr << "Invalid offset value '" << optarg <<"'." << endl << "Should be decimal, e.g. 1024 or hexadecimal, e.g. 0x0400" << endl;
                        exit(-1);
                    }
                }
                else
                {
                    g_mFirmwareMap[nOffset] = optarg;
                    nOffset = -1;
                }
                break;
            default:
                ;
            }
            break;
        }
        case -1:
            //no more options
            bMoreOptions = false;
            break;
        default:
            //invalid options
            exit(-1);
        }
    }
    //Validate parameter quantity
    switch(nCommand)
    {
    case COMMAND::FLASH:
        if(g_mFirmwareMap.size() < 1)
        {
            if(!g_bQuiet)
                cerr << "write_flash expects pairs of parameters" << endl;
            exit(-1);
        }
        break;
    case COMMAND::NONE:
        if(!g_bQuiet)
            cerr << "**No command provided**" << endl;
        ShowHelp();
        exit(-1);
        break;
    default:
        ;
    }
    return nCommand;
}

void ShowVersion()
{
    if(!g_bQuiet) cout << "riban ESP8266 tool version " <<
        AutoVersion::MAJOR << "." << AutoVersion::MINOR << "." << AutoVersion::BUILD
        << " (build " << AutoVersion::BUILDS_COUNT << " " <<
        AutoVersion::YEAR << "-" << AutoVersion::MONTH << "-" << AutoVersion::DATE << ")" << endl;
}

void ShowHelp(COMMAND nCommand)
{
    string sCommonSerialOptions = "\t-p, --port <PORT> \tSerial port device (default: " + g_sPort;
    sCommonSerialOptions += ")\n\t-b, --baud <BAUD> \tBaud rate (default: ";
    sCommonSerialOptions += to_string(g_nBaud) + ")";
    string sCommonOptions = "\t-V, --verbose \t\tIncrease verbosity of output\n\t-q, --quiet \t\tSuppress output";

    cout << endl << "usage: " << g_sAppName;
    switch(nCommand)
    {
        case COMMAND::NONE:
            cout << " [options]" << endl
            << endl  << "options:" << endl
            << "\t-h, --help \t\tShow help [for command]" << endl
            << "\t-v, --version \t\tProgram version" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl
            << "Commands:" << endl
    //            << "\tload_ram \t\t???" << endl
    //            << "\tdump_mem \t\t???" << endl
    //            << "\tread_mem \t\t???" << endl
    //        << "\twrite_mem \t\tWrite image to ESP8266 memory" << endl
            << "\treset \t\t\tHardware reset using RTS/DTR" << endl
            << "\twrite_flash \t\tWrite image to flash memory" << endl
            << "\trun \t\t\tStart ESP8266 program" << endl
    //            << "\timage_info ???" << endl
    //            << "\tmake_image ???" << endl
            << "\telf2image \t\tConvert elf to firmeare image" << endl
            << "\tread_mac \t\tRead MAC from ESP8266" << endl
            << "\tchip_id \t\tRead Chip ID frmo ESP8266"<< endl
            << "\tflash_id \t\tRead Flash ID from ESP8266"<< endl
            << "\tread_flash \t\tDownload flash image from ESP8266" << endl
    //            << "\tverify_flash \t\tVerify flash image in ESP8266" << endl
            << "\terase_flash \t\tErase flash memory" << endl;
            break;
        case COMMAND::FLASH:
            cout << " write_flash [options] <offset> <image> [<offset> <image>...]" << endl
            << endl << "Write firmware <image> to ESP8266 flash at <offset>. "
            << "Several <offset> <image> pairs may be provided to write multiple images." << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl
            << "\t-h, --help \t\tShow this help" << endl
            << "\t-f, --flash-freq \tSet CPU frequency (20m|26m|40m|80m default: 40m)" << endl
            << "\t-m, --flash-mode \tSet flash mode (qio|qout|dio|diout default: qio)" << endl
            << "\t-s, --flash-size \tSet flash mode (detect|2m|4m|8m|16m|32m|16m-c1|32m-c1|32m-c2 default: 4m)" << endl
            << "\t-p, --no-progress \tSuppress progress output" << endl
            << "\t-v, --verify \t\tVerify data after flash. (Should not be required because data is CRC checked during flash)" << endl;
            break;
        case COMMAND::RUN:
            cout << " run [options]" << endl
            << endl << "Put ESP8266 into run mode" << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl;
            break;
        case COMMAND::MAC:
            cout << " read_mac [options]" << endl << endl
            << "Read MAC from ESP8266" << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl;
            break;
        case COMMAND::CHIP_ID:
            cout << " chip_id [options]" << endl
            << endl << "Read Chip ID from ESP8266" << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl;
            break;
        case COMMAND::FLASH_ID:
            cout << " flash_id [options]" << endl
            << endl << "Read Flash ID from ESP8266" << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl;
            break;
        case COMMAND::READ_FLASH:
            cout << " read_flash [options] <flash_image>" << endl
            << endl << "Read image from ESP8266 flash to file <flash_image>" << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl;
            break;
        case COMMAND::ELF2IMAGE:
            cout << " elf2image <elf_image> <firmware_image>" << endl << endl
            << "Convert elf image to firmware image (does not open serial port)" << endl << endl
            << "options:" << endl
            << sCommonOptions << endl;
            break;
        case COMMAND::ERASE:
            cout << " erase" << endl
            << endl << "Erase ESP8266 flash memory" << endl << endl
            << "options:" << endl
            << sCommonOptions << endl;
            break;
        case COMMAND::TERMINAL:
            cout << " terminal" << endl
            << endl << "Start terminal emulator" << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl;
        case COMMAND::RESET:
            cout << " reset" << endl
            << endl << "Hardware reset using RTS / DTR" << endl << endl
            << "options:" << endl
            << sCommonSerialOptions << endl
            << sCommonOptions << endl;
    }
}

bool Elf2Image(string sElf, string sImage)
{
    bool bSuccess = false;
    //!@todo Implement Elf2Image
    if(g_bVerbose && bSuccess)
        cout << "Created firmware image " << sImage;
    if(!(g_bQuiet || bSuccess))
        cerr << "Failed to create firmware image " << sImage;
    return bSuccess;
}

bool WriteFlash(unsigned int nOffset, string sFilename)
{
    //!@todo implement write_flash
    return false;
}
