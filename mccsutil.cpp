#ifndef UNICODE
#define UNICODE
#endif 

#pragma comment(lib, "Dxva2.lib")

#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <windows.h>
#include <physicalmonitorenumerationapi.h>
#include <lowlevelmonitorconfigurationapi.h>
#include <unordered_map>
#include <functional>
#include <map>

std::vector<PHYSICAL_MONITOR> vMonitors{};

std::map<std::string, BYTE> commandMap
{
    { "power", 0xD6 },
    { "input", 0x60 }
};

std::map<std::string, DWORD> powerStateMap
{
    { "on", 0x01 },
    { "standby", 0x02 },
    { "suspend", 0x03 },
    { "off", 0x04 },
    { "offb", 0x05 }
};

std::map<std::string, DWORD> videoInputMap
{
    { "analog1", 0x01 },
    { "analog2", 0x02 },
    { "digital1", 0x03 },
    { "digital2", 0x04 },
    { "composite1", 0x05 },
    { "composite2", 0x06 },
    { "svideo1", 0x07 },
    { "svideo2", 0x08 },
    { "tuner1", 0x09 },
    { "tuner2", 0x0A },
    { "tuner3", 0x0B },
    { "component1", 0x0C },
    { "component2", 0x0D },
    { "component3", 0x0E },
    { "dp1", 0x0F },
    { "dp2", 0x10 },
    { "digital3", 0x11 },
    { "digital4", 0x12 },
    // aliases
    { "vga1", 0x01 },
    { "vga2", 0x02 },
    { "dvi1", 0x03 },
    { "dvi2", 0x04 },
    { "hdmi1", 0x11 },
    { "hdmi2", 0x12 },
};

bool parseMonitorSelection(std::string s, std::vector<int> &selected)
{
    selected.clear();

    // empty string means select all monitors
    if (s.empty())
    {
        for (size_t i = 0; i < vMonitors.size(); i++)
        {
            selected.push_back(i);
        }
        return true;
    }

    // otherwise parse the selection
    try
    {
        std::string delimiter = ",";
        size_t start = 0;
        size_t end = s.find(delimiter);
        while (end != std::string::npos)
        {
            int i = std::stoi(s.substr(start, end - start));
            if (i < 1 || i > vMonitors.size())
            {
                std::cerr << "Invalid monitor index: " << i << std::endl;
                selected.clear();
                return false;
            }
            selected.push_back(i - 1);
            start = end + delimiter.length();
            end = s.find(delimiter, start);
        }
        int i = std::stoi(s.substr(start));
        if (i < 1 || i > vMonitors.size())
        {
            std::cerr << "Invalid monitor index: " << i << std::endl;
            selected.clear();
            return false;
        }
        selected.push_back(i - 1);
    }
    catch (std::invalid_argument const& ex)
    {
        std::cerr << "Invalid monitor specification" << std::endl;
        selected.clear();
        return false;
    }

    return true;
}

bool apply(int monitorNum, BYTE command, DWORD value)
{
    if (monitorNum < 0 || monitorNum >= vMonitors.size())
    {
        std::cerr << "Invalid monitor index: " << monitorNum << std::endl;
        return false;
    }
    BOOL bSuccess = SetVCPFeature(vMonitors[monitorNum].hPhysicalMonitor, command, value);
    if (bSuccess == 0)
    {
        std::cerr << "Failure during SetVCPFeature for monitor " << monitorNum + 1 << std::endl;
        return false;
    }
    return true;
}

bool apply(std::vector<int> monitors, BYTE command, DWORD value)
{
    bool success = true;
    for (size_t i = 0; i < monitors.size(); i++)
    {
        success &= apply(monitors[i], command, value);
    }
    return success;
}

