// ============================================================================
//  VANTA.AI Loader  -  yeniden tasarlanmis surum
//  NOT: Dosyayi "UTF-8 with BOM" olarak kaydedin (ya da derleyiciye /utf-8 verin).
// ============================================================================
#pragma execution_character_set("utf-8")
#define NOMINMAX
#include <Windows.h>
#include <windowsx.h>
#include <TlHelp32.h>
#include <string>
#include <thread>
#include <atomic>
#include <memory>
#include <random>
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <vector>
#include <fstream>
#include <sstream>
#include <shlobj.h>
#include <gdiplus.h>
#include <algorithm>
#include "discord.h"
#include "Config.h"
#include "Trigger.h"
#include "InGameMenu.h"

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "Gdiplus.lib")
#pragma comment(lib, "Shell32.lib")
#pragma comment(lib, "Advapi32.lib")

using namespace Gdiplus;

// ============================================================================
//  DIS DEGISKENLER (diger dosyalar kullaniyor - ismleri degistirmeyin)
// ============================================================================
bool  g_discordRPC = false;
bool  g_autoStart = false;
bool  g_watermarkEnabled = false;
bool  g_triggerModeHold = false;
std::wstring g_keyTrigger = L"INSERT";
float g_cpuUsage = 0.0f;
float g_gpuUsage = 0.0f;
int   g_ramUsage = 0;
bool  g_vgRunning = false;
bool  g_aimlabRunning = false;
bool  g_steamRunning = false;

// ============================================================================
//  YERLESIM
// ============================================================================
namespace Layout {
    constexpr int   WinW = 600;
    constexpr int   WinH = 450;
    constexpr int   HeaderH = 70;
    constexpr int   BtnW = 380;
    constexpr int   BtnH = 80;
    constexpr int   BtnX = (WinW - BtnW) / 2;
    constexpr int   BtnY = 280;
    constexpr float BtnRadius = 10.0f;
    constexpr int   CloseL = WinW - 52, CloseR = WinW - 18, CloseT = 18, CloseB = 52;
    constexpr DWORD LaunchTimeoutMs = 3000;
    constexpr float Pi = 3.14159265f;
}

// ============================================================================
//  RENK TEMASI  (sade, koyu grafit + celik mavisi tek vurgu)
// ============================================================================
namespace Theme {
    inline Color BgTop() { return Color(255, 14, 16, 21); }
    inline Color BgBottom() { return Color(255, 8, 9, 13); }
    inline Color Accent() { return Color(255, 92, 146, 214); }
    inline Color AccentSoft() { return Color(255, 150, 180, 224); }
    inline Color TextMain() { return Color(255, 236, 239, 245); }
    inline Color TextSub() { return Color(255, 140, 148, 162); }
    inline Color TextDim() { return Color(255, 84, 90, 102); }
    inline Color Border() { return Color(255, 48, 53, 64); }
    inline Color Success() { return Color(255, 74, 190, 130); }
}

// ============================================================================
//  DURUM
// ============================================================================
struct AnimState {
    float time = 0.0f;        // surekli akan zaman
    float intro = 0.0f;       // acilis animasyonu zamani
    float closeT = 0.0f;      // kapanis ilerlemesi 0..1
    float hover = 0.0f;       // buton hover 0..1
    float click = 0.0f;       // buton basma 0..1
    float closeHover = 0.0f;  // kapat butonu hover
    float parX = 0.0f, parY = 0.0f;
};

struct MouseState {
    int  x = -1, y = -1;
    bool tracking = false;
    bool dragging = false;
    POINT dragStart{ 0, 0 };
    bool Inside() const { return x >= 0 && y >= 0; }
};

struct Particle { float x, y, vx, vy, size, phase; };
struct Ripple { float x = 0, y = 0, t = 0; bool active = false; };

static constexpr int PARTICLE_COUNT = 34;

static AnimState   A;
static MouseState  g_mouse;
static Particle    g_particles[PARTICLE_COUNT];
static Ripple      g_ripples[4];

static ULONG_PTR   g_gdiplusToken = 0;
static HWND        g_hwndLoader = nullptr;
static std::thread g_worker;
static std::thread g_discordThread;
static bool        g_closing = false;
static bool        g_finalized = false;
static bool        g_isLaunching = false;
static ULONGLONG   g_launchStart = 0;
static LARGE_INTEGER g_qpcFreq{}, g_qpcLast{};

static IDiscordCore* g_discordCore = nullptr;
static IDiscordActivityManager* g_activityManager = nullptr;
static bool        g_discordConnected = false;

// Arka tampon
static HDC     g_memDC = nullptr;
static HBITMAP g_memBM = nullptr;
static HGDIOBJ g_memOld = nullptr;

static std::mt19937 g_rng{ std::random_device{}() };

// ============================================================================
//  FONTLAR
// ============================================================================
struct Fonts {
    std::unique_ptr<FontFamily> segoe, black;
    int   blackStyle = FontStyleRegular;
    std::unique_ptr<Gdiplus::Font> title, logo, sub, v, anta, btn, foot, footTiny;
    float vSize = 44.0f, antaSize = 10.0f;
    float vAscent = 0.0f, antaAscent = 0.0f;   // piksel cinsinden
};
static Fonts g_fonts;

// ============================================================================
//  MATEMATIK / YARDIMCILAR
// ============================================================================
static float Clamp01(float v) { return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v); }
static float Lerp(float a, float b, float t) { return a + (b - a) * t; }
static BYTE  B(float v) { return (BYTE)(v < 0.0f ? 0.0f : (v > 255.0f ? 255.0f : v)); }
static float Damp(float cur, float target, float speed, float dt) {
    return cur + (target - cur) * (1.0f - expf(-speed * dt));
}
static float EaseOutCubic(float t) { t = Clamp01(t); float u = 1.0f - t; return 1.0f - u * u * u; }
static float Stage(float time, float start, float dur) { return Clamp01((time - start) / dur); }
static float Rand01() { return std::uniform_real_distribution<float>(0.0f, 1.0f)(g_rng); }

