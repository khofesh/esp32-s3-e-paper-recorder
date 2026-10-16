# Graph Report - esp32-s3-e-paper-recorder  (2026-10-16)

## Corpus Check
- 57 files · ~262,793 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 542 nodes · 1186 edges · 29 communities (23 shown, 6 thin omitted)
- Extraction: 82% EXTRACTED · 18% INFERRED · 0% AMBIGUOUS · INFERRED: 215 edges (avg confidence: 0.85)
- Token cost: 57,580 input · 0 output

## Community Hubs (Navigation)
- App State Core
- UI Pages & State Access
- GUI Guider UI
- E-Paper Driver
- Board Config Parser
- Codec Board LCD Init
- BSP Headers & ADC
- I2C Device Drivers
- Spec: UI & Hardware
- Component Build Graph
- Codec Init & Teardown
- LVGL Port & Main
- TCA9554 IO Expander
- I2C Master Bus
- Audio Codec Board
- Power Rails
- Dummy Codec
- Codec Board Selection
- Codec & I2S Setup
- RTC & Power Policy
- SD Card Mount
- I2C Pin Init
- NVS Settings
- Test Project
- IDF Driver Dep
- IDF Version Req

## God Nodes (most connected - your core abstractions)
1. `epaper_driver_display` - 34 edges
2. `ESP32-S3 e-Paper Recorder Spec v0.1` - 28 edges
3. `user_app component` - 20 edges
4. `str_same()` - 16 edges
5. `ui_set_text()` - 15 edges
6. `start_recording()` - 15 edges
7. `codec_board component` - 15 edges
8. `user_app_init()` - 14 edges
9. `esp32_s3_epaper_recorder project` - 14 edges
10. `ui_bsp component` - 13 edges

## Surprising Connections (you probably didn't know these)
- `ES8311 codec` --semantically_similar_to--> `ES8311 audio codec`  [INFERRED] [semantically similar]
  components/codec_board/board_cfg.txt → docs/SPEC.md
- `esp_codec_dev` --semantically_similar_to--> `espressif/esp_codec_dev`  [INFERRED] [semantically similar]
  components/audio_bsp/CMakeLists.txt → main/idf_component.yml
- `waveshare/pcf85063a` --semantically_similar_to--> `PCF85063 RTC`  [INFERRED] [semantically similar]
  components/rtc_bsp/CMakeLists.txt → docs/SPEC.md
- `nvs_flash` --semantically_similar_to--> `NVS settings storage`  [INFERRED] [semantically similar]
  components/settings/CMakeLists.txt → docs/SPEC.md
- `lvgl` --semantically_similar_to--> `lvgl/lvgl`  [INFERRED] [semantically similar]
  components/ui_bsp/CMakeLists.txt → main/idf_component.yml

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Drivers sharing the I2C bus** — components_i2c_bus_cmakelists_i2c_bus, components_rtc_bsp_cmakelists_rtc_bsp, components_sensor_bsp_cmakelists_sensor_bsp, components_touch_bsp_cmakelists_touch_bsp [EXTRACTED 1.00]
- **Recording pipeline: codec -> ring buffer -> WAV on SD** — docs_spec_es8311, docs_spec_psram_ring_buffer, docs_spec_wav_recording, docs_spec_sd_card_fat32, docs_spec_task_architecture [INFERRED 0.85]
- **user_app integrates all BSP components** — components_user_app_cmakelists_user_app, components_board_power_bsp_cmakelists_board_power_bsp, components_epaper_driver_bsp_cmakelists_epaper_driver_bsp, components_ui_bsp_cmakelists_ui_bsp, components_audio_bsp_cmakelists_audio_bsp, components_storage_bsp_cmakelists_storage_bsp, components_app_state_cmakelists_app_state [EXTRACTED 1.00]

## Communities (29 total, 6 thin omitted)

### Community 0 - "App State Core"
Cohesion: 0.06
Nodes (37): app_state_begin(), app_state_commit(), app_state_init(), app_state_last_activity_ms(), app_state_set_state(), app_state_set_status(), app_state_wait(), audio_init() (+29 more)

### Community 1 - "UI Pages & State Access"
Cohesion: 0.10
Nodes (50): app_state_post(), app_state_read(), app_state_state(), app_state_version(), battery_level(), battery_voltage(), rtc_get(), rtc_ok() (+42 more)

### Community 2 - "GUI Guider UI"
Cohesion: 0.07
Nodes (13): custom_init(), events_init(), init_keyboard(), init_scr_del_flag(), setup_bottom_layer(), setup_ui(), ui_animation(), ui_init_style() (+5 more)

### Community 3 - "E-Paper Driver"
Cohesion: 0.16
Nodes (8): epaper_driver_display, buffer, epaper_driver_display::epaper_driver_display(), Height, lcd_spi_data, old_buffer, spi, Width

### Community 4 - "Board Config Parser"
Cohesion: 0.14
Nodes (25): fill_camera_cfg(), fill_cfg(), fill_codec_cfg(), fill_i2c_cfg(), fill_i2s_cfg(), fill_lcd_cfg(), fill_sdcard_cfg(), free_cfg_line() (+17 more)

### Community 5 - "Codec Board LCD Init"
Cohesion: 0.09
Nodes (16): get_lcd_cfg(), board_lcd_init(), get_hw_gpio(), init_extend_io(), _init_lcd(), _init_mipi_lcd(), _init_spi_lcd(), _lcd_rest() (+8 more)

### Community 6 - "BSP Headers & ADC"
Cohesion: 0.09
Nodes (7): battery_init(), I2cMasterBus, I2cMasterBus, settings_defaults(), settings_init(), settings_save(), I2cMasterBus