int printUsage(std::vector<std::string> args)
{
    std::cout << "Usage: mccsutil [ list | power <STATE> | input <INPUT> | help ] [ <NUM>,... ]" << std::endl << std::endl
        << "Commands:" << std::endl
        << "  list            print a list of all physical monitors" << std::endl
        << "  power <STATE>   change power state <on/standby/suspend/off/offb>" << std::endl
        << "  input <INPUT>   change video input <see below>" << std::endl
        << "  help            print usage" << std::endl << std::endl
        << "Options:" << std::endl
        << "  <NUM>,...       monitor selection (used with 'power' and 'input' commands)" << std::endl << std::endl
        << "Video Inputs:" << std::endl
        << "     analog1/2" << std::endl
        << "    digital1/2/3/4" << std::endl
        << "  composite1/2" << std::endl
        << "     svideo1/2" << std::endl
        << "      tuner1/2/3" << std::endl
        << "  component1/2/3" << std::endl
        << "         dp1/2" << std::endl
        << "        vga1/2    (aliases for analog1/2)" << std::endl
        << "        dvi1/2    (aliases for digital1/2)" << std::endl
        << "       hdmi1/2    (aliases for digital3/4)" << std::endl << std::endl
        << "Examples:" << std::endl
        << "  Change monitors 1 and 2 to HDMI 2" << std::endl
        << "    mccsutil input hdmi2 1,2" << std::endl << std::endl
        << "  Change all monitors to DisplayPort 1" << std::endl
        << "    mccsutil input dp1" << std::endl << std::endl
        << "  Put monitors 3 and 4 on standby" << std::endl
        << "    mccsutil power standby 3,4" << std::endl << std::endl
        << "  Turn off all monitors, as if by button" << std::endl
        << "    mccsutil power offb" << std::endl << std::endl
        << "Notes:" << std::endl
        << "  - Not all monitors support all power states" << std::endl
        << "  - HDMI on older monitors will be on dvi1/2 (use digital1/2 instead)" << std::endl
        ;
    return 0;
}

int listMonitors(std::vector<std::string> args)
{
    for (size_t i = 0; i < vMonitors.size(); i++)
    {
        std::wcout << "Monitor " << i + 1 << " : ";
        std::wcout << vMonitors[i].szPhysicalMonitorDescription << std::endl;
    }
    return 0;
}

int changePowerState(std::vector<std::string> args)
{
    if (args.size() < 2)
    {
        std::cerr << "Invalid number of args for 'power' command" << std::endl;
        return 1;
    }

    // find out the desired power state
    BYTE command = commandMap.find(args[0])->second;
    auto result = powerStateMap.find(args[1]);
    if (result == powerStateMap.end())
    {
        std::cerr << "Invalid power state" << std::endl;
        return 1;
    }
    DWORD value = result->second;

    std::string str;
    if (args.size() == 2) { str = ""; } // all monitors
    else { str = args[2]; } // subset of monitors
    std::vector<int> selected;
    if (!parseMonitorSelection(str, selected))
    {
        return 1;
    }
    if (!apply(selected, command, value))
    {
        return 1;
    }

    return 0;
}

int changeInputSelect(std::vector<std::string> args)
{
    if (args.size() < 2)
    {
        std::cerr << "Invalid number of args for 'input' command" << std::endl;
        return 1;
    }

    // find out the desired input select
    BYTE command = commandMap.find(args[0])->second;
    auto result = videoInputMap.find(args[1]);
    if (result == videoInputMap.end())
    {
        std::cerr << "Invalid video input" << std::endl;
        return 1;
    }
    DWORD value = result->second;

    std::string str;
    if (args.size() == 2) { str = ""; } // all monitors
    else { str = args[2]; } // subset of monitors
    std::vector<int> selected;
    if (!parseMonitorSelection(str, selected))
    {
        return 1;
    }
    if (!apply(selected, command, value))
    {
        return 1;
    }

    return 0;
}

BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    BOOL bSuccess = FALSE;
    DWORD dwCount;
    bSuccess = GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &dwCount);
    if (bSuccess)
    {
        LPPHYSICAL_MONITOR lpPhysicalMonitors = new PHYSICAL_MONITOR[dwCount];
        bSuccess = GetPhysicalMonitorsFromHMONITOR(hMonitor, dwCount, lpPhysicalMonitors);
        if (bSuccess)
        {
            for (size_t i = 0; i < dwCount; i++)
            {
                vMonitors.push_back(lpPhysicalMonitors[i]);
            }
        }
        else
        {
            std::cerr << "Failed to get physical monitors from HMONITOR" << std::endl;
        }
    }
    else
    {
        std::cerr << "Failed to get number of physical monitors from HMONITOR" << std::endl;
    }

    return TRUE;
}

std::unordered_map<std::string, std::function<int(std::vector<std::string>)>> commands
{
    { "list", listMonitors },
    { "power", changePowerState },
    { "input", changeInputSelect },
    { "help", printUsage }
};

int main(int argc, char *argv[], char *envp[])
{
    std::vector<std::string> args;
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        args.push_back(arg);
    }
    if (argc < 2)
    {
        printUsage(args);
        return 1;
    }

    auto command = commands.find(args[0]);
    if (command == commands.end())
    {
        std::cerr << "Unknown command" << std::endl;
        printUsage(args);
        return 1;
    }

    // setup, execute, cleanup
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
    int error = command->second(args);
    DestroyPhysicalMonitors(vMonitors.size(), vMonitors.data());

    return error;
}
