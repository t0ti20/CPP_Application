/*******************************************************************
 *  FILE DESCRIPTION
-----------------------
 *  Author: Khaled El-Sayed @t0ti20
 *  File: Bootloader.cpp
 *  Date: March 28, 2024
 *  Description: Application For Bootloader
 *  (C) 2023 "@t0ti20". All rights reserved.
*******************************************************************/
/*****************************************
-----------     INCLUDES     -------------
*****************************************/
#include "Bootloader_Interface.hpp"
/*****************************************
---------    Configurations     ----------
*****************************************/
/* Serial Driver Used */
constexpr const char Serial_Driver[]            {"/dev/ttyS0"};
/* Repo Location To Be Downloadded */
constexpr const char Binary_Repo[]              {"/home/root/FOTA"};
/* Application Binary File */
constexpr const char Binary_File[]              {"/Application/Build/"};
/*****************************************
----------   Main Application   ----------
*****************************************/
int main(int argc, char* argv[]) 
{
    /* Use Bootloader Namespace */
    using namespace Bootloader;
    /* Store Program Arguments */
    std::vector<std::string> Arguments;
    /* Setup Appliaction Configuration */
    User_Interface Application
    {
        Serial_Driver,
        Binary_Repo,
        std::string(Binary_Repo)+std::string(Binary_File),
        Arguments
    };
    /* Store Entered Arguments */
    for (size_t Counter=1;Counter<argc;++Counter){Arguments.emplace_back(argv[Counter]);}
    /* If There Is Arguments Start Monitoring Mode */
    if(Arguments.size())
    {
        Application.Start_Monitoring();
    }
    /* Start CLI Mode */
    else
    {
        if(system("clear")){};
        Application.Start_Application();
    }
    return 0;
}
/********************************************************************
 *  END OF FILE:  Bootloader.cpp 
********************************************************************/