static Color LerpColor(const Color& a, const Color& b, float t) {
    return Color(B(Lerp((float)a.GetA(), (float)b.GetA(), t)),
        B(Lerp((float)a.GetR(), (float)b.GetR(), t)),
        B(Lerp((float)a.GetG(), (float)b.GetG(), t)),
        B(Lerp((float)a.GetB(), (float)b.GetB(), t)));
}
// alfayi 0..255 mutlak degerle degistirir
static Color WithAlpha(const Color& c, float a) { return Color(B(a), c.GetR(), c.GetG(), c.GetB()); }
// alfayi carpan ile olcekler
static Color Fade(const Color& c, float k) { return Color(B(c.GetA() * k), c.GetR(), c.GetG(), c.GetB()); }

static bool InClose(int x, int y) {
    return x >= Layout::CloseL && x <= Layout::CloseR && y >= Layout::CloseT && y <= Layout::CloseB;
}
static bool InButton(int x, int y) {
    return x >= Layout::BtnX && x <= Layout::BtnX + Layout::BtnW &&
        y >= Layout::BtnY - 6 && y <= Layout::BtnY + Layout::BtnH;
}

// Islem adina gore PID dondurur (bulunamazsa 0)
static DWORD GetProcessIdByName(const wchar_t* processName) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, processName) == 0) { pid = pe.th32ProcessID; break; }
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return pid;
}

static bool IsProcessRunning(const wchar_t* processName) {
    return GetProcessIdByName(processName) != 0;
}

static void RoundRectPath(GraphicsPath& p, float x, float y, float w, float h, float r) {
    r = std::min(r, std::min(w, h) / 2.0f);
    float d = r * 2.0f;
    p.AddArc(x, y, d, d, 180, 90);
    p.AddArc(x + w - d, y, d, d, 270, 90);
    p.AddArc(x + w - d, y + h - d, d, d, 0, 90);
    p.AddArc(x, y + h - d, d, d, 90, 90);
    p.CloseFigure();
}

static void DrawGlow(Graphics& g, float cx, float cy, float r, const Color& center) {
    GraphicsPath p;
    p.AddEllipse(cx - r, cy - r, r * 2.0f, r * 2.0f);
    PathGradientBrush b(&p);
    b.SetCenterColor(center);
    Color s(0, center.GetR(), center.GetG(), center.GetB());
    int n = 1;
    b.SetSurroundColors(&s, &n);
    g.FillPath(&b, &p);
}

// Harf araligi (tracking) uygulayarak ortalanmis yazi cizer
static float MeasureTracked(Graphics& g, const std::wstring& s, Gdiplus::Font* f, float tracking) {
    const StringFormat* tf = StringFormat::GenericTypographic();
    float w = 0.0f;
    for (wchar_t c : s) {
        if (c == L' ') { w += f->GetSize() * 0.32f + tracking; continue; }
        RectF r;
        g.MeasureString(&c, 1, f, PointF(0, 0), tf, &r);
        w += r.Width + tracking;
    }
    return w > 0.0f ? w - tracking : 0.0f;
}
static void DrawTrackedCentered(Graphics& g, const std::wstring& s, Gdiplus::Font* f,
    const Brush* br, float cx, float y, float tracking) {
    const StringFormat* tf = StringFormat::GenericTypographic();
    float x = cx - MeasureTracked(g, s, f, tracking) / 2.0f;
    for (wchar_t c : s) {
        if (c == L' ') { x += f->GetSize() * 0.32f + tracking; continue; }
        RectF r;
        g.MeasureString(&c, 1, f, PointF(0, 0), tf, &r);
        g.DrawString(&c, 1, f, PointF(x, y), tf, br);
        x += r.Width + tracking;
    }
}

// ============================================================================
//  KAYNAKLAR
// ============================================================================
static std::unique_ptr<FontFamily> MakeFamily(const wchar_t* name) {
    auto f = std::make_unique<FontFamily>(name);
    if (!f->IsAvailable()) f.reset(FontFamily::GenericSansSerif()->Clone());
    return f;
}

static void CreateResources(HWND hwnd) {
    using namespace Layout;

    g_fonts.segoe = MakeFamily(L"Segoe UI");
    {
        FontFamily probe(L"Segoe UI Black");
        if (probe.IsAvailable()) {
            g_fonts.black = std::make_unique<FontFamily>(L"Segoe UI Black");
            g_fonts.blackStyle = FontStyleRegular;
        }
        else {
            g_fonts.black = MakeFamily(L"Segoe UI");
            g_fonts.blackStyle = FontStyleBold;
        }
    }
    const int bs = g_fonts.blackStyle;
    auto mk = [](const FontFamily* fam, float size, int style) {
        return std::make_unique<Gdiplus::Font>(fam, size, style, UnitPixel);
        };
    g_fonts.title = mk(g_fonts.black.get(), 30.0f, bs);
    g_fonts.logo = mk(g_fonts.black.get(), 60.0f, bs);
    g_fonts.sub = mk(g_fonts.segoe.get(), 11.0f, FontStyleBold);
    g_fonts.v = mk(g_fonts.black.get(), g_fonts.vSize, bs);
    g_fonts.anta = mk(g_fonts.segoe.get(), g_fonts.antaSize, FontStyleBold);
    g_fonts.btn = mk(g_fonts.black.get(), 21.0f, bs);
    g_fonts.foot = mk(g_fonts.segoe.get(), 11.0f, FontStyleRegular);
    g_fonts.footTiny = mk(g_fonts.segoe.get(), 9.0f, FontStyleItalic);

    // Baseline hesabi icin ascent (piksel)
    auto ascentPx = [](const FontFamily* fam, int style, float size) {
        return size * (float)fam->GetCellAscent(style) / (float)fam->GetEmHeight(style);
        };
    g_fonts.vAscent = ascentPx(g_fonts.black.get(), bs, g_fonts.vSize);
    g_fonts.antaAscent = ascentPx(g_fonts.segoe.get(), FontStyleBold, g_fonts.antaSize);

    // Cift tamponlama
    HDC dc = GetDC(hwnd);
    g_memDC = CreateCompatibleDC(dc);
    g_memBM = CreateCompatibleBitmap(dc, WinW, WinH);
    g_memOld = SelectObject(g_memDC, g_memBM);
    ReleaseDC(hwnd, dc);

    // Parcaciklar
    for (auto& p : g_particles) {
        p.x = Rand01() * WinW;
        p.y = Rand01() * WinH;
        float ang = Rand01() * 2.0f * Pi;
        float spd = 5.0f + Rand01() * 11.0f;
        p.vx = cosf(ang) * spd;
        p.vy = sinf(ang) * spd;
        p.size = 1.5f + Rand01() * 2.0f;
        p.phase = Rand01() * 6.28f;
    }
}

