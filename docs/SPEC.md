# ESP32-S3 e-Paper Recorder — Spec v0.1

Target board: Waveshare ESP32-S3-Touch-ePaper-1.54 (touch version, with battery)
Framework: ESP-IDF >= 5.5.0 (required by Waveshare for this board)

---

## 1. Feasibility verdict

**Everything requested is feasible on this hardware.** Every peripheral you need is on the board and Waveshare ships a working example for each one. The work is integration, not research.

| Feature | Feasible | Basis | Main risk |
|---|---|---|---|
| Home: time | Yes | PCF85063 RTC, example `02_I2C_PCF85063` | RTC drift; needs a Set Time UI or NTP |
| Home: temp / humidity | Yes | SHTC3, example `03_I2C_STHC3` | Board self-heating skews readings |
| Touch navigation | Yes | FT6336, example `13_FT6336_Test` | 0.3 s partial-refresh latency; small hit targets |
| Record to SD | Yes | ES8311 codec + onboard mic + SDMMC | Must not stall audio while e-paper refreshes |
| Record icon <-> stop icon | Yes | Partial refresh (~0.3 s) | None |
| File list | Yes | FatFs on SDMMC, example `04_SD_Card` | 200x200 px: paging needed, FAT32 only |
| WAV play / stop | Yes | ES8311 + onboard speaker, example `08_Audio_Test` | Only PCM WAV supported by default |
| Settings (time etc.) | Yes | RTC, NVS, ADC | Entering digits by touch on 200x200 |
| Battery operation | Yes | Charger onboard, ADC battery read | Touch-wake from deep sleep is **unverified** |

Things that are *not* realistic on this panel: live timers ticking every second, level meters, animation, scrolling. The panel is 1-bit black/white; refresh is 2 s (full) or 0.3 s (partial). The UI has to be static and event-driven.

---

## 2. Hardware (from Waveshare docs)

| Item | Detail |
|---|---|
| SoC | ESP32-S3-PICO-1-N8R8: dual-core 240 MHz, 8 MB flash, 8 MB PSRAM (V2 board) |
| Display | 1.54" e-paper, 200x200, black/white (2 grey levels), SPI, full refresh 2 s, partial 0.3 s |
| Touch | FT6336 capacitive, I2C (touch version only) |
| Audio | ES8311 codec, onboard microphone, onboard speaker, plus MX1.25 speaker header |
| Storage | TF slot, **FAT32 required**, driven in 1-line SDMMC mode in Waveshare examples |
| RTC | PCF85063 (back of board), alarm wake supported (`12_RTC_Sleep_Test`) |
| Sensor | SHTC3 temperature + humidity (I2C) |
| Power | 3.7 V Li-ion via MX1.25, onboard charger, battery voltage on ADC1 CH3, PWR + BOOT buttons |
| Power rails | Factory firmware switches EPD, audio and VBAT rails in software (`BoardPower_EPD_ON`, `BoardPower_Audio_ON`, `BoardPower_VBAT_ON`) |

Check the V2 label: Waveshare says examples are not interchangeable between V1 and V2. The touch version has only one hardware revision, but confirm what is printed on your board.

### 2.1 Verified on hardware (this unit)

Read off the board over USB-Serial/JTAG with `esptool flash_id`, `espefuse summary` and an `idf.py monitor` boot capture.

| Item | Reported |
|---|---|
| Chip | ESP32-S3-PICO-1 (LGA56), revision v0.2, eFuse block rev v1.3 |
| Cores | dual-core LX7 + LP core, rated 240 MHz (app runs at 160 MHz) |
| Package | LGA56 (`PKG_VERSION` = 1) |
| Crystal | 40 MHz |
| Flash | 8 MB GigaDevice (GD), quad / 4 data lines, 3.3 V — `FLASH_CAP` 8M |
| PSRAM | 8 MB AP Memory (AP_3v3), **octal (OPI)**, 80 MHz, 64 Mbit — `PSRAM_CAP` 8M |
| VDD_SPI | forced 3.3 V (`VDD_SPI_FORCE` + `VDD_SPI_TIEH`) |
| MAC | 70:04:1d:d7:ef:9c |
| Security | flash encryption off, secure boot off, USB-Serial/JTAG and download mode enabled |
| Onboard I2C | SHTC3 responds (`ID:0887`) |

Runtime confirms both memories: `esp_psram: Found 8MB PSRAM device` (octal, 80 MHz, `SPI SRAM memory test OK`), `spi_flash: detected chip: gd`, `flash io: qio`, `SPI Mode: QIO @ 80 MHz`.

