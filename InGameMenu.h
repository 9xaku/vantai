#pragma once
#include <Windows.h>
#include <windowsx.h>
#include <string>
#include <thread>
#include <vector>
#include <pdh.h>
#include <comdef.h>
#include <Wbemidl.h>
#include "Config.h"
#include "Trigger.h"

#pragma comment(lib, "Pdh.lib")
#pragma comment(lib, "wbemuuid.lib")

static const int IGM_W = 600;
static const int IGM_H = 420;

static HWND g_hwndInGame = nullptr;
static bool g_igmVisible = false;
static bool g_isDraggingIGM = false;
static POINT g_dragStartIGM = { 0, 0 };
static int g_mouseX_IGM = -1, g_mouseY_IGM = -1;

static int g_igmActiveTab = 0;
static bool g_isDraggingSliderIGM = false;
static int g_sliderDragType = -1;

extern bool g_discordRPC;
extern bool g_autoStart;
extern bool g_watermarkEnabled;
extern bool g_triggerModeHold;
extern std::wstring g_keyTrigger;
extern float g_cpuUsage;
extern float g_gpuUsage;
extern int g_ramUsage;
extern bool g_vgRunning;
extern bool g_aimlabRunning;
extern bool g_steamRunning;

static void MyFillRoundRectIGM(HDC dc, RECT r, COLORREF c, int radius) {
    HBRUSH b = CreateSolidBrush(c);
    HPEN p = CreatePen(PS_SOLID, 1, c);
    HBRUSH oldB = (HBRUSH)SelectObject(dc, b);
    HPEN oldP = (HPEN)SelectObject(dc, p);
    RoundRect(dc, r.left, r.top, r.right, r.bottom, radius, radius);
    SelectObject(dc, oldB); SelectObject(dc, oldP);
    DeleteObject(b); DeleteObject(p);
}

