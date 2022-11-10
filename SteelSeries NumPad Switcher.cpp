// SteelSeries NumPad Switcher.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#pragma warning(disable : 4996)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <fstream>
#include "HTTPRequest.hpp"
#include "timercpp.h"
#include "shellapi.h"
#include "resource.h"

using namespace std;

namespace {
    HINSTANCE gInstance{};
    const int IDM_EXIT = 100;
    const int IDM_RESET = 101;

    static string& getAddress()
    {
        static string uri = [] {
            ifstream MyReadFile(string{ getenv("PROGRAMDATA") } + "/SteelSeries/SteelSeries Engine 3/coreProps.json");
            string str; getline(MyReadFile, str); MyReadFile.close();

            if (auto x = str.find("address\":\""); x != string::npos) str.erase(0, x + 10).erase(str.find('"'));
            return "http://" + str;
        }();
        return uri;
    }

    // https://gist.github.com/MightyPork/6da26e382a7ad91b5496ee55fdc73db2

    static void watchdog()
    {
        http::Request request{ getAddress() + "/game_heartbeat" };
        const std::string body{ "{\"game\": \"TEST_GAME\"}" };
        const auto response = request.send("POST", body, { {"Content-Type", "application/json"} });
    }

    static void setNumLockColor()
    {
        string str = { (GetKeyState(VK_NUMLOCK) & 0x01) ? "1" : "0" };

        http::Request request{ getAddress() + "/game_event" };
        const std::string body{ "{\"game\": \"TEST_GAME\",\"event\": \"HEALTH\",\"data\": {\"value\": " + str + " }}" };
        const auto response = request.send("POST", body, { {"Content-Type", "application/json"} });
    }

    static void bindEvent()
    {
        http::Request request{ getAddress() + "/bind_game_event" };
        const std::string body = "{\"game\": \"TEST_GAME\",\"event\": \"HEALTH\",\"handlers\": [{\"device-type\": \"rgb-per-key-zones\",\"custom-zone-keys\": [83,89,90,91,92,93,94,95,96,97,98,99],\"mode\": \"color\",\"color\": [{\"low\": 0,\"high\": 0,\"color\": {\"red\": 0,\"green\": 0,\"blue\": 255}},{\"low\": 1,\"high\": 1,\"color\": {\"red\": 0,\"green\": 255,\"blue\": 0}}]}]}";
        const auto response = request.send("POST", body, { {"Content-Type", "application/json"} });

    }

    static void stopGame()
    {
        http::Request request{ getAddress() + "/stop_game" };
        const std::string body{ "{\"game\": \"TEST_GAME\"}" };
        const auto response = request.send("POST", body, { {"Content-Type", "application/json"} });
    }
    void setNotifyIcon(HWND hWnd)
    {

    }
    NOTIFYICONDATA getNid(HWND hWnd)
    {
        NOTIFYICONDATA niData{ sizeof(NOTIFYICONDATA) };
        niData.uID = 1;
        niData.hWnd = hWnd;
        niData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        niData.hIcon = LoadIcon(gInstance, MAKEINTRESOURCE(IDI_ICON1));
        strcpy(niData.szTip, "SteelSeries NumPad Switcher");
        niData.uCallbackMessage = WM_USER + 1;

        return niData;
    }
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static NOTIFYICONDATA nid = getNid(hWnd);

    switch (uMsg)
    {
    case WM_CREATE:
    {
        Shell_NotifyIcon(NIM_ADD, &nid);
        RegisterHotKey(hWnd, 1, MOD_NOREPEAT, VK_NUMLOCK);
        break;
    }

    case WM_DESTROY:
    {
        stopGame();
        Shell_NotifyIcon(NIM_DELETE, &nid);
        UnregisterHotKey(hWnd, 1);
        DestroyIcon(nid.hIcon);
        PostQuitMessage(0);
        break;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDM_EXIT) DestroyWindow(hWnd);
        else if (LOWORD(wParam) == IDM_RESET) {
            stopGame();
            bindEvent();
            setNumLockColor();            
        }
        break;
    }

    case WM_HOTKEY:
        setNumLockColor();
        break;

    case WM_USER + 1:
    {
        WORD cmd = LOWORD(lParam);
        if (cmd == WM_RBUTTONUP || cmd == WM_LBUTTONUP)
        {
            POINT pt;
            GetCursorPos(&pt);
            MENUITEMINFO separatorBtn = { 0 };
            separatorBtn.cbSize = sizeof(MENUITEMINFO);
            separatorBtn.fMask = MIIM_FTYPE;
            separatorBtn.fType = MFT_SEPARATOR;

            HMENU hmenu = CreatePopupMenu();
            InsertMenu(hmenu, -1, MF_BYPOSITION, IDM_EXIT + 1, "Reset");
            InsertMenuItem(hmenu, -1, FALSE, &separatorBtn);
            InsertMenu(hmenu, -1, MF_BYPOSITION, IDM_EXIT, "Exit");
            SetForegroundWindow(hWnd);
            TrackPopupMenu(hmenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, NULL);
            DestroyMenu(hmenu);
        }
        break;
    }        
    
    default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

    int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
    {
        if (auto hWnd = FindWindow("NUMPADSWITCHER", NULL)) SendMessage(hWnd, WM_CLOSE, NULL, NULL);

        gInstance = hInstance;
        WNDCLASSEX wx = { sizeof(WNDCLASSEX) };
        wx.lpszClassName = "NUMPADSWITCHER";
        wx.hInstance = hInstance;
        wx.lpfnWndProc = WndProc;
        RegisterClassEx(&wx);
        CreateWindowEx(0, "NUMPADSWITCHER", "", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, NULL, NULL);

        bindEvent();
        setNumLockColor();

        Timer t; t.setInterval([&]() { watchdog(); }, 12000);

        MSG msg{ 0 };
        while (GetMessage(&msg, NULL, 0, 0) != 0) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        return 0;
    }