The stock firmware on the unit was built for 4 MB and warns at boot: `Detected size(8192k) larger than the size in the binary image header(4096k)`. Build for **8 MB** (see 2.2).

### 2.2 Memory budget

Board memory (ESP32-S3-PICO-1-N8R8), from the Waveshare/Espressif figures:

| Memory | Size | Notes |
|---|---|---|
| SRAM (internal) | 512 KB | Static app regions are DIRAM + IRAM below; the rest is ROM, I-cache (16 KB) and D-cache (32 KB) |
| RTC SRAM | 16 KB | 8 KB RTC SLOW + 8 KB RTC FAST, retained in deep sleep |
| ROM | 384 KB | Espressif ROM, not writable by the app |
| Flash (in-package) | 8 MB | App lives in the `factory` partition, 0x3F0000 (3.94 MiB) |
| PSRAM (in-package) | 8 MB | Octal (OPI) at 80 MHz; not reported by `idf.py size` |

Current firmware, measured with `idf.py size` (ESP-IDF v6.1, target `esp32s3`), bytes:

| Region | Used | Total | Free | Sections |
|---|---|---|---|---|
| Flash Code | 354,462 | — | — | `.text` |
| Flash Data | 312,300 | — | — | `.rodata` 312,028, `.appdesc` 256, `.tdata` 16 |
| DIRAM | 136,734 (40.0%) | 341,760 | 205,026 | `.bss` 69,312, `.text` 52,339, `.data` 15,083 |
| IRAM | 16,384 (100%) | 16,384 | 0 | `.text` 15,356, `.vectors` 1,028 |
| RTC SLOW | 36 (0.4%) | 8,192 | 8,156 | `.force_slow` |
| RTC FAST | 24 (0.3%) | 8,192 | 8,168 | `.rtc_reserved` |
| Image | 750,604 | — | — | `.bin`, pads larger |

How to read it:

- **DIRAM** is the shared D/IRAM region (0x3FC88000 to 0x3FCDB700). It holds RAM data (`.data`, `.bss`) and IRAM code that overflows the dedicated IRAM window. ~200 KB is free for static allocation; the remaining internal SRAM is the runtime heap.
- **IRAM** here is only the 16 KB non-shared window (0x40374000 to 0x40378000) and it is 100% full. The vectors plus current IRAM-resident code fill it, so any new `IRAM_ATTR` code (audio/touch ISRs, section 6) lands in DIRAM `.text` and consumes the ~200 KB above.
- **Flash Code / Data** are the flash-mapped instruction and constant windows, not physical flash. The real budget is the `factory` app partition: 0x3F0000 = 4,128,768 B, of which the 750,604 B image uses 18%, leaving 3,378,164 B (82%).
- **PSRAM** is where the spec puts the large buffers (LVGL full frame buffer ~80 KB, audio ring buffer ~64 KB; sections 6 and 7). It is fixed at 8 MB at runtime and is not reported by the size tool.

Flash size: originally built for 4 MB (`CONFIG_ESPTOOLPY_FLASHSIZE_4MB`); corrected to **8 MB** to match `FLASH_CAP` (2.1). The `factory` app partition is 0x3F0000 (4 MB), so the upper 4 MB of flash is currently unallocated.

---

## 3. Software stack