static void DrawIconIGM(HDC dc, int x, int y, const wchar_t* iconUnicode, COLORREF c, int size = 16) {
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    HFONT f = CreateFontW(size, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe MDL2 Assets");
    HFONT old = (HFONT)SelectObject(dc, f);
    TextOutW(dc, x, y, iconUnicode, (int)wcslen(iconUnicode));
    SelectObject(dc, old); DeleteObject(f);
}

static void TextOutWCustomIGM(HDC dc, int x, int y, const wchar_t* t, COLORREF c, int size = 14, bool bold = false) {
    SetTextColor(dc, c);
    SetBkMode(dc, TRANSPARENT);
    HFONT f = CreateFontW(size, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    HFONT old = (HFONT)SelectObject(dc, f);
    TextOutW(dc, x, y, t, (int)wcslen(t));
    SelectObject(dc, old); DeleteObject(f);
}

static bool IsHoverIGM(RECT r) { return g_mouseX_IGM >= r.left && g_mouseX_IGM <= r.right && g_mouseY_IGM >= r.top && g_mouseY_IGM <= r.bottom; }
static float LerpIGM(float a, float b, float t) { return a + (b - a) * t; }

static void DrawSliderIGM(HDC dc, RECT r, float animValue, float minVal, float maxVal, bool hover, bool dragging) {
    int radius = (r.bottom - r.top) / 2;
    MyFillRoundRectIGM(dc, r, RGB(40, 40, 50), radius);
    float pct = (animValue - minVal) / (maxVal - minVal);
    int fillWidth = (int)((r.right - r.left) * pct);
    RECT fillRect = { r.left, r.top, r.left + fillWidth, r.bottom };
    COLORREF fillColor = dragging ? RGB(150, 220, 255) : (hover ? RGB(130, 200, 255) : RGB(100, 180, 255));
    MyFillRoundRectIGM(dc, fillRect, fillColor, radius);
    int knobSize = (r.bottom - r.top) + 4;
    int knobX = r.left + fillWidth - knobSize / 2;
    RECT knobRect = { knobX, r.top - 2, knobX + knobSize, r.bottom + 2 };
    MyFillRoundRectIGM(dc, knobRect, RGB(230, 230, 240), knobSize / 2);
}

static void DrawToggleIGM(HDC dc, RECT r, float animState, bool hover) {
    int radius = r.bottom - r.top;
    COLORREF activeColor = RGB(80, 220, 130);
    COLORREF bg = RGB(60, 60, 70);
    if (animState > 0.0f) {
        bg = RGB(
            (int)LerpIGM(GetRValue(RGB(60, 60, 70)), GetRValue(activeColor), animState),
            (int)LerpIGM(GetGValue(RGB(60, 60, 70)), GetGValue(activeColor), animState),
            (int)LerpIGM(GetBValue(RGB(60, 60, 70)), GetBValue(activeColor), animState)
        );
    }
    if (hover && animState < 0.5f) bg = RGB(GetRValue(bg) + 15, GetGValue(bg) + 15, GetBValue(bg) + 15);
    MyFillRoundRectIGM(dc, r, bg, radius);
    int padding = 2;
    int knobSize = radius - padding * 2;
    int minX = r.left + padding;
    int maxX = r.right - knobSize - padding;
    int knobX = (int)LerpIGM((float)minX, (float)maxX, animState);
    RECT knobRect = { knobX, r.top + padding, knobX + knobSize, r.bottom - padding };
    MyFillRoundRectIGM(dc, knobRect, RGB(230, 230, 240), knobSize / 2);
}

static void DrawInGameUI(HDC dc) {
    RECT rc = { 0, 0, IGM_W, IGM_H };
    MyFillRoundRectIGM(dc, rc, RGB(15, 15, 20), 12);

    RECT hdr = { 0, 0, IGM_W, 45 };
    MyFillRoundRectIGM(dc, hdr, RGB(25, 25, 32), 12);
    TextOutWCustomIGM(dc, 20, 15, L"Vanta Internal", RGB(100, 180, 255), 18, true);

    RECT closeBtn = { IGM_W - 30, 12, IGM_W - 10, 32 };
    DrawIconIGM(dc, closeBtn.left, closeBtn.top, L"\xE711", IsHoverIGM(closeBtn) ? RGB(230, 80, 80) : RGB(130, 130, 150), 14);

    int tabW = (IGM_W - 40) / 5;
    const wchar_t* tabNames[] = { L"Aim", L"Settings", L"Discord", L"System", L"Misc" };
    for (int i = 0; i < 5; i++) {
        RECT tr = { 20 + i * tabW, 50, 20 + i * tabW + tabW - 5, 75 };
        bool active = (i == g_igmActiveTab);
        if (active) MyFillRoundRectIGM(dc, tr, RGB(40, 40, 50), 6);
        TextOutWCustomIGM(dc, tr.left + 15, tr.top + 6, tabNames[i], active ? RGB(230, 230, 240) : RGB(130, 130, 150), 14, active);
    }

    int cx = 30; int cy = 95;

    if (g_igmActiveTab == 0) {
        TextOutWCustomIGM(dc, cx, cy, L"Trigger Status:", RGB(130, 130, 150), 14);
        TextOutWCustomIGM(dc, cx + 120, cy, g_trigger.load() ? L"ENABLED" : L"DISABLED", g_trigger.load() ? RGB(80, 220, 130) : RGB(230, 80, 80), 14, true);
        cy += 30;

        RECT btnToggle = { cx, cy, cx + 200, cy + 40 };
        MyFillRoundRectIGM(dc, btnToggle, g_trigger.load() ? RGB(80, 220, 130) : RGB(230, 80, 80), 8);
        TextOutWCustomIGM(dc, cx + 60, cy + 11, g_trigger.load() ? L"DISABLE" : L"ENABLE", RGB(230, 230, 240), 16, true);
        cy += 60;

        TextOutWCustomIGM(dc, cx, cy, L"Tolerance:", RGB(130, 130, 150), 14);
        TextOutWCustomIGM(dc, cx + 130, cy, std::to_wstring(g_cfg.tolerance).c_str(), RGB(230, 230, 240), 14);
        cy += 25;
        RECT sliderTol = { cx, cy, cx + 250, cy + 10 };
        DrawSliderIGM(dc, sliderTol, (float)g_cfg.tolerance, 0.0f, 255.0f, IsHoverIGM(sliderTol), (g_isDraggingSliderIGM && g_sliderDragType == 0));
        cy += 40;

        TextOutWCustomIGM(dc, cx, cy, L"Delay (ms):", RGB(130, 130, 150), 14);
        TextOutWCustomIGM(dc, cx + 130, cy, std::to_wstring(g_cfg.clickDelayMs).c_str(), RGB(230, 230, 240), 14);
        cy += 25;
        RECT sliderDelay = { cx, cy, cx + 250, cy + 10 };
        DrawSliderIGM(dc, sliderDelay, (float)g_cfg.clickDelayMs, 1.0f, 1000.0f, IsHoverIGM(sliderDelay), (g_isDraggingSliderIGM && g_sliderDragType == 1));
    }
    else if (g_igmActiveTab == 1) {
        TextOutWCustomIGM(dc, cx, cy, L"Trigger Mode:", RGB(130, 130, 150), 14);
        RECT btnMode = { cx + 130, cy - 4, cx + 230, cy + 22 };
        MyFillRoundRectIGM(dc, btnMode, IsHoverIGM(btnMode) ? RGB(60, 60, 70) : RGB(40, 40, 50), 6);
        TextOutWCustomIGM(dc, cx + 145, cy, g_triggerModeHold ? L"HOLD" : L"TOGGLE", RGB(230, 230, 240), 14);
        cy += 40;

        TextOutWCustomIGM(dc, cx, cy, L"Hotkey:", RGB(130, 130, 150), 14);
        RECT btnBind = { cx + 130, cy - 4, cx + 230, cy + 22 };
        MyFillRoundRectIGM(dc, btnBind, IsHoverIGM(btnBind) ? RGB(60, 60, 70) : RGB(40, 40, 50), 6);
        TextOutWCustomIGM(dc, cx + 145, cy, g_keyTrigger.c_str(), RGB(230, 230, 240), 14);
    }
    else if (g_igmActiveTab == 2) {
        TextOutWCustomIGM(dc, cx, cy, L"Discord Rich Presence:", RGB(130, 130, 150), 14);
        RECT rpcRect = { cx + 180, cy - 4, cx + 220, cy + 16 };
        DrawToggleIGM(dc, rpcRect, g_discordRPC ? 1.0f : 0.0f, IsHoverIGM(rpcRect));
    }
    else if (g_igmActiveTab == 3) {
        TextOutWCustomIGM(dc, cx, cy, L"CPU Usage:", RGB(130, 130, 150), 14);
        TextOutWCustomIGM(dc, cx + 130, cy, (std::to_wstring((int)g_cpuUsage) + L"%").c_str(), RGB(80, 220, 130), 14);
        cy += 25;
        TextOutWCustomIGM(dc, cx, cy, L"GPU Usage:", RGB(130, 130, 150), 14);
        TextOutWCustomIGM(dc, cx + 130, cy, (std::to_wstring((int)g_gpuUsage) + L"%").c_str(), RGB(80, 220, 130), 14);
        cy += 25;
        TextOutWCustomIGM(dc, cx, cy, L"RAM Usage:", RGB(130, 130, 150), 14);
        TextOutWCustomIGM(dc, cx + 130, cy, (std::to_wstring(g_ramUsage) + L"%").c_str(), RGB(80, 220, 130), 14);
        cy += 40;

        TextOutWCustomIGM(dc, cx, cy, L"Vanguard:", RGB(130, 130, 150), 14);
        DrawIconIGM(dc, cx + 130, cy, g_vgRunning ? L"\xE73E" : L"\xE711", g_vgRunning ? RGB(80, 220, 130) : RGB(230, 80, 80), 14);
        cy += 25;
        TextOutWCustomIGM(dc, cx, cy, L"AimLab:", RGB(130, 130, 150), 14);
        DrawIconIGM(dc, cx + 130, cy, g_aimlabRunning ? L"\xE73E" : L"\xE711", g_aimlabRunning ? RGB(80, 220, 130) : RGB(230, 80, 80), 14);
    }
    else if (g_igmActiveTab == 4) {
        TextOutWCustomIGM(dc, cx, cy, L"Watermark:", RGB(130, 130, 150), 14);
        RECT wmRect = { cx + 180, cy - 4, cx + 220, cy + 16 };
        DrawToggleIGM(dc, wmRect, g_watermarkEnabled ? 1.0f : 0.0f, IsHoverIGM(wmRect));
        cy += 30;

        TextOutWCustomIGM(dc, cx, cy, L"Start with Windows:", RGB(130, 130, 150), 14);
        RECT asRect = { cx + 180, cy - 4, cx + 220, cy + 16 };
        DrawToggleIGM(dc, asRect, g_autoStart ? 1.0f : 0.0f, IsHoverIGM(asRect));
    }

    TextOutWCustomIGM(dc, 30, IGM_H - 25, L"Press INSERT to toggle menu visibility", RGB(100, 100, 110), 12);
}

static LRESULT CALLBACK WndProcInGame(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        g_hwndInGame = hwnd;
        HRGN hRgn = CreateRoundRectRgn(0, 0, IGM_W + 1, IGM_H + 1, 15, 15);
        SetWindowRgn(hwnd, hRgn, TRUE);
        SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
        SetTimer(hwnd, 2, 16, nullptr);
        return 0;
    }
    case WM_TIMER: {
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps; HDC dc = BeginPaint(hwnd, &ps);
        HDC memDC = CreateCompatibleDC(dc);
        HBITMAP memBM = CreateCompatibleBitmap(dc, IGM_W, IGM_H);
        SelectObject(memDC, memBM);
        DrawInGameUI(memDC);
        BitBlt(dc, 0, 0, IGM_W, IGM_H, memDC, 0, 0, SRCCOPY);
        DeleteObject(memBM); DeleteDC(memDC);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE: {
        g_mouseX_IGM = GET_X_LPARAM(lp); g_mouseY_IGM = GET_Y_LPARAM(lp);
        if (g_isDraggingIGM) {
            POINT p; GetCursorPos(&p);
            SetWindowPos(hwnd, nullptr, p.x - g_dragStartIGM.x, p.y - g_dragStartIGM.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        if (g_isDraggingSliderIGM) {
            int clickedX = g_mouseX_IGM - 30;
            if (clickedX < 0) clickedX = 0;
            if (clickedX > 250) clickedX = 250;

            if (g_sliderDragType == 0) {
                g_cfg.tolerance = (int)(255.0f * ((float)clickedX / 250.0f));
                SaveConfig();
            }
            else if (g_sliderDragType == 1) {
                g_cfg.clickDelayMs = (int)(1000.0f * ((float)clickedX / 250.0f));
                SaveConfig();
            }
        }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);
        if (x >= IGM_W - 30 && x <= IGM_W - 10 && y >= 12 && y <= 32) {
            ShowWindow(hwnd, SW_HIDE); g_igmVisible = false; return 0;
        }
        if (y < 45) {
            g_isDraggingIGM = true;
            POINT p; GetCursorPos(&p); RECT wr; GetWindowRect(hwnd, &wr);
            g_dragStartIGM.x = p.x - wr.left; g_dragStartIGM.y = p.y - wr.top; return 0;
        }

        int tabW = (IGM_W - 40) / 5;
        if (y >= 50 && y <= 75) {
            for (int i = 0; i < 5; i++) {
                if (x >= 20 + i * tabW && x <= 20 + i * tabW + tabW - 5) { g_igmActiveTab = i; return 0; }
            }
        }

        int cx = 30; int cy = 95;
        if (g_igmActiveTab == 0) {
            cy += 30;
            if (x >= cx && x <= cx + 200 && y >= cy && y <= cy + 40) {
                bool ns = !g_trigger.load(); g_trigger.store(ns); g_cfg.triggerEnabled = ns; SaveConfig();
                return 0;
            }
            cy += 60 + 25;
            RECT sliderTol = { cx, cy, cx + 250, cy + 10 };
            if (IsHoverIGM(sliderTol)) { g_isDraggingSliderIGM = true; g_sliderDragType = 0; return 0; }
            cy += 40 + 25;
            RECT sliderDelay = { cx, cy, cx + 250, cy + 10 };
            if (IsHoverIGM(sliderDelay)) { g_isDraggingSliderIGM = true; g_sliderDragType = 1; return 0; }
        }
        else if (g_igmActiveTab == 1) {
            if (x >= cx + 130 && x <= cx + 230 && y >= cy - 4 && y <= cy + 22) {
                g_triggerModeHold = !g_triggerModeHold;
                return 0;
            }
        }
        else if (g_igmActiveTab == 2) {
            if (x >= cx + 180 && x <= cx + 220 && y >= cy - 4 && y <= cy + 16) {
                g_discordRPC = !g_discordRPC;
                return 0;
            }
        }
        else if (g_igmActiveTab == 4) {
            if (x >= cx + 180 && x <= cx + 220 && y >= cy - 4 && y <= cy + 16) {
                g_watermarkEnabled = !g_watermarkEnabled;
                return 0;
            }
            cy += 30;
            if (x >= cx + 180 && x <= cx + 220 && y >= cy - 4 && y <= cy + 16) {
                g_autoStart = !g_autoStart;
                return 0;
            }
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        g_isDraggingIGM = false;
        g_isDraggingSliderIGM = false;
        g_sliderDragType = -1;
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}