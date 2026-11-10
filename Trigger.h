#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <TlHelp32.h>
#include <psapi.h>
#include <shlwapi.h>
#include <shellapi.h> // ShellExecuteA için eklendi
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include "Config.h"

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "shell32.lib")

static COLORREF GetPixelColor(int x, int y) {
    HDC hdc = GetDC(nullptr);
    COLORREF c = GetPixel(hdc, x, y);
    ReleaseDC(nullptr, hdc);
    return c;
}

static void ClickMouse() {
    INPUT in[2] = {};
    in[0].type = INPUT_MOUSE;
    in[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    in[1].type = INPUT_MOUSE;
    in[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, in, sizeof(INPUT));
}

static bool IsColorClose(COLORREF a, COLORREF b, int tol) {
    return std::abs((int)GetRValue(a) - (int)GetRValue(b)) <= tol &&
        std::abs((int)GetGValue(a) - (int)GetGValue(b)) <= tol &&
        std::abs((int)GetBValue(a) - (int)GetBValue(b)) <= tol;
}

static bool IsAimLabRunning() {
    HWND h = GetForegroundWindow();
    if (!h) return false;
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (!pid) return false;
    HANDLE p = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!p) return false;
    char name[MAX_PATH] = "<unknown>";
    GetModuleFileNameExA(p, nullptr, name, MAX_PATH);
    CloseHandle(p);
    return _stricmp(PathFindFileNameA(name), "AimLab_tb.exe") == 0;
}

static bool FileExists(const std::string& path) {
    DWORD a = GetFileAttributesA(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static std::string FindAimLabPath() {
    const std::vector<std::string> drives = { "C:", "D:", "E:" };
    for (auto& d : drives) {
        std::string p = d + "\\SteamLibrary\\steamapps\\common\\Aim Lab\\AimLab_tb.exe";
        if (FileExists(p)) return p;
    }
    return "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Aim Lab\\AimLab_tb.exe";
}

static void StartAimLab() {
    std::string p = FindAimLabPath();
    if (FileExists(p)) {
        ShellExecuteA(nullptr, "open", p.c_str(), nullptr, nullptr, SW_SHOW);
        Log("AimLab baslatildi");
    }
    else {
        Log("AimLab bulunamadi!");
    }
}

static bool CloseProcessByName(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);
    if (snap == INVALID_HANDLE_VALUE) return false;
    PROCESSENTRY32 pe{}; pe.dwSize = sizeof(pe);
    bool ok = false;
    if (Process32First(snap, &pe)) {
        do {
            char exe[MAX_PATH]; size_t c = 0;
            wcstombs_s(&c, exe, MAX_PATH, pe.szExeFile, _TRUNCATE);
            if (_stricmp(exe, name) == 0) {
                HANDLE p = OpenProcess(PROCESS_TERMINATE, 0, pe.th32ProcessID);
                if (p) { TerminateProcess(p, 9); CloseHandle(p); ok = true; break; }
            }
        } while (Process32Next(snap, &pe));
    }
    CloseHandle(snap);
    return ok;
}

static void CloseAimLab() {
    if (CloseProcessByName("AimLab_tb.exe")) Log("AimLab kapatildi");
    else                                     Log("AimLab kapatilamadi");
}

static void TriggerLoop() {
    while (g_running.load()) {
        if (g_trigger.load() && IsAimLabRunning()) {
            COLORREF target = RGB(g_cfg.targetR, g_cfg.targetG, g_cfg.targetB);
            COLORREF pix = GetPixelColor(g_cfg.sampleX, g_cfg.sampleY);
            if (IsColorClose(pix, target, g_cfg.tolerance)) {
                ClickMouse();
                Sleep(g_cfg.clickDelayMs);
            }
        }
        Sleep(1);
    }
}