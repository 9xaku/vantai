#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <atomic>
#include <string>
#include <fstream>
#include <mutex>
#include <cstdio>

struct Config {
    bool  triggerEnabled = false;
    int   tolerance = 30;
    int   clickDelayMs = 10;
    int   sampleX = 960;
    int   sampleY = 540;
    int   targetR = 0;
    int   targetG = 0;
    int   targetB = 0;
    int   windowX = 200;
    int   windowY = 200;
    int   activeTab = 0;
};

inline Config            g_cfg;
inline std::atomic<bool> g_running{ true };
inline std::atomic<bool> g_trigger{ false };
inline std::atomic<bool> g_menuVisible{ true };

inline const wchar_t* CONFIG_FILE = L"config.ini";
inline const wchar_t* LOG_FILE = L"akubot.log";

inline void Log(const std::string& msg) {
    static std::mutex m;
    std::lock_guard<std::mutex> lk(m);
    SYSTEMTIME st; GetLocalTime(&st);
    std::ofstream f(LOG_FILE, std::ios::app);
    if (!f) return;
    char buf[32];
    sprintf_s(buf, "%02d:%02d:%02d ", st.wHour, st.wMinute, st.wSecond);
    f << buf << msg << "\n";
}

inline void LoadConfig() {
    std::ifstream f(CONFIG_FILE);
    if (!f) return;
    std::string line;
    while (std::getline(f, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        try {
            if (k == "triggerEnabled") g_cfg.triggerEnabled = std::stoi(v) != 0;
            else if (k == "tolerance")      g_cfg.tolerance = std::stoi(v);
            else if (k == "clickDelayMs")   g_cfg.clickDelayMs = std::stoi(v);
            else if (k == "sampleX")        g_cfg.sampleX = std::stoi(v);
            else if (k == "sampleY")        g_cfg.sampleY = std::stoi(v);
            else if (k == "targetR")        g_cfg.targetR = std::stoi(v);
            else if (k == "targetG")        g_cfg.targetG = std::stoi(v);
            else if (k == "targetB")        g_cfg.targetB = std::stoi(v);
            else if (k == "windowX")        g_cfg.windowX = std::stoi(v);
            else if (k == "windowY")        g_cfg.windowY = std::stoi(v);
            else if (k == "activeTab")      g_cfg.activeTab = std::stoi(v);
        }
        catch (...) {}
    }
    g_trigger.store(g_cfg.triggerEnabled);
}

inline void SaveConfig() {
    std::ofstream f(CONFIG_FILE, std::ios::trunc);
    if (!f) return;
    f << "triggerEnabled=" << (g_trigger.load() ? 1 : 0) << "\n";
    f << "tolerance=" << g_cfg.tolerance << "\n";
    f << "clickDelayMs=" << g_cfg.clickDelayMs << "\n";
    f << "sampleX=" << g_cfg.sampleX << "\n";
    f << "sampleY=" << g_cfg.sampleY << "\n";
    f << "targetR=" << g_cfg.targetR << "\n";
    f << "targetG=" << g_cfg.targetG << "\n";
    f << "targetB=" << g_cfg.targetB << "\n";
    f << "windowX=" << g_cfg.windowX << "\n";
    f << "windowY=" << g_cfg.windowY << "\n";
    f << "activeTab=" << g_cfg.activeTab << "\n";
}