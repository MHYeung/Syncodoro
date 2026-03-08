# Syncodoro

A standalone **Pomodoro focus timer** built on the ESP32-2432S028R ("Cheap Yellow Display") — a 2.8-inch resistive touchscreen embedded device that counts down focus sessions, logs them to a microSD card, and serves a live web dashboard over your local WiFi network.

---

## Features

| Feature | Details |
|---|---|
| **Touchscreen UI** | LVGL-powered dark interface with circular arc progress, large countdown display, and tap-to-start/pause gestures |
| **Focus sessions** | Configurable duration (5–120 min in 5-min steps) and class tag (Coding, Reading, Writing, Design, Gaming, Studying) |
| **Session logging** | Completed sessions saved to `/sdcard/sessions.csv` for permanent history |
| **On-device history** | Scrollable session log screen showing tag, duration, and timestamp |
| **WiFi setup portal** | First-boot AP captive portal — connect your phone, enter home WiFi credentials, device reboots into STA mode |
| **Web dashboard** | Served from firmware flash over LAN: session history table, JSON API, and one-click CSV download |
| **SD config** | `/sdcard/config.json` persists your preferred duration and tag across reboots |
| **Dual-boot path** | NVS `setup_mode` flag cleanly separates first-boot setup from normal timer operation |

---

## Hardware

**Target board:** [ESP32-2432S028R](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display) — commonly called the "CYD" (Cheap Yellow Display)

```
┌──────────────────────────────┐
│  ESP32-2432S028R             │
│                              │
│  LCD   ST7789  2.8" 240×320  │
│  Touch XPT2046  (resistive)  │
│  Flash 4 MB                  │
│  SD    microSD (optional)    │
└──────────────────────────────┘
```

### Pin Map

| Peripheral | Signal | GPIO |
|---|---|---|
| **LCD (SPI2)** | MOSI | 13 |
| | MISO | 12 |
| | SCLK | 14 |
| | CS | 15 |
| | DC | 2 |
| | Backlight | 21 |
| **Touch (SPI3)** | MOSI | 32 |
| | MISO | 39 |
| | SCLK | 25 |
| | CS | 33 |
| **SD (shared SPI2)** | CS | 4 |

---

## Architecture

### Boot Flow

```
Power on
   │
   ├─ nvs_flash_init()
   ├─ cyd_init()  (LCD + Touch + LVGL)
   │
   ├─ Read NVS: setup_mode?
   │
   ├── YES (first boot / reset) ──────────────────────────┐
   │    WiFi AP "Syncodoro-Setup"                         │
   │    HTTP captive portal at 192.168.4.1                │
   │    User enters SSID + password in browser            │
   │    Credentials saved to NVS → reboot                 │
   │                                                      │
   └── NO (normal boot) ───────────────────────────────────┘
        Mount SD card (optional, non-fatal)
        Load /sdcard/config.json → apply duration + tag
        WiFi STA connect (non-blocking)
        Show Timer UI
        On IP assigned → start web dashboard server
```

### Screen Navigation

```
Timer Screen
  ├─ [⚙] Settings Screen
  │        ├─ WiFi status + Connect button
  │        ├─ [Timer] → Timer Screen
  │        └─ [History] → History Screen
  └─ [≡] History Screen
           └─ [Back] → Timer Screen
```

### Project Structure

```
syncodoro/
├── partitions.csv          # Custom 4 MB flash layout
├── sdkconfig.defaults      # Flash size, partition table, LVGL fonts, HTTP buffer
└── main/
    ├── main.c              # Boot entry — NVS read, BSP init, mode branch
    ├── bsp/
    │   ├── cyd.c           # ST7789 LCD, XPT2046 touch, SD card init
    │   └── cyd.h
    ├── model/
    │   ├── pomo_model.c    # Global active_session (single source of truth)
    │   └── pomo_model.h    # pomoTimer_t struct + pomotimerState_t enum
    ├── config/
    │   ├── nvs_config.c    # NVS read/write: setup_mode, wifi_ssid, wifi_pass
    │   └── nvs_config.h
    ├── network/
    │   ├── wifi_manager.c  # AP/STA state machine, 5-retry policy, IP callback
    │   ├── wifi_manager.h
    │   ├── http_server.c   # Captive portal (setup) + dashboard (normal mode)
    │   └── http_server.h
    ├── storage/
    │   ├── session_csv.c   # Append/read /sdcard/sessions.csv
    │   ├── session_csv.h
    │   ├── config_json.c   # Read/write /sdcard/config.json
    │   └── config_json.h
    └── ui/
        ├── ui_theme.h      # Overtec dark theme — LVGL color constants
        ├── ui_timer.c/h    # Main timer screen (arc, countdown, modals, nav icons)
        ├── ui_setup.c/h    # First-boot AP instructions screen
        ├── ui_settings.c/h # WiFi status + navigation hub
        └── ui_history.c/h  # Scrollable session list from SD CSV
```