static void FreeResources() {
    g_fonts = Fonts();   // tum GDI+ nesneleri GdiplusShutdown'dan once serbest kalir
    if (g_memDC) {
        SelectObject(g_memDC, g_memOld);
        DeleteObject(g_memBM);
        DeleteDC(g_memDC);
        g_memDC = nullptr; g_memBM = nullptr; g_memOld = nullptr;
    }
}

// ============================================================================
//  DISCORD RPC  (ayri thread - kapanista join edilir)
// ============================================================================
static void DiscordDisconnect() {
    if (g_discordCore) { g_discordCore->destroy(g_discordCore); g_discordCore = nullptr; }
    g_activityManager = nullptr;
    g_discordConnected = false;
}

static void DiscordRPCThread() {
    const int64_t kClientId = 1569417109531009024;   
    DiscordCreateParams params{};
    params.client_id = kClientId;
    params.flags = DiscordCreateFlags_NoRequireDiscord;

    ULONGLONG lastTry = 0;
    while (g_running.load()) {
        if (g_discordRPC) {
            if (!g_discordConnected) {
                if (GetTickCount64() - lastTry >= 3000) {
                    lastTry = GetTickCount64();
                    auto result = DiscordCreate(DISCORD_VERSION, &params, &g_discordCore);
                    if (result == DiscordResult_Ok && g_discordCore) {
                        g_activityManager = g_discordCore->get_activity_manager(g_discordCore);
                        if (g_activityManager) {
                            DiscordActivity activity = {};
                            strcpy_s(activity.state, sizeof(activity.state), "Launcher");
                            strcpy_s(activity.details, sizeof(activity.details), "Vanta.ai");
                            DiscordActivityTimestamps timestamps = { (int64_t)time(nullptr), 0 };
                            activity.timestamps = timestamps;
                            DiscordActivityAssets assets = { "logo", "Vanta.ai", "banner", "vanta.gg" };
                            activity.assets = assets;
                            g_activityManager->update_activity(g_activityManager, &activity, nullptr, nullptr);
                            g_discordConnected = true;
                        }
                        else {
                            DiscordDisconnect();
                        }
                    }
                    else {
                        g_discordCore = nullptr;
                    }
                }
            }
            else if (g_discordCore) {
                if (g_discordCore->run_callbacks(g_discordCore) != DiscordResult_Ok)
                    DiscordDisconnect();   // Discord kapandiysa temizle, tekrar denenecek
            }
        }
        else if (g_discordConnected) {
            DiscordDisconnect();
        }
        Sleep(100);
    }
    DiscordDisconnect();
}

// ============================================================================
//  ANIMASYON GUNCELLEME
// ============================================================================
static void UpdateAnimations(float dt) {
    using namespace Layout;
    A.time += dt;
    A.intro += dt;

    const bool mouseIn = g_mouse.Inside();

    // Hafif parallax
    float tx = mouseIn ? (WinW / 2.0f - g_mouse.x) * 0.04f : 0.0f;
    float ty = mouseIn ? (WinH / 2.0f - g_mouse.y) * 0.04f : 0.0f;
    A.parX = Damp(A.parX, tx, 3.0f, dt);
    A.parY = Damp(A.parY, ty, 3.0f, dt);

    const bool hoverBtn = mouseIn && InButton(g_mouse.x, g_mouse.y) && !g_isLaunching && !g_closing;
    const bool hoverClose = mouseIn && InClose(g_mouse.x, g_mouse.y);
    A.hover = Damp(A.hover, hoverBtn ? 1.0f : 0.0f, 10.0f, dt);
    A.click = Damp(A.click, 0.0f, 14.0f, dt);
    A.closeHover = Damp(A.closeHover, hoverClose ? 1.0f : 0.0f, 14.0f, dt);

    for (auto& r : g_ripples) {
        if (!r.active) continue;
        r.t += dt * 1.6f;
        if (r.t >= 1.0f) r.active = false;
    }

    for (auto& p : g_particles) {
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        if (p.x < -10) p.x = WinW + 10;
        if (p.x > WinW + 10) p.x = -10;
        if (p.y < -10) p.y = WinH + 10;
        if (p.y > WinH + 10) p.y = -10;
        if (mouseIn) {
            float dx = p.x - g_mouse.x, dy = p.y - g_mouse.y;
            float d = sqrtf(dx * dx + dy * dy);
            if (d < 90.0f && d > 0.01f) {
                float push = (90.0f - d) / 90.0f * 110.0f * dt;
                p.x += dx / d * push;
                p.y += dy / d * push;
            }
        }
    }
}

