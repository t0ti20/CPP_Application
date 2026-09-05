/*******************************************************************
 *  FILE DESCRIPTION
-----------------------
*  Author: Khaled El-Sayed @t0ti20
*  File: Bootloader_Host.h
*  Date: March 28, 2024
*  Description: Bootloader Host For STM32
*  Namespace : Bootloader
*  (C) 2024 "@t0ti20". All rights reserved.
*******************************************************************/
#ifndef _BOOTLOADER_INTERFACE_H_
#define _BOOTLOADER_INTERFACE_H_
/******************************************************************/
/*****************************************
------------    Includes     -------------
*****************************************/
#include <regex>
#include <vector>
#include <thread>
#include <chrono>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include <boost/asio.hpp>
/*****************************************
---------    Configurations     ----------
*****************************************/
//#define ENABLE_DEBUG                            (4)
constexpr unsigned char Bootloader_State_ACK    {1};
constexpr unsigned char Bootloader_State_NACK   {2};
constexpr unsigned int Sending_Delay_MS         {200};
constexpr unsigned int Get_Update_Time_Seconds  {5};
constexpr unsigned int Application_Location     {32};
constexpr unsigned int Application_Size         {32};
constexpr unsigned int Version_Location         {0x8007FFC};
constexpr unsigned int CRC_Location             {0x8007FE0};
/*****************************************
-----------    Bootloader     ------------
*****************************************/
namespace Bootloader
{
/*****************************************
------------    CRC_Manage     -----------
*****************************************/
class CRC_Manage
{
/*************** Methods ****************/
protected:
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
void Append_CRC(std::vector<unsigned char> &Data);

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
**********************************************************************
*******************************/
unsigned int CRC_Calculate(const std::vector<unsigned char> &Data);
};
/*****************************************
-----------    Serial Port     -----------
*****************************************/
class Serial_Port : protected CRC_Manage
{
/*************** Methods ****************/
protected:
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
Serial_Port(const std::string& Device_Location);
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
void Send_Data(const unsigned char Data);
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
bool Receive_Data(unsigned char &Data);
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
void Receive_Data(size_t Size);
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
void Send_Data(void);
/*************** Variables **************/
protected:
std::vector<unsigned char> Data_Buffer{};
private:
boost::asio::io_context Input_Output;
boost::asio::serial_port Port;
};
/*****************************************
------------    Services     -------------
*****************************************/
class Services : private Serial_Port
{
private:
enum Bootloader_Command_t 
{
    Bootloader_Command_Get_Help            =(1),
    Bootloader_Command_Get_ID              =(2),
    Bootloader_Command_Get_Version         =(3),
    Bootloader_Command_Erase_Flash          =(4),
    Bootloader_Command_Flash_Application    =(5),
    Bootloader_Command_Address_Jump         =(6),
    Bootloader_Command_Say_Hi               =(7),
    Bootloader_Command_Say_Bye              =(8),
    Bootloader_Command_Send_Data            =(9)
};
/*************** Methods ****************/
public:
/****************************************************************************************************
* Constructor Name: Services
* Class           : Services
* Description     : Initializes the Services object with the specified device location.
* Parameters (in) : Device_Location - Reference to the string containing the device location.
* Parameters (out): None
* Return value    : None
* Notes           : - This constructor initializes the Services object by calling the constructor of the base class (Serial_Port) with the specified device location.
*****************************************************************************************************/
Services(const std::string &Device_Location);



unsigned int Calculate_Application_CRC(void);

unsigned int Get_Version(const std::string& Location);

bool Set_Application_Information(std::string &File_Location);





bool Get_File(std::string &File_Location);
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
void Get_Version(void);
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
bool Get_Version(unsigned int &ID,unsigned int &Major,unsigned int &Minor);
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
void Get_Help(void);
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
void Get_ID(void);
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
bool Get_ID(std::vector<unsigned int> &Built_ID);
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
void Jump_Address(void);
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
void Erase_Flash(void);
/****************************************************************************************************
* Function Name   : Flash_Application
* Class           : Services
* Namespace       : Bootloader
* Description     : Prompts the user to enter the start page and binary file location, and then
*                   flashes the application to the controller.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function prompts the user to enter the start page and binary file location.
*                   - It then reads the binary file and flashes the application to the controller
*                     with the specified start page and binary payload.
*****************************************************************************************************/
void Flash_Application(void);
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
bool Jump_Address(unsigned int &Address);
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
bool Erase_Flash(unsigned int &Start_Page, unsigned int &Pages_Count);
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
bool Flash_Application(unsigned int &Start_Page,std::string &File_Location);
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
void Write_Data(void);
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
bool Write_Data(unsigned int &Address,const unsigned int &Data);
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
void Exit_Bootloader(void);
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
void Start_Target_Bootloader(void);
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
bool Say_Hi(void);
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
bool Say_Bye(void);
private:
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
bool Get_Acknowledge(void);
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
void Update_Buffer(void);
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
bool Send_Frame(Bootloader_Command_t Service);
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
bool Send_Frame(std::vector<unsigned char> &Data);
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
bool Send_Frame(Bootloader_Command_t Service,std::vector<unsigned char> &Data);
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
bool Read_File(std::vector<unsigned char> &Binary_File,std::string &File_Location);
};
/*****************************************
--------------   Monitor   ---------------
*****************************************/
class Monitor
{
/*************** Methods ****************/
public:
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
Monitor(Services &User_Interface,const std::string &Repository_Path,const std::string &Binary_Location,std::vector<std::string> &Arguments);
/****************************************************************************************************
* Function Name   : Start_Monitoring
* Class           : Monitor
* Description     : Starts the monitoring process based on the provided command-line arguments.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function checks the command-line arguments and starts the update service if required.
*****************************************************************************************************/
void Start_Monitoring();
private:
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
void Update_Application(void);
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
bool Build_Directory(void);
/****************************************************************************************************
* Function Name   : Update_Available
* Class           : Monitor
* Description     : Checks whether the .bin file in the local firmware directory has a newer
*                   modification time than the last one seen.
* Parameters (in) : None
* Parameters (out): None
* Return value    : bool - True if the file is new/changed since the last check, false otherwise.
*****************************************************************************************************/
bool Update_Available(void);
/****************************************************************************************************
* Function Name   : Wait_For_Update
* Class           : Monitor
* Description     : Polls the local firmware directory until a new/changed .bin file appears.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function first tries to build the directory by calling Build_Directory.
*                   - It then polls Update_Available every Get_Update_Time_Seconds until it reports
*                     a change, at which point the new binary is already locally present (no
*                     download step needed).
*****************************************************************************************************/
void Wait_For_Update(void);
/************** Variables ***************/
private:
std::string Binary_Repository{};
std::string Directory_Location{};
std::filesystem::file_time_type Last_Write_Time{};
std::vector<std::string> &Commands;
Services &Interface;
};
/*****************************************
---------    User Interface     ----------
*****************************************/
class User_Interface : private Services , public Monitor
{
/*************** Methods ****************/
public:
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
User_Interface(const std::string &User_Interface_File,const std::string &Repository_Path,const std::string &Binary_Location,std::vector<std::string> &Arguments);
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
~User_Interface();
/****************************************************************************************************
* Function Name   : Start_Application
* Class           : User_Interface
* Description     : Starts the application interface for user interaction.
* Parameters (in) : None
* Parameters (out): None
* Return value    : None
* Notes           : - This function displays a menu for various options and handles user input accordingly.
*****************************************************************************************************/
void Start_Application(void);
/****************************************************************************************************
* Function Name   : Print_Target_Info
* Class           : User_Interface
* Description     : Prints information about the target device.
* Parameters (in) : State - Boolean indicating whether the target device is detected or not.
* Parameters (out): None
* Return value    : None
* Notes           : - This function retrieves information about the target device and prints it.
*****************************************************************************************************/
void Print_Target_Info(bool State);
};
}
/********************************************************************
 *  END OF FILE:  Bootloader_Interface.h
********************************************************************/
#endif