---

## Data Model

```c
typedef struct {
    char             timerID[32];      // Unique session identifier
    int              focusDuration;    // Target duration in minutes
    int              remaining_secs;   // Live countdown (persists across screen nav)
    double           pomoCount;        // Accumulated completed sessions
    char             classTag[32];     // "Coding", "Reading", etc.
    char             dateAndTime[32];  // Timestamp string
    bool             isCompleted;      // True when timer reached zero
    pomotimerState_t timerState;       // IDLE → COUNTING → STOP → COMPLETED
} pomoTimer_t;
```

A single global `active_session` is the shared state across all UI screens, storage modules, and the HTTP API — no dynamic allocation required.

### Session CSV Format

```
timerID,focusDuration,pomoCount,classTag,dateAndTime,isCompleted
s1709123456,25,1.0,Coding,1709123456s,1
s1709127890,20,2.0,Reading,1709127890s,1
```

---

## Web Dashboard

Once the device is connected to your WiFi, open `http://<device-ip>` in any browser on the same network.

| Endpoint | Method | Description |
|---|---|---|
| `/` | GET | Session history dashboard (HTML, embedded in firmware) |
| `/api/sessions` | GET | All sessions as a JSON array (streamed from SD CSV) |
| `/download` | GET | Download `sessions.csv` directly as a file attachment |

> **No filesystem component needed** — all HTML, CSS, and JavaScript is embedded as C string literals in flash.

---

## WiFi Setup (First Boot)

1. Power on the device — it starts in AP mode.
2. The screen shows setup instructions.
3. On your phone or PC, connect to the WiFi network **`Syncodoro-Setup`** (open, no password).
4. Open a browser and go to **`http://192.168.4.1`**.
5. Enter your home WiFi SSID and password, then tap **Save & Connect**.
6. The device saves credentials to NVS, shows a confirmation, and **reboots automatically** after 2 seconds.
7. On the next boot, it connects to your WiFi in STA mode and the web dashboard becomes available.

To **reset WiFi credentials**, use `idf.py erase-flash` or add an NVS erase mechanism.

---

## Building & Flashing

### Requirements

- [ESP-IDF v5.4.x](https://docs.espressif.com/projects/esp-idf/en/v5.4/esp32/get-started/)
- Python 3.x
- USB cable connected to the ESP32-2432S028R

### Build

```bash
# Clone and enter the project
cd syncodoro

# Set up IDF environment (Windows PowerShell)
. $env:IDF_PATH\export.ps1

# Build
idf.py build
```

### Flash

```bash
idf.py -p COM<PORT> flash monitor
```

Replace `COM<PORT>` with your serial port (e.g. `COM3` on Windows, `/dev/ttyUSB0` on Linux).

### First-time Flash Size Note

The project uses a **custom 4 MB partition table**. If flashing over a previous project, run a full erase first:

```bash
idf.py -p COM<PORT> erase-flash
idf.py -p COM<PORT> flash
```

---

## Flash Layout

```
0x00001000  Bootloader        (from ESP-IDF)
0x00008000  Partition table   (partitions.csv)
0x00009000  NVS               24 KB   — WiFi credentials, setup_mode flag
0x0000F000  PHY init data     4 KB
0x00010000  App (factory)     1.94 MB — firmware binary (~1.35 MB used, 33% free)
0x00200000  Storage (SPIFFS)  2 MB    — reserved (SD card used instead for now)
```

---

## Dependencies

| Component | Version | Purpose |
|---|---|---|
| `esp-idf` | `>=5.4` | Core framework, FreeRTOS, WiFi, NVS, HTTP server |
| `espressif/esp_lvgl_port` | `^2.7.1` | LVGL ↔ ESP-IDF display/touch integration |
| `atanisoft/esp_lcd_touch_xpt2046` | `^1.0.6` | XPT2046 resistive touch driver |
| `lvgl/lvgl` | `9.5.0` | UI framework (pulled in by esp_lvgl_port) |

---

## Roadmap

- [ ] RTC integration (DS3231) for accurate session timestamps
- [ ] Notion API sync — post sessions to a Notion database
- [ ] Color theme selection in Settings UI
- [ ] Break timer mode (short / long break)
- [ ] Daily session goal and streak tracking
- [ ] OTA firmware update via web dashboard

---

## License

MIT License — see [LICENSE](LICENSE) for details.