// ============================================================================
//  CIZIM
// ============================================================================
static void DrawBackground(Graphics& g) {
    using namespace Layout;
    LinearGradientBrush bg(Point(0, 0), Point(0, WinH), Theme::BgTop(), Theme::BgBottom());
    g.FillRectangle(&bg, 0, 0, WinW, WinH);

    const float t = A.time, px = A.parX, py = A.parY;

    // Cok hafif, yavas hareket eden ambiyans isiklari
    DrawGlow(g, 170 + sinf(t * 0.35f) * 70 + px, 110 + cosf(t * 0.30f) * 40 + py, 300, Color(30, 70, 105, 175));
    DrawGlow(g, 450 + cosf(t * 0.30f) * 80 + px * 1.4f, 360 + sinf(t * 0.40f) * 50 + py * 1.4f, 280, Color(24, 55, 75, 130));

    // Parcacik baglanti cizgileri
    const float maxD = 80.0f;
    Pen linePen(Color(0, 110, 140, 185), 1.0f);
    for (int i = 0; i < PARTICLE_COUNT; i++) {
        for (int j = i + 1; j < PARTICLE_COUNT; j++) {
            float dx = g_particles[i].x - g_particles[j].x;
            float dy = g_particles[i].y - g_particles[j].y;
            float d2 = dx * dx + dy * dy;
            if (d2 < maxD * maxD) {
                float k = 1.0f - sqrtf(d2) / maxD;
                linePen.SetColor(Color(B(k * 28), 110, 140, 185));
                g.DrawLine(&linePen, g_particles[i].x + px * 0.5f, g_particles[i].y + py * 0.5f,
                    g_particles[j].x + px * 0.5f, g_particles[j].y + py * 0.5f);
            }
        }
    }

    SolidBrush pb(Color(60, 120, 150, 190));
    for (auto& p : g_particles) {
        float a = 50.0f + 30.0f * sinf(t * 1.6f + p.phase);
        pb.SetColor(Color(B(a), 125, 155, 195));
        g.FillEllipse(&pb, p.x + px * 0.5f - p.size / 2, p.y + py * 0.5f - p.size / 2, p.size, p.size);
    }
}

static void DrawHeader(Graphics& g) {
    using namespace Layout;
    const float k = EaseOutCubic(Stage(A.intro, 0.0f, 0.5f));

    SolidBrush headerBrush(Color(B(240 * k), 16, 18, 24));
    g.FillRectangle(&headerBrush, 0, 0, WinW, HeaderH);

    // Alt ayirici cizgi + yavasca gezen vurgu
    SolidBrush sep(Color(B(255 * k), 40, 45, 55));
    g.FillRectangle(&sep, 0, HeaderH - 2, WinW, 2);

    float cx = fmodf(A.time * 110.0f, WinW + 400.0f) - 200.0f;
    LinearGradientBrush hl(PointF(cx - 150.0f, 0), PointF(cx + 150.0f, 0), Theme::Accent(), Theme::Accent());
    Color cols[3] = { Color(0, 92, 146, 214), Color(B(210 * k), 92, 146, 214), Color(0, 92, 146, 214) };
    REAL pos[3] = { 0.0f, 0.5f, 1.0f };
    hl.SetInterpolationColors(cols, pos, 3);
    g.FillRectangle(&hl, cx - 150.0f, (REAL)(HeaderH - 2), 300.0f, 2.0f);

    // Baslik
    float tx = 30.0f - (1.0f - EaseOutCubic(Stage(A.intro, 0.05f, 0.55f))) * 30.0f;
    float ta = Stage(A.intro, 0.05f, 0.4f);
    SolidBrush titleBrush(Fade(Theme::TextMain(), ta));
    g.DrawString(L"VANTA.AI", -1, g_fonts.title.get(), PointF(tx, 15), &titleBrush);

    // Kapat butonu
    const float ccx = (Layout::CloseL + Layout::CloseR) / 2.0f;
    const float ccy = (Layout::CloseT + Layout::CloseB) / 2.0f;
    SolidBrush hoverBg(Color(B(A.closeHover * 26), 255, 255, 255));
    g.FillEllipse(&hoverBg, ccx - 16.0f, ccy - 16.0f, 32.0f, 32.0f);

    Pen closePen(LerpColor(Theme::TextSub(), Color(255, 255, 255, 255), A.closeHover), 1.8f);
    closePen.SetStartCap(LineCapRound);
    closePen.SetEndCap(LineCapRound);
    g.DrawLine(&closePen, ccx - 5.5f, ccy - 5.5f, ccx + 5.5f, ccy + 5.5f);
    g.DrawLine(&closePen, ccx + 5.5f, ccy - 5.5f, ccx - 5.5f, ccy + 5.5f);
}

static void DrawCenterText(Graphics& g) {
    using namespace Layout;
    const float e = EaseOutCubic(Stage(A.intro, 0.15f, 0.7f));
    const float a = Stage(A.intro, 0.15f, 0.5f);

    const float cx = WinW / 2.0f;
    const float bob = sinf(A.time * 1.4f) * 2.0f;
    const float centerY = 185.0f + bob - (1.0f - e) * 30.0f;

    Gdiplus::Font* logo = g_fonts.logo.get();
    StringFormat sf(StringFormat::GenericTypographic());
    sf.SetAlignment(StringAlignmentCenter);

    RectF tb;
    g.MeasureString(L"NO MORE MISS", -1, logo, PointF(0, 0), &sf, &tb);
    const float lineH = logo->GetHeight(&g);
    const RectF layout(0.0f, centerY - lineH / 2.0f, (REAL)WinW, lineH);

    // Yumusak arka isik
    float pulse = 0.7f + 0.3f * sinf(A.time * 1.8f);
    DrawGlow(g, cx, centerY, 230.0f, Color(B(22 * pulse * a), 92, 146, 214));

    // Golge
    SolidBrush shadow(Color(B(120 * a), 0, 0, 0));
    g.DrawString(L"NO MORE MISS", -1, logo, RectF(layout.X + 2, layout.Y + 3, layout.Width, layout.Height), &sf, &shadow);

    // Gumus -> celik mavisi -> gumus, yavas akan gecis
    float shift = sinf(A.time * 0.7f) * 90.0f;
    float x0 = cx - tb.Width / 2.0f + shift;
    LinearGradientBrush grad(PointF(x0, 0), PointF(x0 + tb.Width, 0), Theme::TextMain(), Theme::TextMain());
    Color cols[3] = { Fade(Theme::TextMain(), a), Fade(Color(255, 138, 170, 220), a), Fade(Theme::TextMain(), a) };
    REAL pos[3] = { 0.0f, 0.5f, 1.0f };
    grad.SetInterpolationColors(cols, pos, 3);
    grad.SetWrapMode(WrapModeTileFlipX);
    g.DrawString(L"NO MORE MISS", -1, logo, layout, &sf, &grad);

    // Alt baslik (harf araligi ile, bozuk karakter yok: \u2022 = bullet)
    float sa = Stage(A.intro, 0.55f, 0.5f);
    SolidBrush subBrush(Fade(Theme::TextSub(), sa));
    DrawTrackedCentered(g, L"PRECISION  \u2022  SPEED  \u2022  CONTROL", g_fonts.sub.get(), &subBrush,
        cx, 232.0f + (1.0f - sa) * 8.0f, 3.2f);
}

