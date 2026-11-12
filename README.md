<div align="center">

<img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=12,20,24,30&height=240&section=header&text=VANTAI&fontSize=90&fontColor=ffffff&animation=fadeIn&fontAlignY=38&desc=Aim%20Lab%20Trigger%20Bot&descSize=24&descAlignY=60" width="100%" alt="VANTAI banner" />

<a href="https://github.com/9xaku/vantai">
  <img src="https://readme-typing-svg.demolab.com?font=Fira+Code&weight=600&size=22&duration=3000&pause=900&color=A78BFA&center=true&vCenter=true&width=640&height=50&lines=Pixel-based+trigger+bot+for+Aim+Lab;Written+in+modern+C%2B%2B+for+Windows;Live+in-game+menu+%7C+Persistent+config;Fast.+Lightweight.+Configurable." alt="Typing animation" />
</a>

<br/>

<a href="https://discord.gg/hgAvUUY6KP">
  <img src="https://img.shields.io/badge/JOIN%20THE%20DISCORD-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Join Discord" height="42" />
</a>

<br/><br/>

<img src="https://img.shields.io/badge/language-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white" alt="C++" />
<img src="https://img.shields.io/badge/platform-Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white" alt="Windows" />
<img src="https://img.shields.io/badge/IDE-Visual%20Studio-5C2D91?style=for-the-badge&logo=visualstudio&logoColor=white" alt="Visual Studio" />
<img src="https://img.shields.io/github/license/9xaku/vantai?style=for-the-badge&color=22c55e" alt="MIT License" />

<br/>

<img src="https://img.shields.io/github/stars/9xaku/vantai?style=flat-square&logo=github&color=facc15" alt="Stars" />
<img src="https://img.shields.io/github/forks/9xaku/vantai?style=flat-square&logo=github&color=38bdf8" alt="Forks" />
<img src="https://img.shields.io/github/issues/9xaku/vantai?style=flat-square&color=f472b6" alt="Issues" />
<img src="https://img.shields.io/github/last-commit/9xaku/vantai?style=flat-square&color=a78bfa" alt="Last commit" />
<img src="https://komarev.com/ghpvc/?username=9xaku-vantai&label=Views&color=8b5cf6&style=flat-square" alt="Views" />

</div>

<br/>

---

## ✦ About

**vantai** is a lightweight, pixel-based trigger bot for **Aim Lab**, written in C++ with the Win32 API. It watches a single screen pixel, and when the color at that spot matches your configured target color (within a tolerance you control), it fires a click after a short, adjustable delay.

Everything is tuned live from an **in-game menu**, and your settings are saved automatically between sessions.


## ✦ Usage

**Aim:**
<br> Trigger - disable/enable  <br>
Tolarance - 20/25/30 range stable <br>
Delay - 0 :)
_______________________________________________________________________

**Settings:**
hoykeys settings- <br>
Trigger mode - hold/toggle <br> 
hotkeys- menu open/close <br>

_______________________________________________________________________

**Discord:**
discord rpc Features - playing vanta.ai

_______________________________________________________________________

**System:**
live performance-cpu/gpu/ram <br>
vgk controller <br> 

_______________________________________________________________________

**Misc:** <br>
Watermark - soon too <br> 
starting windows... <br>

---

## ✦ Features

| | Feature | Description |
|:-:|---|---|
| 🎯 | **Pixel-color trigger** | Samples a screen coordinate and compares it against a target RGB color |
| 🎚️ | **Adjustable tolerance** | Control how strict the color match is (default `30`) |
| ⏱️ | **Click delay** | Set the delay between detection and click in milliseconds (default `10 ms`) |
| 📍 | **Custom sample point** | Choose the exact `X` / `Y` pixel to watch (default `960, 540`, center of a 1920×1080 screen) |
| 🖥️ | **In-game menu** | Tabbed overlay menu, toggle its visibility on the fly, window position is remembered |
| 💾 | **Persistent config** | Settings are loaded from and saved to `config.ini` automatically |
| 📝 | **Logging** | Timestamped activity log written to `akubot.log` |
| 💬 | **Discord SDK** | Ships with `discord_game_sdk.dll` for Discord integration |
| 🧵 | **Thread-safe core** | Atomic flags and mutex-guarded logging for stable multi-threaded operation |

---

## ✦ Project Structure

```text
vantai/
├── main.cpp                 # Entry point
├── Trigger.h                # Trigger / detection logic
├── InGameMenu.h             # In-game menu overlay
├── Config.h                 # Settings, config load/save, logging
├── config.ini               # Saved settings
├── akubot.log               # Runtime log
├── discord_game_sdk.dll     # Discord Game SDK
├── vantaai.slnx             # Visual Studio solution
├── vantaai.vcxproj          # Visual Studio project
└── LICENSE                  # MIT
```

---

## ✦ Getting Started

### Requirements

- Windows 10 / 11
- Visual Studio 2022 or newer with the **Desktop development with C++** workload

### Build

```bash
git clone https://github.com/9xaku/vantai.git
cd vantai
```

1. Open `vantaai.slnx` in Visual Studio.
2. Select the **Release | x64** configuration.
3. Press **Ctrl + Shift + B** to build.
4. Keep `discord_game_sdk.dll` next to the compiled `.exe`.

---

## ✦ Configuration

All values live in `config.ini` and can also be changed from the in-game menu.

| Key | Default | Description |
|---|:-:|---|
| `triggerEnabled` | `0` | Trigger on/off (`1` = on) |
| `tolerance` | `30` | Allowed color difference when matching the target |
| `clickDelayMs` | `10` | Delay before the click, in milliseconds |
| `sampleX` | `960` | X coordinate of the watched pixel |
| `sampleY` | `540` | Y coordinate of the watched pixel |
| `targetR` / `targetG` / `targetB` | `0` | Target color (RGB, 0–255) |
| `windowX` / `windowY` | `200` | Menu window position |
| `activeTab` | `0` | Last opened menu tab |

---

## ✦ Community

<div align="center">

Questions, feedback, or want to hang out? Come say hi.

<a href="https://discord.gg/hgAvUUY6KP">
  <img src="https://img.shields.io/badge/Discord-Join%20the%20server-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Join the Discord" />
</a>

**Contact:** `neverlose.acc` on Discord

</div>

---

## ✦ Disclaimer

This project is provided for **educational and research purposes**. Using automation tools in games or apps may violate their Terms of Service and can affect leaderboard integrity or result in account restrictions. You use this software entirely at your own risk, and the author takes no responsibility for any consequences.

---

## ✦ License

Released under the [MIT License](LICENSE).

---

<div align="center">

<a href="https://star-history.com/#9xaku/vantai&Date">
  <img src="https://api.star-history.com/svg?repos=9xaku/vantai&type=Date&theme=dark" alt="Star History" width="560" />
</a>

<br/>

<sub>If you like the project, drop a ⭐ on the repo.</sub>

<img src="https://capsule-render.vercel.app/api?type=waving&color=gradient&customColorList=12,20,24,30&height=140&section=footer" width="100%" alt="footer" />

</div>
