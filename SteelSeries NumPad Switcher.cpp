// SteelSeries NumPad Switcher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#pragma warning(disable : 4996)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <fstream>
#include <psapi.h>
#include "HTTPRequest.hpp"
#include "timercpp.h"

using namespace std;

string &getAddress()
{
    static string uri{};
    if (uri.empty()) {
        ifstream MyReadFile(string{ getenv("PROGRAMDATA") } + "/SteelSeries/SteelSeries Engine 3/coreProps.json");
        string str; getline(MyReadFile, str); MyReadFile.close();

        if (auto x = str.find("address\":\""); x != string::npos) str.erase(0, x + 10).erase(str.find('"'));
        uri = "http://" + str;
    }
    return uri;
}

void watchdog()
{
    http::Request request{ getAddress() + "/game_heartbeat" };
    const std::string body{ "{\"game\": \"TEST_GAME\"}" };
    const auto response = request.send("POST", body, { {"Content-Type", "application/json"} });
}

bool isRunning()
{
    char buffer[128];
    GetModuleBaseNameA(GetCurrentProcess(), NULL, buffer, 128);

    auto mutex = CreateMutexA(NULL, 0, buffer);
    return (GetLastError() == ERROR_ALREADY_EXISTS);
}

void setNumLockColor()
{
    string str = { (GetKeyState(VK_NUMLOCK) & 0x01) ? "1" : "0" };

    http::Request request{ getAddress() + "/game_event" };
    const std::string body{ "{\"game\": \"TEST_GAME\",\"event\": \"HEALTH\",\"data\": {\"value\": " + str + " }}" };
    const auto response = request.send("POST", body, { {"Content-Type", "application/json"} });
}

// https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2



int main()
{
    if(isRunning()) exit(0);

    http::Request request{ getAddress() + "/bind_game_event" };
    const std::string body = "{\"game\": \"TEST_GAME\",\"event\": \"HEALTH\",\"handlers\": [{\"device-type\": \"rgb-per-key-zones\",\"custom-zone-keys\": [83,89,90,91,92,93,94,95,96,97,98,99],\"mode\": \"color\",\"color\": [{\"low\": 0,\"high\": 0,\"color\": {\"red\": 0,\"green\": 0,\"blue\": 255}},{\"low\": 1,\"high\": 1,\"color\": {\"red\": 0,\"green\": 255,\"blue\": 0}}]}]}";
    const auto response = request.send("POST", body, {{"Content-Type", "application/json"}});

    setNumLockColor();

    Timer t; t.setInterval([&]() { setNumLockColor(); }, 10000);

    RegisterHotKey(NULL, 1, 0, VK_NUMLOCK);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0) != 0) if (msg.message == WM_HOTKEY) setNumLockColor();

    return 0;
}