static void DrawButton(Graphics& g) {
    using namespace Layout;
    const float intro = EaseOutCubic(Stage(A.intro, 0.35f, 0.6f));
    const float fade = Stage(A.intro, 0.35f, 0.4f);
    const float hov = A.hover;
    const bool  launching = g_isLaunching;
    const float act = launching ? 1.0f : hov;   // "aktif" gorunum katsayisi

    const float bx = (float)BtnX, bw = (float)BtnW, bh = (float)BtnH;
    const float y = BtnY + (1.0f - intro) * 30.0f - hov * 3.0f + A.click * 2.0f;
    const float scale = 1.0f + hov * 0.010f - A.click * 0.012f;

    GraphicsState state = g.Save();
    g.TranslateTransform(bx + bw / 2.0f, y + bh / 2.0f);
    g.ScaleTransform(scale, scale);
    g.TranslateTransform(-(bx + bw / 2.0f), -(y + bh / 2.0f));

    // ---- Dis isik (hover/launch) ----
    if (act > 0.01f)
        DrawGlow(g, bx + bw / 2.0f, y + bh / 2.0f + 8.0f, 250.0f, Color(B(24 * act * fade), 92, 146, 214));

    // ---- Yumusak katmanli golge ----
    {
        const float shOff = 8.0f + hov * 3.0f;
        for (int i = 0; i < 6; i++) {
            GraphicsPath sp;
            float inf = i * 2.2f;
            RoundRectPath(sp, bx - inf, y + shOff - inf + 4.0f, bw + inf * 2.0f, bh + inf * 2.0f, BtnRadius + inf);
            SolidBrush sb(Color(B((16.0f - i * 2.4f) * fade), 0, 0, 0));
            g.FillPath(&sb, &sp);
        }
    }

    GraphicsPath body;
    RoundRectPath(body, bx, y, bw, bh, BtnRadius);

    // ---- Govde ----
    {
        Color top = LerpColor(Color(255, 33, 37, 45), Color(255, 40, 47, 60), act);
        Color bot = LerpColor(Color(255, 21, 24, 30), Color(255, 25, 30, 40), act);
        LinearGradientBrush bodyBrush(PointF(0, y), PointF(0, y + bh), Fade(top, fade), Fade(bot, fade));
        g.FillPath(&bodyBrush, &body);
    }

    // ---- Ic efektler (govdeye kirpilmis) ----
    {
        Region clip(&body);
        g.SetClip(&clip, CombineModeReplace);

        // Soldan gelen vurgu tonu
        if (act > 0.01f) {
            LinearGradientBrush tint(PointF(bx, 0), PointF(bx + bw, 0),
                Color(B(act * 42), 92, 146, 214), Color(0, 92, 146, 214));
            g.FillRectangle(&tint, bx, y, bw, bh);
        }

        // Yumusak isik suzmesi (sadece hover'da)
        if (hov > 0.01f) {
            const float bandW = 110.0f;
            float sx = bx - bandW + fmodf(A.time * 170.0f, bw + bandW * 2.0f);
            LinearGradientBrush band(PointF(sx, 0), PointF(sx + bandW, 0), Color(0, 255, 255, 255), Color(0, 255, 255, 255));
            Color cols[3] = { Color(0, 255, 255, 255), Color(B(hov * 20), 255, 255, 255), Color(0, 255, 255, 255) };
            REAL pos[3] = { 0.0f, 0.5f, 1.0f };
            band.SetInterpolationColors(cols, pos, 3);
            g.FillRectangle(&band, sx, y, bandW, bh);
        }

        // Tiklama dalgalari
        for (auto& r : g_ripples) {
            if (!r.active) continue;
            float rad = EaseOutCubic(r.t) * bw * 0.8f;
            SolidBrush rb(Color(B((1.0f - r.t) * 36), 150, 180, 224));
            g.FillEllipse(&rb, r.x - rad, r.y - rad, rad * 2, rad * 2);
        }

        // Alt vurgu cizgisi: hover'da ortadan genisler, launch'ta ilerleme cubugu olur
        if (launching) {
            float prog = Clamp01((float)(GetTickCount64() - g_launchStart) / (float)LaunchTimeoutMs);
            SolidBrush track(Color(B(40 * fade), 255, 255, 255));
            g.FillRectangle(&track, bx, y + bh - 3.0f, bw, 3.0f);
            SolidBrush prg(Theme::Accent());
            g.FillRectangle(&prg, bx, y + bh - 3.0f, bw * prog, 3.0f);
        }
        else if (hov > 0.01f) {
            float w = bw * EaseOutCubic(hov);
            SolidBrush line(Color(B(255 * hov), 92, 146, 214));
            g.FillRectangle(&line, bx + (bw - w) / 2.0f, y + bh - 2.0f, w, 2.0f);
        }

        g.ResetClip();
    }

    // ---- Ust ic parlama cizgisi (1px, yarim yukseklikte kaybolur) ----
    {
        GraphicsPath inner;
        RoundRectPath(inner, bx + 1.0f, y + 1.0f, bw - 2.0f, bh - 2.0f, BtnRadius - 1.0f);
        LinearGradientBrush hlBrush(PointF(0, y), PointF(0, y + bh), Color(0, 255, 255, 255), Color(0, 255, 255, 255));
        Color cols[3] = { Color(B(44 * fade), 255, 255, 255), Color(0, 255, 255, 255), Color(0, 255, 255, 255) };
        REAL pos[3] = { 0.0f, 0.5f, 1.0f };
        hlBrush.SetInterpolationColors(cols, pos, 3);
        Pen hlPen(&hlBrush, 1.0f);
        g.DrawPath(&hlPen, &inner);
    }

    // ---- Kenarlik ----
    {
        Pen borderPen(Fade(LerpColor(Theme::Border(), Theme::Accent(), act), fade), 1.5f);
        g.DrawPath(&borderPen, &body);
    }

    // ================= ICERIK =================
    SolidBrush whiteBrush(Fade(LerpColor(Theme::TextMain(), Color(255, 255, 255, 255), hov), fade));
    SolidBrush subBrush(Fade(Theme::TextSub(), fade));
    const StringFormat* typo = StringFormat::GenericTypographic();

    // --- V harfi + altinda sag hizali "anta" ---
    {
        RectF vb, ab;
        g.MeasureString(L"V", -1, g_fonts.v.get(), PointF(0, 0), typo, &vb);
        g.MeasureString(L"anta", -1, g_fonts.anta.get(), PointF(0, 0), typo, &ab);

        const float capH = g_fonts.vSize * 0.70f;          // buyuk harf yuksekligi
        const float xH = g_fonts.antaSize * 0.50f;         // kucuk harf yuksekligi
        const float gap = 3.0f;
        const float groupH = capH + gap + xH;
        const float groupTop = y + (bh - groupH) / 2.0f;
        const float baseline = groupTop + capH;

        const float vX = bx + 34.0f + hov * 2.0f;
        const float vY = baseline - g_fonts.vAscent;
        const float antaX = vX + vb.Width - ab.Width;
        const float antaY = baseline + gap - (g_fonts.antaAscent - xH);

        g.DrawString(L"V", -1, g_fonts.v.get(), PointF(vX, vY), typo, &whiteBrush);
        g.DrawString(L"anta", -1, g_fonts.anta.get(), PointF(antaX, antaY), typo, &subBrush);
    }

    // --- Dikey ayirici ---
    {
        Pen div(Color(B(34 * fade), 255, 255, 255), 1.0f);
        g.DrawLine(&div, bx + 100.0f, y + 18.0f, bx + 100.0f, y + bh - 18.0f);
    }

    // --- Ana yazi ---
    {
        StringFormat sf(StringFormat::GenericTypographic());
        sf.SetAlignment(StringAlignmentNear);
        sf.SetLineAlignment(StringAlignmentCenter);
        RectF tr(bx + 122.0f + hov * 3.0f, y - 1.0f, bw - 122.0f - 66.0f, bh);

        std::wstring text = L"LAUNCH AIMLAB";
        if (launching) {
            int dots = ((int)(A.time * 3.0f)) % 4;
            text = L"LAUNCHING" + std::wstring((size_t)dots, L'.');
        }
        g.DrawString(text.c_str(), -1, g_fonts.btn.get(), tr, &sf, &whiteBrush);
    }

    // --- Sag taraf: ok (normal) / donen halka (launch) ---
    {
        const float ax = bx + bw - 44.0f;
        const float ay = y + bh / 2.0f;
        if (!launching) {
            float off = hov * 6.0f + sinf(A.time * 5.0f) * 1.5f * hov;
            Pen arrow(Color(B((100 + hov * 155) * fade), 255, 255, 255), 2.4f);
            arrow.SetStartCap(LineCapRound); arrow.SetEndCap(LineCapRound); arrow.SetLineJoin(LineJoinRound);
            g.DrawLine(&arrow, ax + off - 3.0f, ay - 8.0f, ax + off + 5.0f, ay);
            g.DrawLine(&arrow, ax + off + 5.0f, ay, ax + off - 3.0f, ay + 8.0f);
        }
        else {
            Pen ring(Color(50, 255, 255, 255), 2.4f);
            g.DrawEllipse(&ring, ax - 10.0f, ay - 10.0f, 20.0f, 20.0f);
            Pen spin(Theme::AccentSoft(), 2.4f);
            spin.SetStartCap(LineCapRound); spin.SetEndCap(LineCapRound);
            g.DrawArc(&spin, ax - 10.0f, ay - 10.0f, 20.0f, 20.0f, A.time * 360.0f, 90.0f);
        }
    }

    g.Restore(state);
}

