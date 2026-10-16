# ESP32-S3 e-Paper Recorder

Voice-recorder firmware for the **Waveshare ESP32-S3-Touch-ePaper-1.54** (V2) board.
Static, event-driven 200x200 1-bit e-paper UI with touch navigation, WAV recording to
microSD, WAV playback, RTC clock, temperature/humidity, and battery monitoring.

Full design notes live in [`docs/SPEC.md`](docs/SPEC.md).

- SoC: ESP32-S3-PICO-1-N8R8 — dual-core 240 MHz, 8 MB flash, 8 MB octal PSRAM
- Framework: ESP-IDF >= 5.5 (developed against **v6.1**), target `esp32s3`
- UI: LVGL 9 (component-managed)

---

## Hardware

| Item | Detail |
|---|---|
| Display | 1.54" e-paper, 200x200, black/white, SPI — full refresh ~2 s, partial ~0.3 s |
| Touch | FT6336 capacitive, I2C (0x38), polled |
| Audio | ES8311 codec, onboard mic + speaker (I2S) |
| Storage | microSD via 1-line SDMMC, **FAT32 required** |
| RTC | PCF85063A (I2C 0x51) |
| Sensor | SHTC3 temp/humidity (I2C 0x70) |
| Power | 3.7 V Li-ion, onboard charger, battery on ADC1_CH3, PWR/BOOT buttons |

Pins and board constants are in [`main/user_config.h`](main/user_config.h).

---

## Build and flash

```sh
. $HOME/.espressif/export.sh          # or your IDF environment
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyACM0 flash monitor
```

Notes:
- `managed_components/` is downloaded by the component manager — never edit it.
- `sdkconfig` is generated; change settings in [`sdkconfig.defaults`](sdkconfig.defaults)
  (delete `sdkconfig` to regenerate). Key settings: 8 MB flash, octal PSRAM @ 80 MHz,
  FATFS long filenames, LVGL fonts 12/16/20/28/48, LVGL heap 128 KB.
- The Waveshare reference examples live outside this repo; their root is in `.env`
  (`EXAMPLE_PATH`), e.g. `11_FactoryProgram`, `03_I2C_SHTC3`, `13_FT6336_Test`.

---

## Project layout

| Component | Responsibility |
|---|---|
| `main` | `app_main`, LVGL port + e-paper flush, board pins (`user_config.h`) |
| `board_power_bsp` | EPD / audio / VBAT power rails |
| `epaper_driver_bsp` | SPI e-paper driver (full + partial refresh) |
| `i2c_bus` | Shared I2C master bus (`i2c_master`) |
| `touch_bsp` | FT6336 polling driver |
| `rtc_bsp` | PCF85063A wrapper (`waveshare/pcf85063a`) |
| `sensor_bsp` | SHTC3 temperature / humidity |
| `battery_bsp` | Battery voltage and percent (ADC) |
| `storage_bsp` | SDMMC/FAT mount, file listing, WAV header read/write, mutex |
| `audio_bsp` | ES8311 codec via vendored `codec_board` + `espressif/esp_codec_dev` |
| `codec_board` | Vendored ESP codec-board config (adapted for IDF 6) |
| `settings` | NVS-backed configuration |
| `app_state` | State machine, event queue, UI snapshot, activity tracking |
| `ui_bsp` | GUI Guider output plus the code-driven pages in `custom/ui_pages.c` |
| `user_app` | Application glue: init + recorder/player/sensor/power tasks |

### Tasks

| Task | Core | Job |
|---|---|---|
| `LVGL` | 1 | LVGL tick + flush (blocking e-paper refresh) |
| `app` | 1 | Consumes record/play events |
| `recorder` / `player` | 1 | Stream WAV to/from SD (spawned on demand) |
| `sensor` | 0 | RTC time + SHTC3 + battery, once a minute |
| `power` | 0 | Idle auto-sleep (deep sleep) |

---

## UI and navigation

Four pages in linear order: **Home (0) → Record (1) → Files (2) → Settings (3)**.
A fixed 48 px bottom bar has four 50 px slots; absent icons leave a blank slot.

| Page | Slot 1 Home | Slot 2 Gear | Slot 3 Back `<-` | Slot 4 Next `->` |
|---|---|---|---|---|
| Home | blank | Settings | blank | Record |
| Record | Home | Settings | Home | Files |
| Files | Home | Settings | Record | Settings |
| Settings | Home | blank | Files | Home (wraps) |

- **Home** — `HH:MM`, date, temperature/humidity, battery; updates on minute boundary.
- **Record** — large circle (idle) / square (recording) symbol; status line; in-place WAV streaming.
- **Files** — 5-row paged list (▲▼), tapping a `.wav` opens the player view (play/stop, List).
- **Settings** — scrollable list with an inline `+` / `-` / `OK` editor (time, timezone, 12/24h,
  temperature unit/offset, full-refresh interval, sample rate, mic gain, volume, auto-sleep, SD/battery/about).

### Recording format

WAV, PCM 16-bit mono, 16 kHz (default). Streamed to `/sdcard/REC_YYYYMMDD_HHMMSS.wav`,
flushed with a header patch roughly every 2 s so the file is playable after a power cut.
Playback accepts PCM 16-bit mono/stereo, 8–48 kHz.

### Display refresh policy

In-page changes and page changes use partial refresh; a full refresh runs every
`full_refresh` partials (default 10) to clear ghosting. Partial refresh writes both the
new frame (0x24) and the previous frame (0x26), which is required for the SSD1681 family
to erase pixels correctly.

To extend panel life the controller is put into deep sleep after every refresh, identical
frames are skipped, boot uses a single full refresh, deep sleep waits for any in-flight
refresh, and LVGL theme animations are disabled. See `docs/SPEC.md` section 9.1.

---

## Status

Implemented and verified on hardware: board bring-up, 4-page touch UI, RTC/date/time,
SHTC3 temperature/humidity, battery, SD mount, WAV record to SD, WAV playback, settings + NVS.

Not verified (needs bench testing): microphone capture quality, e-paper refresh noise
during recording, touch-wake from deep sleep, sustained write on low-quality cards.
Deep sleep is implemented but **disabled by default** (`auto_sleep_s = 0`) because touch
wake is unconfirmed.

---

## References

- Product page: https://www.waveshare.com/esp32-s3-epaper-1.54.htm
- ESP-IDF guide: https://docs.waveshare.com/ESP32-S3-ePaper-1.54/ESP-IDF
- Resources (schematic, datasheets): https://docs.waveshare.com/ESP32-S3-ePaper-1.54/Resources-And-Documents
- Examples: https://github.com/waveshareteam/ESP32-S3-ePaper-1.54
