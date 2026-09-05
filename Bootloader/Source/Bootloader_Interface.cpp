/*******************************************************************
 *  FILE DESCRIPTION
-----------------------
 *  Author: Khaled El-Sayed @t0ti20
 *  File: Bootloader_Interface.cpp
 *  Date: March 28, 2024
 *  Description: Interface File Fot Bootloader Functionality
 *  Namespace: Bootloader
 *  (C) 2023 "@t0ti20". All rights reserved.
*******************************************************************/
/*****************************************
-----------     INCLUDES     -------------
*****************************************/
#include "Bootloader_Interface.hpp"
/*****************************************
----------    GLOBAL DATA     ------------
*****************************************/
constexpr const char Red[] = "\033[1;31m";
constexpr const char Green[] = "\033[1;32m";
constexpr const char Yellow[] = "\033[1;33m";
constexpr const char Blue[] = "\033[1;34m";
constexpr const char Default[] = "\033[0m";
/****************************************/
namespace Bootloader
{
/*****************************************
------------    CRC_Manage     -----------
*****************************************/
/****************************************************************************************************
* Function Name   : Append_CRC
* Class           : CRC_Manage
* Namespace       : Bootloader
* Description     : Appends CRC (Cyclic Redundancy Check) to the given data vector.
* Parameters (in) : Data - Reference to a vector of unsigned characters representing the data.
* Parameters (out): Data - Updated data vector with CRC appended.
* Return value    : None
* Notes           : - This function adds padding bits to the data until its size is a multiple of 4.
*                   - It calculates CRC for the data using CRC_Calculate function.
*                   - After calculation, it removes the padding bits and appends the CRC to the data.
*****************************************************************************************************/
void CRC_Manage::Append_CRC(std::vector<unsigned char>& Data)
{
    size_t Padding_Bits{};
    unsigned int Result{};
    unsigned char* Result_Byte = reinterpret_cast<unsigned char*>(&Result);
    /* Add padding bits until the size of Data is a multiple of 4 */
    while(Data.size() % 4 != 0) 
    {
        Data.push_back(0);
        Padding_Bits++;
    }
    /* Calculate CRC */
    Result = CRC_Calculate(Data);
    /* Remove padding bits */
    while(Padding_Bits--) 
    {
        Data.pop_back();
    }
    /* Append CRC to Data */
    Data.push_back(Result_Byte[0]);
    Data.push_back(Result_Byte[1]);
    Data.push_back(Result_Byte[2]);
    Data.push_back(Result_Byte[3]);
}

/****************************************************************************************************
* Function Name   : CRC_Calculate
* Class           : CRC_Manage
* Namespace       : Bootloader
* Description     : Calculates CRC (Cyclic Redundancy Check) for the given data.
* Parameters (in) : Data - Reference to a vector of unsigned characters representing the data.
* Parameters (out): None
* Return value    : Unsigned integer representing the calculated CRC value.
* Notes           : - This function uses CRC32 algorithm to calculate CRC.
*                   - It initializes CRC with CRC32_INIT value, and then iterates through each byte of the data,
*                     performing bitwise XOR operations and polynomial division.
*                   - Finally, it returns the calculated CRC value.
*****************************************************************************************************/
unsigned int CRC_Manage::CRC_Calculate(const std::vector<unsigned char> &Data)
{
    constexpr bool CRC32_REFIN=false;
    constexpr bool CRC32_REFOUT=false;
    constexpr unsigned int CRC32_INIT=0xFFFFFFFF;
    constexpr unsigned int CRC32_XOROUT=0x00000000;
    constexpr unsigned int CRC32_POLYNOMIAL=0x04C11DB7;
    unsigned int CRC{CRC32_INIT};
    unsigned int Result{};
    for (const unsigned char Byte : Data)
	{
        CRC ^= static_cast<unsigned int>(Byte)<<24;
        for (int Counter=0;Counter<8;++Counter) 
        {
            if (CRC&0x80000000){CRC=(CRC<<1)^CRC32_POLYNOMIAL;}
            else{CRC <<= 1;}
        }
    }
    if (!CRC32_REFOUT){Result=CRC^CRC32_XOROUT;}   
    else{Result=CRC;}
    return Result;
}

/*****************************************
-----------    Serial Port     -----------
*****************************************/
/****************************************************************************************************
* Constructor Name: Serial_Port
* Class           : Serial_Port
* Namespace       : Bootloader
* Description     : Initializes a serial port with the specified device location.
* Parameters (in) : Device_Location   - The location of the serial device.
* Parameters (out): None
* Return value    : None
* Notes           : - This constructor initializes the serial port with specified parameters such as baud rate,
*                     character size, stop bits, and parity.
*****************************************************************************************************/
Serial_Port::Serial_Port(const std::string& Device_Location)
:Port{Input_Output, Device_Location}
{
    /* Set baud rate */
    Port.set_option(boost::asio::serial_port_base::baud_rate(115200));
    /* Set character size */
    Port.set_option(boost::asio::serial_port_base::character_size(8));
    /* Set stop bits */
    Port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
    /* Set parity */
    Port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    /* Cancel any pending asynchronous operations */
    Port.cancel();
}

/****************************************************************************************************
* Function Name   : Send_Data
* Class           : Serial_Port
* Namespace       : Bootloader
* Description     : Writes data to the serial port after appending data length and CRC.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function inserts the data length at the beginning of the buffer and appends CRC to the data.
*                   - It then writes the data to the serial port using asynchronous write operation.
*                   - If ENABLE_DEBUG is defined, it prints debugging information about the sent frame.
*                   - Finally, it clears the data buffer.
*****************************************************************************************************/
void Serial_Port::Send_Data(void)
{
    /* Insert data length at the beginning of the buffer */
    Data_Buffer.insert(Data_Buffer.begin(), Data_Buffer.size() + 4);
    /* Append CRC to the data */
    Append_CRC(Data_Buffer);
    /* Write data to the serial port */
    boost::asio::write(Port, boost::asio::buffer(Data_Buffer));
    /* Debugging information */
    #ifdef ENABLE_DEBUG
        std::cout<<"[DEBUG] Sent Frame (Format="<<Data_Buffer.size()<<") : ";
        for(auto &Byte : Data_Buffer) { std::cout<<std::hex<<"0x"<<static_cast<int>(Byte)<<" "; }
        std::cout<<std::endl<<std::dec;
    #endif
    /* Clear the data buffer */
    Data_Buffer.clear();
}

/****************************************************************************************************
* Function Name   : Send_Data
* Class           : Serial_Port
* Namespace       : Bootloader
* Description     : Writes a single byte of data to the serial port.
* Parameters (in) : Data - The single byte of data to be written.
* Parameters (out): None
* Return value    : None
* Notes           : - This function writes a single byte of data to the serial port using asynchronous write operation.
*                   - If ENABLE_DEBUG is defined, it prints debugging information about the sent frame.
*****************************************************************************************************/
void Serial_Port::Send_Data(const unsigned char Data) 
{
    /* Write single byte data to the serial port */
    boost::asio::write(Port, boost::asio::buffer(&Data, 1));
    /* Debugging information */
    #ifdef ENABLE_DEBUG
        std::cout<<"[DEBUG] Sent Frame (Char) : ";
        std::cout<<std::hex<<"0x"<<static_cast<int>(Data)<<" ";
        std::cout<<std::endl<<std::dec;
    #endif
}

/****************************************************************************************************
* Function Name   : Receive_Data
* Class           : Serial_Port
* Namespace       : Bootloader
* Description     : Reads data from the serial port into the data buffer.
* Parameters (in) : Size - The size of data to be read from the serial port.
* Parameters (out): None
* Return value    : None
* Notes           : - This function resizes the data buffer to the specified size, reads data from the serial port,
*                     and then reverses the order of bytes in the buffer.
*                   - If ENABLE_DEBUG is defined, it prints debugging information about the received frame.
*****************************************************************************************************/
void Serial_Port::Receive_Data(size_t Size)
{
    /* Resize data buffer */
    Data_Buffer.resize(Size);
    /* Read data from the serial port */
    boost::asio::read(Port, boost::asio::buffer(Data_Buffer));
    /* Reverse the order of bytes */
    std::reverse(Data_Buffer.begin(), Data_Buffer.end());
    /* Debugging information */
    #ifdef ENABLE_DEBUG
        std::cout<<"[DEBUG] Received Frame (Format="<<Data_Buffer.size()<<") : ";
        for(auto &Byte : Data_Buffer) { std::cout<<std::hex<<"0x"<<static_cast<int>(Byte)<<" "; }
        std::cout<<std::endl<<std::dec;
    #endif
}

/****************************************************************************************************
* Function Name   : Receive_Data
* Class           : Serial_Port
* Namespace       : <Namespace>
* Description     : Receives data from the serial port.
* Parameters (in) : None
* Parameters (out): Data - Reference to the variable where received data will be stored.
* Return value    : bool - True if data is successfully received, false otherwise.
* Notes           : - This function performs an asynchronous read operation on the serial port.
*                   - It sets up a timer to handle timeouts.
*                   - If data is received before the timeout, it cancels the timer and returns true.
*                   - If a timeout occurs, it cancels the asynchronous operation and returns false.
*                   - Debugging information is printed if ENABLE_DEBUG is defined.
*****************************************************************************************************/
bool Serial_Port::Receive_Data(unsigned char &Data)
{
    bool Status{true};
    boost::asio::steady_timer Timer(Input_Output, boost::asio::chrono::seconds(1));
    /* Perform asynchronous read with handlar */
    boost::asio::async_read(Port, boost::asio::buffer(&Data, 1),
    [&](const boost::system::error_code& Error_Code, std::size_t Bytes_Transferred)
    {
        /* Cancel the timer if data is received */
        if (!Error_Code){Timer.cancel();}
    }
    );
    /* Set up timeout handler */
    Timer.async_wait([&](const boost::system::error_code& Error_Code) 
    {
        /* Cancel the asynchronous operation if timeout occurs */
        if (!Error_Code){Status=false;Port.cancel();}
    });
    /* Reset the IO service */
    Input_Output.reset();
    /* Run the IO service */
    Input_Output.run();
    /* Debugging information */
    #ifdef ENABLE_DEBUG
        std::cout << "[DEBUG] Received Frame (Char) : ";
        std::cout << std::hex << "0x" << static_cast<int>(Data) << " ";
        std::cout << std::endl << std::dec;
    #endif
    return Status;
}

/*****************************************
------------    Services     -------------
*****************************************/
/****************************************************************************************************
* Constructor Name: Services
* Class           : Services
* Description     : Initializes the Services object with the specified device location.
* Parameters (in) : Device_Location - Reference to the string containing the device location.
* Parameters (out): None
* Return value    : None
* Notes           : - This constructor initializes the Services object by calling the constructor of the base class (Serial_Port) with the specified device location.
*****************************************************************************************************/
Services::Services(const std::string &Device_Location)
:Serial_Port{Device_Location}{}

/****************************************************************************************************
* Function Name   : Send_Frame
* Class           : Services
* Namespace       : Bootloader
* Description     : Sends a frame with a specified service command and additional data to the controller.
* Parameters (in) : Service - The service command to be sent.
*                   Data    - Additional data to be sent along with the service command.
* Parameters (out): None
* Return value    : bool - True if the frame is sent successfully and acknowledged, false otherwise.
* Notes           : - This function prepares a frame with the specified service command and additional data
*                     and sends it to the controller.
*                   - It checks for acknowledgment after sending the frame. If acknowledgment is received,
*                     it returns true.
*                   - If no acknowledgment is received or an error occurs during transmission, it returns false.
*****************************************************************************************************/
bool Services::Send_Frame(Bootloader_Command_t Service, std::vector<unsigned char> &Data)
{
    /* Initialize Result as false */
    bool Result{}; 
    /* Clear Data_Buffer */
    Data_Buffer.clear(); 
    /* Push Service into Data_Buffer */
    Data_Buffer.push_back(Service); 
    /* Copy Data into Data_Buffer */
    std::copy(Data.begin(), Data.end(), std::back_inserter(Data_Buffer)); 
    /* Call Send_Data function */
    Send_Data(); 
    /* Check if acknowledgement is received */
    if (Get_Acknowledge()) 
    {
        /* Set Result to true */
        Result = true; 
    }
    /* Return Result */
    return Result; 
}

/****************************************************************************************************
* Function Name   : Read_File
* Class           : Services
* Namespace       : Bootloader
* Description     : Reads binary file content into a vector.
* Parameters (in) : None
* Parameters (out): Binary_File - Reference to a vector where the file content will be stored.
* Return value    : bool - True if the file is successfully read, false otherwise.
* Notes           : - This function reads the content of a binary file located at File_Location.
*                   - It determines the size of the file, allocates memory in Binary_File accordingly,
*                     and reads the file content into the buffer.
*                   - If successful, it returns true and Binary_File contains the file content.
*                   - If unsuccessful (e.g., file not found or unable to read), it returns false
*                     and Binary_File is cleared.
*****************************************************************************************************/
bool Services::Read_File(std::vector<unsigned char> &Binary_File,std::string &File_Location)
{
    /* Initialize Status as true */
    bool Status{true};
    /* Open the file */
    std::ifstream File(File_Location,std::ios::binary);
    /* Check if file is successfully opened */
    if (File.is_open())
    {
        /* Seek to the end of the file to determine its size */
        File.seekg(0, std::ios::end);
        std::streamsize Size = File.tellg();
        File.seekg(0, std::ios::beg);
        Binary_File.resize(Size);
        /* Read the file content into the buffer */
        if (!File.read(reinterpret_cast<char*>(Binary_File.data()),Size))
        {
            /* If failed to read, set Status to false and clear Binary_File */
            Status=false;
            Binary_File.clear();
        }
        /* Close the file */
        File.close();
    }
    else
    {
        /* If failed to open file, set Status to false */
        Status=false;
    }
    /* Return Status */
    return Status;
}

/****************************************************************************************************
* Function Name   : Send_Frame
* Class           : Services
* Namespace       : Bootloader
* Description     : Sends a frame with a specified service command to the controller.
* Parameters (in) : Service - The service command to be sent.
* Parameters (out): None
* Return value    : bool - True if the frame is sent successfully and acknowledged, false otherwise.
* Notes           : - This function prepares a frame with the specified service command and sends it to the controller.
*                   - It checks for acknowledgment after sending the frame. If acknowledgment is received,
*                     it updates the data buffer.
*                   - It returns true if the frame is sent successfully and acknowledged, otherwise returns false.
*****************************************************************************************************/
bool Services::Send_Frame(Bootloader_Command_t Service)
{
    /* Initialize Result as false */
    bool Result{};
    /* Clear Data_Buffer */
    Data_Buffer.clear();
    /* Push Service into Data_Buffer */
    Data_Buffer.push_back(Service);
    /* Call Send_Data function */
    Send_Data();
    /* Check if acknowledgement is received */
    if (Get_Acknowledge())
    {
        /* Update Data_Buffer */
        Update_Buffer();
        /* Set Result to true */
        Result=true;
    }
    /* Return Result */
    return Result;
}

/****************************************************************************************************
* Function Name   : Get_Help
* Class           : Services
* Namespace       : Bootloader
* Description     : Sends a get help command to the controller and prints available commands.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function sends a get help command to the controller.
*                   - It retrieves the list of available commands from the received data buffer and prints them.
*                   - It prints an error message if no acknowledgment is received after sending the get help command.
*****************************************************************************************************/
void Services::Get_Help(void)
{
    /* Check if Get_Help frame is sent successfully */
    if(Send_Frame(Bootloader_Command_Get_Help))
    {
        /* Iterate through Data_Buffer */
        for(auto &Command:Data_Buffer)
        {
            /* Print command details based on the received command */
            switch (Command)
            {
                case Bootloader_Command_Get_Help:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Get Help."<<std::endl;
                    break;
                case Bootloader_Command_Get_ID:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Get ID."<<std::endl;
                    break;
                case Bootloader_Command_Get_Version:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Get Version."<<std::endl;
                    break;
                case Bootloader_Command_Erase_Flash:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Erase Current Flash."<<std::endl;
                    break;
                case Bootloader_Command_Flash_Application:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Write On Flash."<<std::endl;
                    break;
                case Bootloader_Command_Address_Jump:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Jumb On Specific Address."<<std::endl;
                    break;               
                case Bootloader_Command_Say_Hi:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Say Hello To Chip."<<std::endl;
                    break;
                default:
                    std::cout<<Yellow<<" -> (0x"<<static_cast<int>(Command)<<")"<<Default<<" Unknown New Feature"<<std::endl;
                    break;
            }
        }
    }
    else
    {
        /* Print error message if no acknowledgment received */
        std::cout<<Red<<"X) There Is No ACK From Controller On Get Help Frame"<<std::endl;
    }
}

/****************************************************************************************************
* Function Name   : Get_Version
* Class           : Services
* Namespace       : <Namespace>
* Description     : Retrieves the version information from the controller.
* Parameters (in) : None
* Parameters (out): ID    - Reference to store the chip ID.
*                   Major - Reference to store the major version.
*                   Minor - Reference to store the minor version.
* Return value    : bool - True if the version information is successfully retrieved, false otherwise.
* Notes           : - This function sends a Get_Version frame to the controller and retrieves the version information.
*                   - If the frame is sent successfully and an acknowledgment is received, it assigns the chip ID, major
                      and minor versions to the provided references and returns true.
*                   - If no acknowledgment is received, it returns false.
*****************************************************************************************************/
bool Services::Get_Version(unsigned int &ID,unsigned int &Major,unsigned int &Minor)
{
    bool Status{};
    if(Send_Frame(Bootloader_Command_Get_Version))
    {
        ID=static_cast<int>(Data_Buffer[0]);
        Major=static_cast<int>(Data_Buffer[1]);
        Minor=static_cast<int>(Data_Buffer[2]);
        Status=true;
    }
    return Status;
}

/****************************************************************************************************
* Function Name   : Get_Version
* Class           : Services
* Namespace       : <Namespace>
* Description     : Retrieves the version information from the controller.
* Parameters (in) : None
* Parameters (out): None
* Return value    : void
* Notes           : - This function sends a Get_Version frame to the controller and retrieves the version information.
*                   - If the frame is sent successfully and an acknowledgment is received, it prints the chip ID, major, and minor versions.
*                   - If no acknowledgment is received, it prints an error message.
*****************************************************************************************************/
void Services::Get_Version(void)
{
    unsigned int ID{};
    unsigned int Major{};
    unsigned int Minor{};
    /* Check if Get_Version frame is sent successfully */
    if(Get_Version(ID,Major,Minor))
    {
        /* Print Chip ID, Major, and Minor version */
        std::cout<<Yellow<<" -> "<<"Chip ID : "<<Default<<ID<<std::endl;
        std::cout<<Yellow<<" -> "<<"Major : "<<Default<<Major<<std::endl;
        std::cout<<Yellow<<" -> "<<"Minor : "<<Default<<Minor<<std::endl;
    }
    else
    {
        /* Print error message if no acknowledgment received */
        std::cout<<Red<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx "<<Default<<"There Is No ACK From Controller On Get Version Frame"<<Red<<" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"<<std::endl;
    }
}

/****************************************************************************************************
* Function Name   : Send_Frame
* Class           : Services
* Namespace       : <Namespace>
* Description     : Sends the data frame to the controller.
* Parameters (in) : Data - Reference to the vector containing data to be sent.
* Parameters (out): None
* Return value    : bool - True if the frame is successfully sent and acknowledged, false otherwise.
* Notes           : - This function sends the data frame to the controller in chunks of maximum 250 bytes.
*                   - It waits for acknowledgment after sending each chunk.
*                   - If acknowledgment is not received, it returns false.
*****************************************************************************************************/
bool Services::Send_Frame(std::vector<unsigned char> &Data)
{
    /* Initialize Result as true */
    bool Result{true};
    /* Loop until Data is empty */
    while(Data.size())
    {
        Data_Buffer.clear();
        /* Copy data into Data_Buffer, maximum 250 bytes */
        std::copy(Data.begin(), Data.size() >= 250 ? Data.begin() + 250 : Data.end(), std::back_inserter(Data_Buffer));
        /* Call Send_Data function */
        Send_Data();
        /* Check if acknowledgment is not received */
        if(!Get_Acknowledge())
        {
            /* Set Result to false and break the loop */
            Result = false;
            break;
        }
        /* Erase sent data from Data */
        Data.erase(Data.begin(), Data.size() >= 250 ? Data.begin() + 250 : Data.end());
        /* Delay sending next frame */
        std::this_thread::sleep_for(std::chrono::milliseconds(Sending_Delay_MS)); 
    }
    /* Return Result */
    return Result;
}

/****************************************************************************************************
* Function Name   : Flash_Application
* Class           : Services
* Namespace       : <Namespace>
* Description     : Flashes the application onto the controller.
* Parameters (in) : Start_Page     - Reference to the starting page of the flash memory.
*                   File_Location  - Reference to the location of the file containing the application.
* Parameters (out): None
* Return value    : bool - True if the application is successfully flashed, false otherwise.
* Notes           : - This function reads the file containing the application into a vector.
*                   - It sends the flash application command along with the data bytes to the controller.
*                   - If the command is successfully sent, it sends the payload in chunks and returns true.
*                   - If any error occurs during the process, it returns false.
*****************************************************************************************************/
unsigned int Services::Get_Version(const std::string& Location)
{
    unsigned char ID{},Major{},Minor{};     
    std::istringstream iss(Location);
    std::string Number;
    size_t Counter{};
    while (std::getline(iss,Number,'.'))
    {
        Counter++;
        if (Counter == 2){ID = std::stoi(Number);}
        else if (Counter == 3){Major = std::stoi(Number);}
        else if (Counter == 4){Minor = std::stoi(Number);} 
    }
    return((Minor<<24)|(Major<<16)|(ID<<8));
}
unsigned int Services::Calculate_Application_CRC(void)
{
    std::vector<unsigned char> Data{};
    unsigned char Swap_Temp{};
    std::string File_Location{"/home/root/FOTA/Application/Build"};
    unsigned int CRC_Result{};
    size_t Appendded_Bytes{};
    if(Get_File(File_Location))
    {
        if(Read_File(Data,File_Location))
        {
            Appendded_Bytes=((Application_Size*1024)-Data.size());
            while(Appendded_Bytes--){Data.push_back(0xff);}
            /* Convert Endianness */
            for (size_t Counter{};Counter<Data.size();Counter+=4)
            {
                Swap_Temp=Data[Counter];
                Data[Counter]=Data[Counter+3];
                Data[Counter+3]=Swap_Temp;
                Swap_Temp=Data[Counter+1];
                Data[Counter+1]=Data[Counter+2];
                Data[Counter+2]=Swap_Temp;
            }
            CRC_Result=CRC_Calculate(Data);
        }
    }
    return CRC_Result;
}
bool Services::Set_Application_Information(std::string &File_Location)
{
    bool Status{};
    unsigned char ID{},Major{},Minor{};
    unsigned int Version{Get_Version(File_Location)};
    unsigned int Total_Pages{1};
    unsigned int Start_Page{Application_Location-1};
    unsigned int Boot_Version_Address{Version_Location};
    unsigned int Application_CRC_Address{CRC_Location};
    Status=Erase_Flash(Start_Page,Total_Pages);
    if(Status)
    {
        /* Set Version */
        Status=Write_Data(Application_CRC_Address,Calculate_Application_CRC());
        /* Set Application CRC */
        if(Status){Status=Write_Data(Boot_Version_Address,Version);}
    }
    return Status;
}
bool Services::Flash_Application(unsigned int &Start_Page, std::string &File_Location)
{
    std::vector<unsigned char> Payload{};
    bool Status{Read_File(Payload,File_Location)};
    if(Status)
    {
        /* Prepare data bytes */
        std::vector<unsigned char> Data_Bytes{static_cast<unsigned char>(Start_Page), static_cast<unsigned char>((Payload.size() / 250) + 1)};
        /* Send flash application command */
        Status=Send_Frame(Bootloader_Command_Flash_Application,Data_Bytes);
        if(Status)
        {
            Status=Send_Frame(Payload);
            /* Send payload */
            if(Status)
            {
                Status=Set_Application_Information(File_Location);
            }
        }
    }
    return Status;
}

/****************************************************************************************************
* Function Name   : Flash_Application
* Class           : Services
* Namespace       : <Namespace>
* Description     : Flashes the application onto the controller.
* Parameters (in) : None
* Parameters (out): None
* Return value    : void
* Notes           : - This function prompts the user to enter the start page and file location.
*                   - If the user wants to edit the file location, it prompts again for the new location.
*                   - It displays a loading animation while flashing the application.
*                   - After flashing, it prints a success message and resets the MCU to start the application.
*                   - If any error occurs during the process, it prints an error message.
*****************************************************************************************************/
bool Services::Get_File(std::string &Location)
{
    bool Status{};
    std::regex Pattern(".*\\.bin");
    std::ifstream File(Location);
    if(Location.find(".bin")==std::string::npos)
    {
        
        for (const auto& Current_File : std::filesystem::directory_iterator(Location))
        {
            if (std::filesystem::is_regular_file(Current_File) && std::regex_match(Current_File.path().filename().string(), Pattern))
            {
                Location=Current_File.path();
                Status=true;
                break;
            }
        }
    }
    else if (File.is_open())
    {
        Status=true;
        /* Close the file */
        std::cout<<"=====";
        File.close();
    }
    return Status;
}
void Services::Flash_Application(void)
{
    std::string File_Location{"/home/root/FOTA/Application/Build"};
    bool Animation_Running{true};
    auto Loading = [&Animation_Running]() 
    {
        const char Animation[] = {'|', '/', '-', '\\'};
        int Counter {};
        while (Animation_Running)
        {
            std::cout<<Yellow<<"\r"<<" -> "<<Green<<Animation[Counter]<<Default<<" Loading "<<Green<<Animation[Counter]<<std::flush;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            Counter=(Counter+1)%4;
        }
        std::cout<<std::endl;
    };
    unsigned int Start_Page{};
    std::string Input{};
    /* Prompt user to enter start page */
    std::cout<<Yellow<<" -> "<<Default<<"Please Enter Start Page : ";
    std::cin>>Start_Page;
    /* Prompt user to edit file location */
    std::cout<<Yellow<<" -> "<<Default<<"Binary File Or Directory Location ["<<File_Location<<"] Want To Edit [N/y] : ";
    std::cin.ignore();
    std::getline(std::cin, Input);
    /* If user wants to edit file location */
    if (!Input.empty() && (Input == "y" || Input == "Y"))
    {
        std::cout<<Yellow<<" -> "<<Default<<"Please Enter File Location : ";
        std::cin>>File_Location;
    }
    if(Get_File(File_Location))
    {
        std::cout<<Yellow<<" -> "<<Default<<"Flashing Application ["<<File_Location<<"]\n";
        std::thread Animation_Thread(Loading);
        /* Try Flasing Application */
        if(Flash_Application(Start_Page, File_Location))
        {
            Animation_Running=false;
            Animation_Thread.join();                                       
            std::cout<<Green<<"================================ "<<Default<<"Done Flashing Application"<<Green<<" ===============================\n";
        }
        else
        {
            Animation_Running=false;
            Animation_Thread.join();
            std::cout<<Red<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx "<<Default<<"Error In Sending Frames"<<Red<<" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n";
        }
    }
    else
    {
        std::cout<<Red<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx "<<Default<<"Can't Find Binary File"<<Red<<" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\n";
    }
}

/****************************************************************************************************
* Function Name   : Erase_Flash
* Class           : Services
* Namespace       : Bootloader
* Description     : Sends an erase flash command to the controller with the specified start page and
*                   number of pages to erase.
* Parameters (in) : Start_Page   - The start page for erasing.
*                   Pages_Count  - The number of pages to erase.
* Parameters (out): None
* Return value    : None
* Notes           : - This function prepares the data bytes containing the start page and number of pages
*                     to erase and sends an erase flash command to the controller.
*                   - It prints a message if erasing is done successfully, otherwise, it prints an error message
*                     if no acknowledgment is received after sending the erase pages command.
*****************************************************************************************************/
bool Services::Erase_Flash(unsigned int &Start_Page, unsigned int &Pages_Count)
{
    bool State{};
    /* Prepare data bytes */
    std::vector<unsigned char> Data_Bytes{static_cast<unsigned char>(Start_Page), static_cast<unsigned char>(Pages_Count)};
    /* Send erase flash command */
    if(Send_Frame(Bootloader_Command_Erase_Flash, Data_Bytes)){State=true;}
    return State;
}

/****************************************************************************************************
* Function Name   : Erase_Flash
* Class           : Services
* Namespace       : Bootloader
* Description     : Prompts the user to enter the start page and the total number of pages to erase,
*                   and then initiates flash erasing.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function prompts the user to enter the start page and the total number of pages
*                     to erase for flashing.
*                   - It then initiates the flash erasing process with the specified start page and number of pages.
*****************************************************************************************************/
void Services::Erase_Flash(void)
{
    unsigned int Start_Page{};
    unsigned int Pages_Count{};
    /* Prompt user to enter start page */
    std::cout<<Yellow<<" -> "<<Default<<"Please Enter Start Page : ";
    std::cin>>Start_Page;
    /* Prompt user to enter total number of pages to erase */
    std::cout<<Yellow<<" -> "<<Default<<"Please Enter Total Number Of Pages To Erase : ";
    std::cin>>Pages_Count;
    /* Erase flash */
    if(Erase_Flash(Start_Page, Pages_Count))
    {
        /* Print message if erasing is done successfully */
        std::cout<<Green<<"=============================== "<<Default<<"Erasing Done Successfully"<<Green<<" ================================"<<std::endl;
    }
    else
    {
        /* Print error message if no acknowledgment received after sending erase pages */
        std::cout<<Red<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx "<<Default<<"There Is No ACK After Sending Eraseing Pages"<<Red<<" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"<<std::endl;
    }
}

/****************************************************************************************************
* Function Name   : Jump_Address
* Class           : Services
* Namespace       : Bootloader
* Description     : Sends a jump address command to the controller with the specified address.
* Parameters (in) : Address - The address to jump to.
* Parameters (out): None
* Return value    : None
* Notes           : - This function prepares the address bytes and sends a jump address command to the controller
*                     with the specified address.
*                   - It prints a message if jumping is done successfully, otherwise, it prints an error message
*                     if no acknowledgment is received after sending the jump address.
*****************************************************************************************************/
bool Services::Jump_Address(unsigned int &Address)
{
    bool Status{};
    /* Prepare address bytes */
    std::vector<unsigned char> Address_Bytes(sizeof(Address));
    std::memcpy(Address_Bytes.data(), &Address, sizeof(Address));
    /* Send jump address command */
    if(Send_Frame(Bootloader_Command_Address_Jump, Address_Bytes)){Status=true;}
    return Status;
}

/****************************************************************************************************
* Function Name   : Jump_Address
* Class           : Services
* Namespace       : Bootloader
* Description     : Prompts the user to enter an address in hexadecimal and jumps to that address.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function prompts the user to enter an address in hexadecimal format.
*                   - It then jumps to the entered address.
*****************************************************************************************************/
void Services::Jump_Address(void)
{
    unsigned int Address{};
    /* Prompt user to enter address in hexadecimal */
    std::cout<<Yellow<<" -> "<<Default<<"Please Enter Address In HEX : 0x";
    std::cin>>std::hex>>Address>>std::dec;
    /* Jump to address */
    if(Jump_Address(Address))
    {
        /* Print message if jumping is done successfully */
        std::cout<<Green<<"================================ "<<Default<<"Jumbing Done Successfully"<<Green<<" ==============================="<<std::endl;
    }
    else
    {
        /* Print error message if no acknowledgment received after sending jump address */
        std::cout<<Red<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx "<<Default<<"There Is No ACK After Sending Jump Address"<<Red<<" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"<<std::endl;
    }
}

/****************************************************************************************************
* Function Name   : Get_ID
* Class           : Services
* Namespace       : <Namespace>
* Description     : Retrieves the built-in ID from the controller.
* Parameters (in) : None
* Parameters (out): Built_ID - Reference to a vector to store the built-in ID.
* Return value    : bool - True if the built-in ID is successfully retrieved, false otherwise.
* Notes           : - This function sends the get ID command to the controller.
*                   - If the command is successful and the data is received, it stores the built-in ID in the provided vector.
*****************************************************************************************************/
bool Services::Get_ID(std::vector<unsigned int> &Built_ID)
{
    bool Status{};
    if(Send_Frame(Bootloader_Command_Get_ID))
    {
        /* Store built-in ID */
        Built_ID.push_back((reinterpret_cast<unsigned int*>(Data_Buffer.data()))[0]);
        Built_ID.push_back((reinterpret_cast<unsigned int*>(Data_Buffer.data()))[1]);
        Built_ID.push_back((reinterpret_cast<unsigned int*>(Data_Buffer.data()))[2]);
        Status=true;
    }
    return Status;
}

/****************************************************************************************************
* Function Name   : Get_ID
* Class           : Services
* Namespace       : <Namespace>
* Description     : Retrieves the built-in ID from the controller and prints it.
* Parameters (in) : None
* Parameters (out): None
* Return value    : void
* Notes           : - This function sends the get ID command to the controller and retrieves the built-in ID.
*                   - If the command is successful and the data is received, it prints the built-in ID.
*                   - If no acknowledgment is received, it prints an error message.
*****************************************************************************************************/
void Services::Get_ID(void)
{
    std::vector<unsigned int> ID{};
    /* Check if Get_ID frame is sent successfully */
    if(Get_ID(ID))
    {
        /* Print built-in ID */
        std::cout<<Yellow<<" -> Built In ID :"<<Default<<" 0x"<<std::hex<<ID[0]<<" 0x"<<ID[1]<<" 0x"<<ID[2]<<std::endl<<std::dec;
    }
    else
    {
        /* Print error message if no acknowledgment received */
        std::cout<<"There Is No ACK From Controller On Get ID Frame"<<std::endl;
    }
}

/****************************************************************************************************
* Function Name   : Get_Acknowledge
* Class           : Services
* Namespace       : <Namespace>
* Description     : Receives acknowledgment from the controller.
* Parameters (in) : None
* Parameters (out): None
* Return value    : bool - True if acknowledgment (ACK) is received, false otherwise.
* Notes           : - This function receives data from the controller and checks if it matches the acknowledgment (ACK) state.
*****************************************************************************************************/
bool Services::Get_Acknowledge(void)
{
    unsigned char Return{};
    Receive_Data(Return);
    /* Check if received data is ACK */
    return  Return == Bootloader_State_ACK ? true : false;
}

/****************************************************************************************************
* Function Name   : Update_Buffer
* Class           : Services
* Namespace       : Bootloader
* Description     : Updates the buffer by reading data size and then reading data based on the size.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function reads a byte of data to determine the size of the incoming data.
*                   - It then reads data from the serial port based on the determined size and updates the buffer.
*****************************************************************************************************/
void Services::Update_Buffer(void)
{
    unsigned char Data_Size{};
    /* Read data size */
    if(Receive_Data(Data_Size))
    {
        /* Read data based on size */
        Receive_Data(static_cast<size_t>(Data_Size));
    }
}

/****************************************************************************************************
* Function Name   : Exit_Bootloader
* Class           : Services
* Namespace       : <Namespace>
* Description     : Exits the bootloader mode.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function sends a "Say Bye" command to the controller to exit the bootloader
*                     mode; the target resets itself in software (SCB->AIRCR) on receipt, so no
*                     hardware reset-pin pulse is needed here.
*****************************************************************************************************/
void Services::Exit_Bootloader(void)
{
    Say_Bye();
}

/****************************************************************************************************
* Function Name   : Write_Data
* Class           : Services
* Namespace       : <Namespace>
* Description     : Writes data to the specified address.
* Parameters (in) : None
* Parameters (out): None
* Return value    : void
* Notes           : - This function prompts the user to enter the desired address and data to write in hexadecimal format.
*                   - It then sends the write data command to the controller.
*                   - If the command is successful, it prints a success message. Otherwise, it prints an error message.
*****************************************************************************************************/
void Services::Write_Data(void)
{
    unsigned int Address{};
    unsigned int Data{};
    /* Prompt user to enter start page */
    std::cout<<Yellow<<" -> "<<Default<<"Please Enter Desired Address In HEX : 0x";
    std::cin>>std::hex>>Address>>std::dec;
    /* Prompt user to enter total number of pages to erase */
    std::cout<<Yellow<<" -> "<<Default<<"Please Enter Data To Write In HEX : 0x";
    std::cin>>std::hex>>Data>>std::dec;
    /* Erase flash */
    if(Write_Data(Address, Data))
    {
        /* Print message if erasing is done successfully */                        
        std::cout<<Green<<"================================ "<<Default<<"Writing Done Successfully"<<Green<<" ==============================="<<std::endl;
    }
    else
    {
        /* Print error message if no acknowledgment received after sending erase pages */
        std::cout<<Red<<"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx "<<Default<<"There Is No ACK After Sending Writing Data"<<Red<<" xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"<<std::endl;
    }
}

/****************************************************************************************************
* Function Name   : Write_Data
* Class           : Services
* Namespace       : <Namespace>
* Description     : Writes data to the specified address.
* Parameters (in) : Address - Reference to the address where data will be written.
*                   Data    - Reference to the data to be written.
* Parameters (out): None
* Return value    : bool - True if data is successfully written, false otherwise.
* Notes           : - This function constructs the data bytes to be sent, including the address and data.
*                   - It then sends the write data command along with the data bytes to the controller.
*****************************************************************************************************/
bool Services::Write_Data(unsigned int &Address,const unsigned int &Data)
{
    bool Status{};
    std::vector<unsigned char> Sending_Bytes(sizeof(Address) + sizeof(Data));
    std::memcpy(Sending_Bytes.data(), &Address, sizeof(Address));
    std::memcpy(Sending_Bytes.data() + sizeof(Address), &Data, sizeof(Data));
    if(Send_Frame(Bootloader_Command_Send_Data, Sending_Bytes)){Status=true;}
    return Status;
}

/****************************************************************************************************
* Function Name   : Say_Hi
* Class           : Services
* Namespace       : <Namespace>
* Description     : Sends a "Say Hi" command to the controller.
* Parameters (in) : None
* Parameters (out): None
* Return value    : bool - True if the "Say Hi" command is successfully sent, false otherwise.
* Notes           : - This function sends the "Say Hi" command to the controller.
*****************************************************************************************************/
bool Services::Say_Hi(void)
{
    return Send_Frame(Bootloader_Command_Say_Hi);
}

/****************************************************************************************************
* Function Name   : Say_Bye
* Class           : Services
* Namespace       : <Namespace>
* Description     : Sends a "Say Bye" command to the controller.
* Parameters (in) : None
* Parameters (out): None
* Return value    : bool - True if the "Say Bye" command is successfully sent, false otherwise.
* Notes           : - This function sends the "Say Bye" command to the controller.
*****************************************************************************************************/
bool Services::Say_Bye(void)
{
    return Send_Frame(Bootloader_Command_Say_Bye);
}

/****************************************************************************************************
* Function Name   : Start_Target_Bootloader
* Class           : Services
* Namespace       : <Namespace>
* Description     : Starts the target bootloader by sending a "Say Hi" command repeatedly.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function repeatedly sends a "Say Hi" command to the controller until it receives an acknowledgment.
*****************************************************************************************************/
void Services::Start_Target_Bootloader(void)
{
    bool Status{true};
    while(Status)
    {
        Status=!Say_Hi();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

/*****************************************
-------------    Monitor     -------------
*****************************************/
/****************************************************************************************************
* Constructor Name: Monitor
* Class           : Monitor
* Description     : Initializes the Monitor object with the specified services, repository path, binary location, and arguments.
* Parameters (in) : User_Interface   - Reference to the Services object for user interface.
*                   Repository_Path  - Reference to the string containing the repository path.
*                   Binary_Location - Reference to the string containing the binary location.
*                   Arguments        - Reference to the vector containing command-line arguments.
* Parameters (out): None
* Return value    : None
* Notes           : - This constructor initializes the Monitor object with the specified services, repository path, binary location, and arguments.
*****************************************************************************************************/
Monitor::Monitor(Services &User_Interface,const std::string &Repository_Path,const std::string &Binary_Location,std::vector<std::string> &Arguments)
:Interface{User_Interface},Binary_Repository{Repository_Path},Directory_Location{Binary_Location},Commands{Arguments}{}

/****************************************************************************************************
* Function Name   : Build_Directory
* Class           : Monitor
* Description     : Verifies the local firmware directory exists and records the modification
*                   time of whatever .bin file (if any) is currently in it, as a baseline for
*                   Update_Available.
* Parameters (in) : None
* Parameters (out): None
* Return value    : bool - True if the directory exists, false otherwise.
*****************************************************************************************************/
bool Monitor::Build_Directory(void)
{
    bool Status{};
    std::error_code Error_Code{};
    if(std::filesystem::is_directory(Binary_Repository,Error_Code))
    {
        Status=true;
        /* Record whatever is already there as the baseline, so a pre-existing file doesn't
         * immediately look like a fresh update on the very first check. */
        std::string Current_File{Directory_Location};
        if(Interface.Get_File(Current_File))
        {
            Last_Write_Time=std::filesystem::last_write_time(Current_File,Error_Code);
        }
    }
    else
    {
        std::cerr << "Error: Firmware directory [" << Binary_Repository << "] does not exist." << std::endl;
    }
    return Status;
}

/****************************************************************************************************
* Function Name   : Update_Available
* Class           : Monitor
* Description     : Checks whether the .bin file in the local firmware directory has a newer
*                   modification time than the last one seen.
* Parameters (in) : None
* Parameters (out): None
* Return value    : bool - True if the file is new/changed since the last check, false otherwise.
*****************************************************************************************************/
bool Monitor::Update_Available(void)
{
    bool Status{};
    std::string Current_File{Directory_Location};
    /* Re-resolve the current *.bin match every poll, in case the file name itself changed */
    if(Interface.Get_File(Current_File))
    {
        std::error_code Error_Code{};
        std::filesystem::file_time_type Modified_Time{std::filesystem::last_write_time(Current_File,Error_Code)};
        if(!Error_Code && Modified_Time!=Last_Write_Time)
        {
            Last_Write_Time=Modified_Time;
            Status=true;
        }
    }
    return Status;
}

/****************************************************************************************************
* Function Name   : Wait_For_Update
* Class           : Monitor
* Description     : Polls the local firmware directory until a new/changed .bin file appears.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function first tries to build the directory by calling Build_Directory.
*                   - It then polls Update_Available every Get_Update_Time_Seconds until it reports
*                     a change - the new binary is already locally present at that point, no
*                     download step needed.
*****************************************************************************************************/
void Monitor::Wait_For_Update(void)
{
    if(Build_Directory())
    {
        std::cout << "Start Monitoring For Any Updates\n";
        std::cout << "Waiting For An Update\n";
        while(true)
        {
            /* Check If There Is Update */
            if(Update_Available())
            {
                std::cout << "There Is An Update" << std::endl;
                break;
            }
            else
            {
                /* Sleep for some time before checking again */
                std::this_thread::sleep_for(std::chrono::seconds(Get_Update_Time_Seconds));
            }
        }
    }
    else
    {
        std::cerr << "Error: Cannot find the local firmware directory." << std::endl;
    }
}

/****************************************************************************************************
* Function Name   : Update_Application
* Class           : Monitor
* Description     : Updates the application by downloading the update and flashing it.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function waits for updates, then flashes the application with the downloaded update.
*                   - It starts the target bootloader, waits for reset, and then flashes the application.
*****************************************************************************************************/
void Monitor::Update_Application(void)
{
    unsigned int Location{Application_Location};
    std::string File_Location{Directory_Location};
    Wait_For_Update();
    std::cout<<"Update Will Be Flashed Next Reset"<<std::endl;
    std::cout<<"Waiting For Reset"<<std::endl;
    Interface.Start_Target_Bootloader();
    std::cout<<"Bootloader Started On Target"<<std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(Sending_Delay_MS)); 
    if(Interface.Say_Hi())
    {
        if(Interface.Get_File(File_Location))
        {
            std::cout<<"Start Flashing Application : "<<File_Location<<std::endl;
            if(Interface.Flash_Application(Location,File_Location))
            {
                Interface.Exit_Bootloader();
                std::cout << "Application Flashed Successfully\n";
            }
        }
        else
        {
            std::cout << "Cant Find Binary File in ["<<File_Location<<"}\n";
        }
    }
    else
    {
        std::cout << "Application Downloadded But MCU Not Connected\n";
    }
}

/****************************************************************************************************
* Function Name   : Start_Monitoring
* Class           : Monitor
* Description     : Starts the monitoring process based on the provided command-line arguments.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function checks the command-line arguments and starts the update service if required.
*****************************************************************************************************/
void Monitor::Start_Monitoring(void)
{
    /* Force line buffering for stdout */
    setvbuf(stdout, NULL, _IOLBF, BUFSIZ);
    /* Check For Arguments */
    for (const auto& Option : Commands)
    {
        /* Start Update Service */
        if (Option == "-r")
        {
            while(true)
            {
                Update_Application();
            }
        }
        else
        {
            std::cout << "Unknown option or parameter: " << Option << std::endl;
        }
    }
}

/*****************************************
---------    User Interface     ----------
*****************************************/
/****************************************************************************************************
* Constructor Name: User_Interface
* Class           : User_Interface
* Description     : Initializes the User_Interface object with the specified parameters.
* Parameters (in) : User_Interface_File - Reference to the string containing the user interface file.
*                   Repository_Path     - Reference to the string containing the repository path.
*                   Binary_Location     - Reference to the string containing the binary location.
*                   Arguments           - Reference to the vector containing command-line arguments.
* Parameters (out): None
* Return value    : None
* Notes           : - This constructor initializes the User_Interface object with the specified parameters.
*                   - It initializes the Services and Monitor objects.
*****************************************************************************************************/
User_Interface::User_Interface(const std::string &User_Interface_File,const std::string &Repository_Path,const std::string &Binary_Location,std::vector<std::string> &Arguments)
:Services{User_Interface_File},
Monitor{(*this),Repository_Path,Binary_Location,Arguments}{}

/****************************************************************************************************
* Destructor Name : ~User_Interface
* Class           : User_Interface
* Description     : Destructor for the User_Interface class.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This destructor ensures proper cleanup when the User_Interface object is destroyed.
*                   - It exits the bootloader if connected and resets the console color.
*****************************************************************************************************/
User_Interface::~User_Interface()
{
    if(Say_Hi())
    {
        Exit_Bootloader();
    }
    std::cout<<Default;
}

/****************************************************************************************************
* Function Name   : Print_Target_Info
* Class           : User_Interface
* Description     : Prints information about the target device.
* Parameters (in) : State - Boolean indicating whether the target device is detected or not.
* Parameters (out): None
* Return value    : None
* Notes           : - This function retrieves information about the target device and prints it.
*****************************************************************************************************/
void User_Interface::Print_Target_Info(bool State)
{
    unsigned int ID{},Major{},Minor{};
    std::vector<unsigned int> Built_ID{};
    if(State)
    {
        Get_Version(ID,Major,Minor);
        Get_ID(Built_ID);
        std::cout<<Green;
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cout<<"+++++++++++++++++++++++++++++++++++++ Target Detected ++++++++++++++++++++++++++++++++++++\n";
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        //std::cout<<Yellow<<" -> Built In ID : "<<Default<<"0x"<<std::hex<<Built_ID[0]<<" 0x"<<Built_ID[1]<<" 0x"<<Built_ID[2]<<std::endl<<std::dec;
        std::cout<<Yellow<<" -> Chip Number : "<<Default<<ID<<std::endl;
        std::cout<<Yellow<<" -> Application Version : "<<Default<<Major<<"."<<Minor<<std::endl;
        std::cout<<Green;
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
    }
    else
    {
        std::cout<<Red;
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cout<<"++++++++++++++++++++++++++++++++++ Failed To Find Target +++++++++++++++++++++++++++++++++\n";
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cout<<Default;
    }
}

/****************************************************************************************************
* Function Name   : Start_Application
* Class           : User_Interface
* Description     : Starts the application interface for user interaction.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function displays a menu for various options and handles user input accordingly.
*****************************************************************************************************/
void User_Interface::Start_Application(void)
{
    bool Flag{true};
    std::string Chosen_Option{};
    /* Initialize Default Location For Serial And GPIO */
    if(!Say_Hi())
    {
        std::cout<<Green;
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cout<<Yellow<<" -> "<<Default<<"Trying To Reset Target To Start Bootloader ...\n"<<Green;
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        Start_Target_Bootloader();
        if(system("clear")){};
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(Sending_Delay_MS)); 
    while(Flag)
    {
        {
            std::cout<<Green;
            std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n+";
            std::cout<<"                                                                                        +""\n+"<<Default;
            std::cout<<"    ██████╗░░█████╗░░█████╗░████████╗██╗░░░░░░█████╗░░█████╗░██████╗░███████╗██████╗░   "<<Green<<"+\n+"<<Default;
            std::cout<<"    ██╔══██╗██╔══██╗██╔══██╗╚══██╔══╝██║░░░░░██╔══██╗██╔══██╗██╔══██╗██╔════╝██╔══██╗   "<<Green<<"+\n+"<<Default;
            std::cout<<"    ██████╦╝██║░░██║██║░░██║░░░██║░░░██║░░░░░██║░░██║███████║██║░░██║█████╗░░██████╔╝   "<<Green<<"+\n+"<<Default;
            std::cout<<"    ██╔══██╗██║░░██║██║░░██║░░░██║░░░██║░░░░░██║░░██║██╔══██║██║░░██║██╔══╝░░██╔══██╗   "<<Green<<"+\n+"<<Default;
            std::cout<<"    ██████╦╝╚█████╔╝╚█████╔╝░░░██║░░░███████╗╚█████╔╝██║░░██║██████╔╝███████╗██║░░██║   "<<Green<<"+\n+"<<Default;
            std::cout<<"    ╚═════╝░░╚════╝░░╚════╝░░░░╚═╝░░░╚══════╝░╚════╝░╚═╝░░╚═╝╚═════╝░╚══════╝╚═╝░░╚═╝   "<<Green<<"+\n+"<<Default;
            std::cout<<"                            🅚 🅗 🅐 🅛 🅔 🅓  _  🅔 🅛 - 🅢 🅐 🅨 🅔 🅓                             "<<Green<<"+\n+";
            std::cout<<"                                                                                        +""\n"<<Default;
        }
        Print_Target_Info(Say_Hi());
        std::cout<<Blue;
        std::cout<<"Options:"<<std::endl;
        std::cout<<Yellow<<"  1. "<<Default<<"Get ID."<<std::endl;
        std::cout<<Yellow<<"  2. "<<Default<<"Get Help."<<std::endl;
        std::cout<<Yellow<<"  3. "<<Default<<"Write Data." <<std::endl;
        std::cout<<Yellow<<"  4. "<<Default<<"Get Version."<<std::endl;
        std::cout<<Yellow<<"  5. "<<Default<<"Erase Flash."<<std::endl;
        std::cout<<Yellow<<"  6. "<<Default<<"Jump Address."<<std::endl;
        std::cout<<Yellow<<"  7. "<<Default<<"Flash Application."<<std::endl;
        std::cout<<Yellow<<"  9. "<<Default<<"Exit."<<std::endl;
        std::cout<<Blue<<"Enter Option : "<<Yellow;
        /* Scan The Input By User */
        std::cin>>Chosen_Option;
        std::cout<<Green;
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        /* Jump For The Option */
        switch (Chosen_Option[0])
        {
            case '4':Get_Version();break;
            case '2':Get_Help();break;
            case '3':Write_Data();break;
            case '1':Get_ID();break;
            case '6':Jump_Address();break;
            case '5':Erase_Flash();break;
            case '7':Flash_Application();break;
            case '9':Flag=false;break;
        }
        if(!Flag){if(system("clear")){};break;}
        std::cout<<Green;
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cout<<"++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
        std::cin.ignore();
        std::getline(std::cin, Chosen_Option);
        if(system("clear")){};
    }
}

}
/********************************************************************
 *  END OF FILE:  Bootloader_Interface.cpp 
********************************************************************/