static void DrawFooter(Graphics& g) {
    using namespace Layout;
    const float a = Stage(A.intro, 0.7f, 0.5f);
    SolidBrush subBrush(Fade(Theme::TextSub(), a));
    SolidBrush dimBrush(Fade(Theme::TextDim(), a));

    g.DrawString(L"v0.01", -1, g_fonts.foot.get(), PointF(30, WinH - 40), &subBrush);
    g.DrawString(L"aku", -1, g_fonts.footTiny.get(), PointF(68, WinH - 38), &dimBrush);

    // Nabiz atan durum noktasi
    const float p = fmodf(A.time * 0.7f, 1.0f);
    const float cx = WinW - 118.0f, cy = WinH - 32.0f;
    SolidBrush halo(Color(B((1.0f - p) * 80 * a), 74, 190, 130));
    float hr = 4.5f + p * 9.0f;
    g.FillEllipse(&halo, cx - hr, cy - hr, hr * 2, hr * 2);
    SolidBrush dot(Fade(Theme::Success(), a));
    g.FillEllipse(&dot, cx - 4.5f, cy - 4.5f, 9.0f, 9.0f);
    g.DrawString(L"Secure", -1, g_fonts.foot.get(), PointF(WinW - 104.0f, WinH - 41.0f), &subBrush);
}

static void DrawLoaderUI(HDC hdc) {
    using namespace Layout;
    Graphics g(hdc);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    DrawBackground(g);
    DrawHeader(g);
    DrawCenterText(g);
    DrawButton(g);
    DrawFooter(g);

    // Pencere kenar cizgisi (region yaricapi = 10)
    GraphicsPath border;
    RoundRectPath(border, 0.5f, 0.5f, WinW - 1.0f, WinH - 1.0f, 10.0f);
    Pen bp(Color(60, 110, 130, 165), 1.0f);
    g.DrawPath(&bp, &border);
}

