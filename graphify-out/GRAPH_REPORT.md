# Graph Report - esp32-s3-e-paper-recorder  (2026-10-16)

## Corpus Check
- 31 files · ~245,863 words
- Verdict: corpus is large enough that graph structure adds value.
- Unclassified: 4 file(s) not represented in the graph (top: (none) 2, .csv 1, .defaults 1)

## Summary
- 163 nodes · 293 edges · 8 communities (6 shown, 2 thin omitted)
- Extraction: 77% EXTRACTED · 21% INFERRED · 1% AMBIGUOUS · INFERRED: 62 edges (avg confidence: 0.85)
- Token cost: 51,542 input · 0 output

## Community Hubs (Navigation)
- E-Paper SPI Driver
- Build & Spec Architecture
- GUI Guider Custom/Events
- App Init & IDF Deps
- GUI Guider Screen Setup
- Board Power Rails

## God Nodes (most connected - your core abstractions)
1. `epaper_driver_display` - 33 edges
2. `board_power_bsp_t` - 10 edges
3. `Waveshare ESP32-S3-Touch-ePaper-1.54 board` - 8 edges
4. `setup_ui()` - 7 edges
5. `ui_animation()` - 6 edges
6. `user_app component` - 6 edges
7. `Linear 4-page navigation with fixed 4-slot bottom bar` - 6 edges
8. `ui_load_scr_animation()` - 5 edges
9. `app_main()` - 5 edges
10. `epaper_driver_display::epaper_driver_display()` - 4 edges

## Surprising Connections (you probably didn't know these)
- `IDF version >=4.1.0 requirement` --conceptually_related_to--> `Waveshare ESP32-S3-Touch-ePaper-1.54 board`  [AMBIGUOUS]
  main/idf_component.yml → docs/SPEC.md
- `app_main()` --calls--> `user_app_init()`  [INFERRED]
  main/main.cpp → components/user_app/user_app.cpp
- `ESP-IDF project 02_ePaper_1_54_Test` --conceptually_related_to--> `Waveshare 11_FactoryProgram example`  [AMBIGUOUS]
  CMakeLists.txt → docs/SPEC.md
- `user_app component` --implements--> `App state machine (IDLE/RECORDING/PLAYING/SLEEPING)`  [AMBIGUOUS]
  components/user_app/CMakeLists.txt → docs/SPEC.md
- `lvgl/lvgl ^9.3.0 dependency` --conceptually_related_to--> `LVGL 8.3.11`  [AMBIGUOUS]
  main/idf_component.yml → docs/SPEC.md

## Import Cycles
- None detected.

## Hyperedges (group relationships)
- **Four-page linear UI** — docs_spec_home_screen, docs_spec_record_screen, docs_spec_files_screen, docs_spec_settings_screen, docs_spec_navigation_model [EXTRACTED 1.00]
- **Audio record/playback storage pipeline** — docs_spec_es8311_codec, docs_spec_wav_recording_pipeline, docs_spec_wav_playback, docs_spec_sdmmc_fat32_storage, docs_spec_sd_file_mutex [EXTRACTED 1.00]
- **ESP-IDF component dependency graph** — components_user_app_cmakelists_user_app, components_board_power_bsp_cmakelists_board_power_bsp, components_epaper_driver_bsp_cmakelists_epaper_driver_bsp, components_ui_bsp_cmakelists_ui_bsp, main_cmakelists_main [EXTRACTED 1.00]

## Communities (8 total, 2 thin omitted)

### Community 0 - "E-Paper SPI Driver"
Cohesion: 0.15
Nodes (7): epaper_driver_display, buffer, epaper_driver_display::epaper_driver_display(), Height, lcd_spi_data, spi, Width

### Community 1 - "Build & Spec Architecture"
Cohesion: 0.09
Nodes (28): ESP-IDF project 02_ePaper_1_54_Test, board_power_bsp component, epaper_driver_bsp component, ESP-IDF driver component, ui_bsp component (LVGL GUI Guider generated), user_app component, Software-switched power rails (EPD/Audio/VBAT), 1.54in 200x200 B/W e-paper display (+20 more)

### Community 2 - "GUI Guider Custom/Events"
Cohesion: 0.11
Nodes (4): custom_init(), events_init(), kb_event_cb(), ta_event_cb()

### Community 3 - "App Init & IDF Deps"
Cohesion: 0.11
Nodes (7): user_app_init(), user_ui_init(), app_main(), example_lvgl_flush_cb(), example_lvgl_lock(), example_lvgl_port_task(), example_lvgl_unlock()

### Community 4 - "GUI Guider Screen Setup"
Cohesion: 0.13
Nodes (9): init_keyboard(), init_scr_del_flag(), setup_bottom_layer(), setup_ui(), ui_animation(), ui_init_style(), ui_load_scr_animation(), video_play() (+1 more)

### Community 5 - "Board Power Rails"
Cohesion: 0.16
Nodes (4): board_power_bsp_t, audio_power_pin, epd_power_pin, vbat_power_pin

## Ambiguous Edges - Review These
- `ESP-IDF project 02_ePaper_1_54_Test` → `Waveshare 11_FactoryProgram example`  [AMBIGUOUS]
  CMakeLists.txt · relation: conceptually_related_to
- `user_app component` → `App state machine (IDLE/RECORDING/PLAYING/SLEEPING)`  [AMBIGUOUS]
  components/user_app/CMakeLists.txt · relation: implements
- `lvgl/lvgl ^9.3.0 dependency` → `LVGL 8.3.11`  [AMBIGUOUS]
  main/idf_component.yml · relation: conceptually_related_to
- `IDF version >=4.1.0 requirement` → `Waveshare ESP32-S3-Touch-ePaper-1.54 board`  [AMBIGUOUS]
  main/idf_component.yml · relation: conceptually_related_to

## Knowledge Gaps
- **12 isolated node(s):** `epd_power_pin`, `audio_power_pin`, `vbat_power_pin`, `lcd_spi_data`, `Width` (+7 more)
  These have ≤1 connection - possible missing edges or undocumented components. (Counts symbols only; 53 node(s) total have ≤1 connection when file, concept and rationale nodes are included.)
- **2 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `epaper_driver_display` connect `E-Paper SPI Driver` to `Board Power Rails`?**
  _High betweenness centrality (0.081) - this node is a cross-community bridge._
- **What connects `epd_power_pin`, `audio_power_pin`, `vbat_power_pin` to the rest of the system?**
  _12 weakly-connected nodes found - possible documentation gaps or missing edges._
- **Should `Build & Spec Architecture` be split into smaller, more focused modules?**
  _Cohesion score 0.09090909090909091 - nodes in this community are weakly interconnected._
- **Should `GUI Guider Custom/Events` be split into smaller, more focused modules?**
  _Cohesion score 0.1076923076923077 - nodes in this community are weakly interconnected._

### Low-confidence Hints
_AMBIGUOUS edges — the extractor was unsure. Verify before acting on these._

- **What is the exact relationship between `ESP-IDF project 02_ePaper_1_54_Test` and `Waveshare 11_FactoryProgram example`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._
- **What is the exact relationship between `user_app component` and `App state machine (IDLE/RECORDING/PLAYING/SLEEPING)`?**
  _Edge tagged AMBIGUOUS (relation: implements) - confidence is low._
- **What is the exact relationship between `lvgl/lvgl ^9.3.0 dependency` and `LVGL 8.3.11`?**
  _Edge tagged AMBIGUOUS (relation: conceptually_related_to) - confidence is low._