### Community 7 - "I2C Device Drivers"
Cohesion: 0.09
Nodes (10): app_state_note_activity(), rtc_dev_init(), crc8(), sensor_init(), sensor_read(), shtc3_send(), reg_read(), touch_init() (+2 more)

### Community 8 - "Spec: UI & Hardware"
Cohesion: 0.15
Nodes (17): 1.54in 200x200 1-bit e-paper display, ESP32-S3-PICO-1-N8R8 SoC, UI-service event queue (EVT_TOUCH, EVT_REC_START, EVT_REC_STOP, EVT_PLAY_DONE, EVT_SD_ERROR, EVT_MINUTE_TICK), Waveshare 11_FactoryProgram example, Files screen (list + player views), FT6336 capacitive touch, Home screen (time, temp/humidity, battery), Milestones M0-M6 (+9 more)

### Community 9 - "Component Build Graph"
Cohesion: 0.18
Nodes (21): esp32_s3_epaper_recorder project, app_state component, board_power_bsp component, epaper_driver_bsp component, i2c_bus component, sensor_bsp component, storage_bsp component, touch_bsp component (+13 more)

### Community 10 - "Codec Init & Teardown"
Cohesion: 0.11
Nodes (6): deinit_codec(), deinit_i2c(), get_playback_handle(), get_record_handle(), _i2c_deinit(), _i2s_deinit()

### Community 11 - "LVGL Port & Main"
Cohesion: 0.13
Nodes (10): user_app_start(), user_ui_init(), LVGL graphics library, lvgl, lvgl/lvgl, app_main(), lvgl_flush_cb(), lvgl_lock() (+2 more)

### Community 12 - "TCA9554 IO Expander"
Cohesion: 0.29
Nodes (11): expander_dev_prob(), tca9554_deinit(), tca9554_get_input_state(), tca9554_get_io_config(), tca9554_get_output_state(), tca9554_init(), tca9554_read_reg(), tca9554_set_io_config() (+3 more)

### Community 13 - "I2C Master Bus"
Cohesion: 0.20
Nodes (3): I2cMasterBus, bus_handle, instance_

### Community 14 - "Audio Codec Board"
Cohesion: 0.18
Nodes (14): audio_bsp component, ES8311 codec, Board config S3_ePaper_1_54 (ES8311, I2C 47/48, I2S pins, PA 46), codec_board changelog (v0.5.0 - v2.0.0), codec_board_parse_all_config API, set_default_codec_board API, codec_board component, esp-webrtc-solution (vendored source) (+6 more)

### Community 15 - "Power Rails"
Cohesion: 0.23
Nodes (4): board_power_bsp_t, audio_power_pin, epd_power_pin, vbat_power_pin

### Community 16 - "Dummy Codec"
Cohesion: 0.33
Nodes (6): dummy_codec_close(), dummy_codec_enable(), dummy_codec_is_open(), dummy_codec_new(), dummy_codec_open(), dummy_codec_set_fs()

### Community 17 - "Codec Board Selection"
Cohesion: 0.25
Nodes (4): get_camera_cfg(), get_i2s_pin(), set_codec_board_type(), set_default_codec_board()

### Community 18 - "Codec & I2S Setup"
Cohesion: 0.31
Nodes (6): get_in_codec_cfg(), get_out_codec_cfg(), check_i2c_inited(), get_i2c_bus_handle(), _i2s_init(), init_codec()

### Community 19 - "RTC & Power Policy"
Cohesion: 0.40
Nodes (5): battery_bsp component, rtc_bsp component, PCF85063 RTC, esp_adc, waveshare/pcf85063a

### Community 20 - "SD Card Mount"
Cohesion: 0.33
Nodes (4): get_sdcard_config(), enable_mmc_phy_power(), mount_sdcard(), sdmmc_get_slot()

### Community 22 - "I2C Pin Init"
Cohesion: 0.50
Nodes (3): get_i2c_pin(), _i2c_init(), init_i2c()

### Community 23 - "NVS Settings"
Cohesion: 1.00
Nodes (3): settings component, NVS settings storage, nvs_flash

## Knowledge Gaps
- **28 isolated node(s):** `audio_power_pin`, `epd_power_pin`, `vbat_power_pin`, `ESP-IDF project 02_ePaper_1_54_Test`, `ESP-IDF driver component` (+23 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 139 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **6 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `user_app component` connect `Component Build Graph` to `LVGL Port & Main`, `RTC & Power Policy`, `Audio Codec Board`, `NVS Settings`?**
  _High betweenness centrality (0.048) - this node is a cross-community bridge._
- **Are the 2 inferred relationships involving `user_app component` (e.g. with `esp32_s3_epaper_recorder project` and `FreeRTOS task layout (audio_rec, sd_writer, audio_play, ui, input, sensor)`) actually correct?**
  _`user_app component` has 2 INFERRED edges - model-reasoned connections that need verification._
- **What connects `audio_power_pin`, `epd_power_pin`, `vbat_power_pin` to the rest of the system?**
  _28 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `App State Core` be split into smaller, more focused modules?**
  _Cohesion score 0.05548654244306418 - nodes in this community are weakly interconnected._
- **Why does `ui_bsp component` connect `Component Build Graph` to `Spec: UI & Hardware`, `LVGL Port & Main`, `RTC & Power Policy`, `NVS Settings`?**
  _High betweenness centrality (0.033) - this node is a cross-community bridge._
- **Should `UI Pages & State Access` be split into smaller, more focused modules?**
  _Cohesion score 0.09679370840895342 - nodes in this community are weakly interconnected._
- **Why does `audio_bsp component` connect `Audio Codec Board` to `Component Build Graph`, `LVGL Port & Main`?**
  _High betweenness centrality (0.031) - this node is a cross-community bridge._