- ESP-IDF 5.5.x, C/C++ (Waveshare's drivers are C++ classes such as `I2cMasterBus`, `I2cFt6336Dev`).
- LVGL **8.3.11** (used by Waveshare examples 08, 09, 11, 12; v9.3.0 also exists in `10_LVGL_V9_Test`). Choose v8.3.11 to reuse their glue code.
- Start the project from `11_FactoryProgram`: it already initializes power rails, I2C, touch, RTC, SD, ADC, SHTC3, buttons and codec in one place.
- SensorLib (PCF85063), FatFs (ESP-IDF built-in), NVS for settings.
- Enable FatFs long filenames (`CONFIG_FATFS_LFN_HEAP`).

---

## 4. Navigation model

Pages in linear order: **Home (0) -> Record (1) -> Files (2) -> Settings (3)**.

Fixed 4-slot bottom bar, 200 px wide, 48 px tall (y = 152..199). Slots never move, absent icons leave a blank slot so touch muscle memory stays consistent.

| Page | Slot 1 Home | Slot 2 Gear | Slot 3 Back `<-` | Slot 4 Next `->` |
|---|---|---|---|---|
| Home | blank | Settings | blank | Record |
| Record | Home | Settings | Home (prev) | Files |
| Files | Home | Settings | Record (prev) | Settings (next) |
| Settings | Home | blank | Files (prev) | Home (wraps) |

Decided: `->` on Settings wraps to Home; `<-` always means the previous page in linear order (not browser-style history), so `<-` from Settings goes to Files even if Settings was opened via the gear.

Hit boxes: each slot is 50x48 px. The panel is roughly 28 mm wide (estimate from standard 1.54" panels; verify against the mechanical drawing), so a slot is ~7 mm, which is small but usable. Use tap only. No swipes.

---

## 5. Screens (200x200, content area 200x152)

### 5.1 Home
- Time `HH:MM` large (centre-top), date small beneath.
- Temperature and humidity on one row.
- Battery % small, top-right.
- Refresh: minute boundary only, partial refresh. Sensor read at the same time.

### 5.2 Record
- Centre: large record symbol (filled circle) when `isRecording == false`; stop symbol (filled square) when `true`.
- Small status line: `Ready` / `REC` + current filename.
- Tap symbol toggles. State is held in firmware, the screen only reflects it.
- No per-second elapsed timer by default (see 9). Optional setting: update elapsed time every 10 s.
- Errors shown in the status line: `No SD`, `SD full`, `Battery low`.

### 5.3 Files
Two sub-views.

**List view**
- Header row: `Files  1/3` (page of list).
- 5 rows x ~26 px, filename left, size right, truncated with ellipsis.
- Scroll with ▲ / ▼ buttons on the right edge of the content area. The bottom-bar `<-`/`->` are page navigation, so they cannot double as list paging.
- `.wav` rows tappable; other files drawn greyed and not tappable.

**Player view** (after tapping a `.wav`)
- Filename, duration (from header), play/stop button (centre), `List` button to return.
- Leaving the view (any nav icon) stops playback.
- Bottom bar remains per the table in section 4.

### 5.4 Settings
Scrollable list, same ▲ / ▼ buttons. Tapping an item opens an inline editor with `+` / `-` / `OK`.

| Setting | Notes |
|---|---|
| Set time and date | `+`/`-` per field; writes PCF85063 |
| Sync time over Wi-Fi (NTP) | Optional; Wi-Fi on only during sync |
| Timezone | Offset in hours (e.g. UTC+7); RTC stores local time |
| 12h / 24h | |
| Temperature unit | C / F |
| Temperature offset | Calibrate for board self-heating |
| Full-refresh interval | Every N partial refreshes (ghost clearing) |
| Record sample rate | Default 16 kHz mono; others after testing |
| Mic gain | ES8311 ADC gain |
| Speaker volume | |
| Auto-sleep timeout | Idle time before sleeping |
| SD card info | Capacity, free space (read only) |
| Battery | Voltage and % (read only) |
| About | Firmware version, hardware version |

Not recommended in v1: "Format SD card" (destructive, easy to mis-tap on a 200 px screen).

---

## 6. Architecture

### Tasks (suggested)
| Task | Core | Priority | Job |
|---|---|---|---|
| `audio_rec` | 1 | high | I2S read -> ring buffer |
| `sd_writer` | 1 | high | Ring buffer -> WAV file, periodic sync |
| `audio_play` | 1 | high | WAV -> I2S |
| `ui` | 0 | medium | LVGL tick + e-paper flush (blocking) |
| `input` | 0 | medium | Touch interrupt -> events |
| `sensor` | 0 | low | SHTC3 + RTC + battery sampling |

Audio and SD work is pinned away from the UI task because an e-paper refresh blocks for 0.3 to 2 s.

### State
- Global app state machine: `IDLE`, `RECORDING`, `PLAYING`, `SLEEPING`.
- `isRecording` derived from state, not a separate flag.
- Interlock: one codec, so **no playback while recording and no recording while playing**. A tap on the disabled action is ignored with a status message.
- UI communicates with services via an event queue (`EVT_TOUCH`, `EVT_REC_START`, `EVT_REC_STOP`, `EVT_PLAY_DONE`, `EVT_SD_ERROR`, `EVT_MINUTE_TICK`).

### Modules
`board_power`, `epd_display`, `touch`, `rtc`, `sensors`, `battery`, `audio_codec`, `wav`, `storage`, `settings` (NVS), `ui_pages`, `app_state`.

---

## 7. Recording and playback

### Recording
- Format: **WAV, PCM 16-bit, mono, 16 kHz** (32 KB/s default). This is far below what 1-line SDMMC can sustain.
- Streamed to the SD card, never buffered whole in RAM. This satisfies "saved immediately".
- Ring buffer in PSRAM, ~64 KB (about 2 s at default rate) between `audio_rec` and `sd_writer`.
- On start: create `REC_YYYYMMDD_HHMMSS.wav` (RTC time). If RTC is invalid, fall back to `REC_0001.wav`, incrementing.
- Write a 44-byte WAV header with placeholder sizes immediately.
- Every ~2 s: flush, patch the header sizes, `fsync`. After a power cut the file is playable up to the last sync.
- On stop: final flush, final header patch, close.
- Auto-stop on: SD error, disk full, battery below threshold. Always finalize the file before stopping.

### Playback
- Parse RIFF header. Accept PCM 16-bit, mono or stereo, rates the codec supports (8 to 48 kHz). Otherwise show `Unsupported`.
- Stream in 4 to 8 KB chunks SD -> I2S -> ES8311 -> speaker.
- Enable audio power rail only while recording/playing.

---

## 8. Storage rules
- Mount FAT32 at `/sdcard`. If mount fails: show `No SD` on Record and Files, never crash.
- Hold a mutex around file operations between `sd_writer` and the Files page.
- Do not list or open the active recording file from the Files page while recording.

---

## 9. Display refresh policy
- In-page changes (icon toggle, value update): **partial refresh**.
- Page change: partial refresh by default; configurable.
- Full refresh every N partial refreshes (default proposal: 10, tunable in Settings) to clear ghosting.
- Home updates once per minute. Nothing updates every second.
- E-paper retains its image with no power, so a static screen costs nothing while asleep.
- Keep total refresh count modest. E-paper panels have limited write cycles.

---

## 10. Power
- Waveshare FAQ figures: ~1 mA board current in low-power mode; ~10 days continuous low-power, ~4 days if waking every 60 s; ~44 minutes to full charge.
- Idle on Home: draw the screen, enter sleep, wake on RTC alarm each minute, refresh, sleep.
- Recording and playback keep the system awake.
- Wake sources: RTC alarm (demonstrated by Waveshare), PWR button. **Touch wake from deep sleep is unverified**: it depends on the FT6336 INT pin being an RTC-capable GPIO. Check the schematic. If not, the user presses PWR to wake.
- Battery low (<10%): stop recording cleanly, show warning.

---

## 11. Risks and open items

| # | Item | Status |
|---|---|---|
| 1 | Pin map, EPD driver details, ES8311 init sequence | **Not verified.** GitHub blocked automated reading of the repo source. Read the schematic PDF and `11_FactoryProgram` / `08_Audio_Test` directly. |
| 2 | Does the IDF EPD driver in the examples expose partial refresh? | Panel supports it (0.3 s). Driver API **unverified**. |
| 3 | Mic count / audio quality | Docs mention one "Microphone"; assume mono. Quality unmeasured. |
| 4 | E-paper refresh noise or power spikes affecting recording | **Unverified**. Test by recording during a refresh. |
| 5 | Touch wake from sleep | Unverified (section 10). |
| 6 | Sustained SD write behaviour on cheap cards | Rate is low, but card latency spikes happen; the 2 s ring buffer is the mitigation. Test with the actual card. |
| 7 | Nav semantics | Resolved, see decisions. |

### Decisions (confirmed)
1. `->` on Settings wraps to Home.
2. `<-` is "previous page in order".
3. Recording is mono 16 kHz, 16-bit PCM (voice quality). Higher rates are out of scope for v1.

### Open questions
1. Should the Home screen keep running on a schedule (wake every minute), or sleep until touched?

---

## 12. Milestones
1. **M0 Bring-up:** flash `11_FactoryProgram`; confirm every peripheral works on your unit.
2. **M1 UI shell:** LVGL on EPD, touch, 4-page navigation with the bottom bar.
3. **M2 Home:** RTC + SHTC3 + minute refresh.
4. **M3 Record:** WAV streaming to SD, crash-safe header, stop/start.
5. **M4 Files + playback.**
6. **M5 Settings + NVS.**
7. **M6 Power:** sleep/wake, battery handling, soak tests (long recording, SD removal, power cut).

---

## References
- Product page: https://www.waveshare.com/esp32-s3-epaper-1.54.htm
- ESP-IDF guide: https://docs.waveshare.com/ESP32-S3-ePaper-1.54/ESP-IDF
- FAQ: https://docs.waveshare.com/ESP32-S3-ePaper-1.54/FAQ
- Resources (schematic, datasheets): https://docs.waveshare.com/ESP32-S3-ePaper-1.54/Resources-And-Documents
- Examples: https://github.com/waveshareteam/ESP32-S3-ePaper-1.54