// ============================================================================
//  KAPANIS
// ============================================================================
static void Finalize(HWND hwnd) {
    if (g_finalized) return;
    g_finalized = true;
    KillTimer(hwnd, 1);

    g_running.store(false);
    if (g_worker.joinable()) g_worker.join();
    if (g_discordThread.joinable()) g_discordThread.join();   // core'u kendi thread'i yok eder

    SaveConfig();
    if (g_hwndInGame && IsWindow(g_hwndInGame)) DestroyWindow(g_hwndInGame);
    DestroyWindow(hwnd);
}

// ============================================================================
//  OYUN DURUMU KONTROLU (saniyede bir)
// ============================================================================
// ----------------------------------------------------------------------------
//  IN-GAME MENU: sadece AimLab calisirken ve AimLab penceresinin uzerinde gorunur
// ----------------------------------------------------------------------------
static bool g_igmWanted = false;        // kullanici (veya oyun acilisi) menuyu istiyor mu
static bool g_igmShownByUs = false;     // menuyu biz gosterdik mi
static HWND g_aimlabWnd = nullptr;

struct WndSearch { DWORD pid; HWND best; long long bestArea; };

static BOOL CALLBACK EnumAimlabProc(HWND h, LPARAM lp) {
    auto* s = reinterpret_cast<WndSearch*>(lp);
    DWORD pid = 0;
    GetWindowThreadProcessId(h, &pid);
    if (pid != s->pid || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
    RECT r;
    if (!GetWindowRect(h, &r)) return TRUE;
    const long w = r.right - r.left, hgt = r.bottom - r.top;
    if (w < 200 || hgt < 200) return TRUE;
    const long long area = (long long)w * hgt;
    if (area > s->bestArea) { s->best = h; s->bestArea = area; }
    return TRUE;
}

// AimLab'in ana (en buyuk gorunur) penceresini bulur
static HWND FindAimlabWindow() {
    const DWORD pid = GetProcessIdByName(L"AimLab_tb.exe");
    if (!pid) return nullptr;
    WndSearch s{ pid, nullptr, 0 };
    EnumWindows(EnumAimlabProc, reinterpret_cast<LPARAM>(&s));
    return s.best;
}

static void HideInGameMenu() {
    if (g_hwndInGame && IsWindowVisible(g_hwndInGame)) ShowWindow(g_hwndInGame, SW_HIDE);
    g_igmShownByUs = false;
    g_igmVisible = false;
}

static void UpdateInGameOverlay() {
    if (!g_hwndInGame) return;

    static ULONGLONG lastFind = 0;
    static bool prevInsert = false;
    const ULONGLONG now = GetTickCount64();

    // Oyun kapaliysa menu de kapali
    if (!g_aimlabRunning) {
        g_aimlabWnd = nullptr;
        g_igmWanted = false;
        prevInsert = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
        HideInGameMenu();
        return;
    }

    if (!g_aimlabWnd || !IsWindow(g_aimlabWnd) || now - lastFind > 500) {
        lastFind = now;
        g_aimlabWnd = FindAimlabWindow();
    }

    const HWND fg = GetForegroundWindow();
    const bool aimFocused = g_aimlabWnd && (fg == g_aimlabWnd || fg == g_hwndInGame);

    // INSERT: sadece AimLab onde iken calisir (global kisayol degil, baska uygulamalari etkilemez)
    const bool down = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
    if (down && !prevInsert && aimFocused) g_igmWanted = !g_igmWanted;
    prevInsert = down;

    bool visible = IsWindowVisible(g_hwndInGame) != FALSE;

    // Kullanici menuyu kendi (X / kapat) ile kapattiysa istegi sifirla
    if (!visible && g_igmShownByUs) {
        g_igmWanted = false;
        g_igmShownByUs = false;
    }

    const bool desired = g_igmWanted && aimFocused && !IsIconic(g_aimlabWnd);

    if (desired && !visible) {
        // AimLab penceresinin ortasina yerlestir (pencere disina tasmasin)
        RECT r;
        GetWindowRect(g_aimlabWnd, &r);
        int x = r.left + ((r.right - r.left) - IGM_W) / 2;
        int y = r.top + ((r.bottom - r.top) - IGM_H) / 2;
        x = std::max<int>(r.left, x);
        y = std::max<int>(r.top, y);
        SetWindowPos(g_hwndInGame, HWND_TOPMOST, x, y, IGM_W, IGM_H, SWP_SHOWWINDOW | SWP_NOACTIVATE);
        g_igmShownByUs = true;
    }
    else if (!desired && visible) {
        ShowWindow(g_hwndInGame, SW_HIDE);
        g_igmShownByUs = false;
    }

    g_igmVisible = IsWindowVisible(g_hwndInGame) != FALSE;
}

static void CheckAimlabState(HWND hwnd) {
    static ULONGLONG lastCheck = 0;
    const ULONGLONG now = GetTickCount64();
    if (now - lastCheck < 1000) return;
    lastCheck = now;

    const bool wasRunning = g_aimlabRunning;
    g_aimlabRunning = IsProcessRunning(L"AimLab_tb.exe");

    if (g_aimlabRunning && !wasRunning) {
        ShowWindow(hwnd, SW_HIDE);      // loader gizlenir
        g_igmWanted = true;             // menu, AimLab penceresi hazir olunca onun icinde acilir
        g_isLaunching = false;
    }
    if (!g_aimlabRunning && wasRunning && !g_closing) {
        PostMessage(hwnd, WM_CLOSE, 0, 0);
    }
    if (g_isLaunching && now - g_launchStart > Layout::LaunchTimeoutMs) {
        g_isLaunching = false;
    }
}

// ============================================================================
//  PENCERE PROSEDURU
// ============================================================================
static void OnTimer(HWND hwnd) {
    if (g_finalized) return;

    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    float dt = (float)(now.QuadPart - g_qpcLast.QuadPart) / (float)g_qpcFreq.QuadPart;
    g_qpcLast = now;
    if (dt > 0.05f) dt = 0.05f;
    if (dt < 0.0f) dt = 0.0f;

    if (g_closing) {
        A.closeT += dt / 0.28f;
        if (A.closeT >= 1.0f) { Finalize(hwnd); return; }
    }

    UpdateAnimations(dt);

    // Acilista fade-in, kapanista fade-out
    float alpha = Clamp01(A.intro / 0.45f) * (1.0f - EaseOutCubic(A.closeT));
    SetLayeredWindowAttributes(hwnd, 0, (BYTE)(alpha * 255.0f), LWA_ALPHA);

    CheckAimlabState(hwnd);
    if (g_finalized) return;
    UpdateInGameOverlay();

    if (IsWindowVisible(hwnd)) {
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
    }
}

static LRESULT CALLBACK WndProcLoader(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    using namespace Layout;
    switch (msg) {
    case WM_CREATE: {
        g_hwndLoader = hwnd;
        LoadConfig();
        g_worker = std::thread(TriggerLoop);
        g_discordThread = std::thread(DiscordRPCThread);

        CreateResources(hwnd);

        HRGN hRgn = CreateRoundRectRgn(0, 0, WinW + 1, WinH + 1, 20, 20);
        SetWindowRgn(hwnd, hRgn, TRUE);
        SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);   // fade-in 0'dan baslar

        QueryPerformanceFrequency(&g_qpcFreq);
        QueryPerformanceCounter(&g_qpcLast);
        SetTimer(hwnd, 1, 16, nullptr);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;

    case WM_SETCURSOR: {
        if (LOWORD(lp) == HTCLIENT) {
            POINT p; GetCursorPos(&p); ScreenToClient(hwnd, &p);
            bool hand = (InClose(p.x, p.y) || (InButton(p.x, p.y) && !g_isLaunching)) && !g_mouse.dragging;
            SetCursor(LoadCursor(nullptr, hand ? IDC_HAND : IDC_ARROW));
            return TRUE;
        }
        break;
    }
    case WM_TIMER:
        OnTimer(hwnd);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        if (g_memDC) {
            DrawLoaderUI(g_memDC);
            BitBlt(dc, 0, 0, WinW, WinH, g_memDC, 0, 0, SRCCOPY);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE: {
        g_mouse.x = GET_X_LPARAM(lp);
        g_mouse.y = GET_Y_LPARAM(lp);
        if (!g_mouse.tracking) {
            TRACKMOUSEEVENT tme = { sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            g_mouse.tracking = true;
        }
        if (g_mouse.dragging) {
            POINT p; GetCursorPos(&p);
            SetWindowPos(hwnd, nullptr, p.x - g_mouse.dragStart.x, p.y - g_mouse.dragStart.y,
                0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        return 0;
    }
    case WM_MOUSELEAVE: {
        g_mouse.tracking = false;
        if (!g_mouse.dragging) { g_mouse.x = -1; g_mouse.y = -1; }
        return 0;
    }
    case WM_LBUTTONDOWN: {
        if (g_closing) return 0;
        const int x = GET_X_LPARAM(lp), y = GET_Y_LPARAM(lp);

        if (InClose(x, y)) { PostMessage(hwnd, WM_CLOSE, 0, 0); return 0; }

        if (y < HeaderH) {
            g_mouse.dragging = true;
            SetCapture(hwnd);
            POINT p; GetCursorPos(&p);
            RECT wr; GetWindowRect(hwnd, &wr);
            g_mouse.dragStart.x = p.x - wr.left;
            g_mouse.dragStart.y = p.y - wr.top;
            return 0;
        }

        if (InButton(x, y) && !g_isLaunching) {
            A.click = 1.0f;
            for (auto& r : g_ripples) {
                if (!r.active) { r.active = true; r.x = (float)x; r.y = (float)y; r.t = 0.0f; break; }
            }
            g_isLaunching = true;
            g_launchStart = GetTickCount64();
            StartAimLab();
        }
        return 0;
    }
    case WM_LBUTTONUP: {
        if (g_mouse.dragging) { g_mouse.dragging = false; ReleaseCapture(); }
        return 0;
    }
    case WM_RBUTTONDOWN:
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return 0;

    case WM_CLOSE: {
        if (!g_closing) {
            g_closing = true;
            g_mouse.dragging = false;
            // Pencere gizliyse fade-out gorunmez, dogrudan kapat
            if (!IsWindowVisible(hwnd)) Finalize(hwnd);
        }
        return 0;
    }
    case WM_DESTROY: {
        FreeResources();
        PostQuitMessage(0);
        return 0;
    }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ============================================================================
//  GIRIS NOKTASI
// ============================================================================
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int nShow) {
    using namespace Layout;

    GdiplusStartupInput gdiplusStartupInput;
    if (GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr) != Ok) return 1;

    WNDCLASSW wcLoader = {};
    wcLoader.lpfnWndProc = WndProcLoader;
    wcLoader.hInstance = hInst;
    wcLoader.lpszClassName = L"VantaLoaderWnd";
    wcLoader.hCursor = LoadCursor(nullptr, IDC_ARROW);

    WNDCLASSW wcGame = {};
    wcGame.lpfnWndProc = WndProcInGame;
    wcGame.hInstance = hInst;
    wcGame.lpszClassName = L"VantaInGameWnd";
    wcGame.hCursor = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&wcLoader) || !RegisterClassW(&wcGame)) {
        GdiplusShutdown(g_gdiplusToken);
        return 1;
    }

    // Oyun ici pencere once olusturulur (loader zamanlayicisi onu kullanir), gizli baslar
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"VantaInGameWnd", L"Vanta Overlay",
        WS_POPUP, (screenW - IGM_W) / 2, (screenH - IGM_H) / 2, IGM_W, IGM_H,
        nullptr, nullptr, hInst, nullptr);

    HWND hLoader = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED, L"VantaLoaderWnd", L"Vanta Loader",
        WS_POPUP | WS_VISIBLE, 200, 200, WinW, WinH,
        nullptr, nullptr, hInst, nullptr);
    if (!hLoader) {
        GdiplusShutdown(g_gdiplusToken);
        return 1;
    }
    ShowWindow(hLoader, nShow);

    MSG m;
    while (GetMessageW(&m, nullptr, 0, 0) > 0) {
        TranslateMessage(&m);
        DispatchMessageW(&m);
    }

    GdiplusShutdown(g_gdiplusToken);
    return 0;
}