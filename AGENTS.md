# agents

## Project

- Waveshare ESP32-S3-Touch-ePaper-1.54 (ESP32-S3-PICO-1-N8R8, 8 MB flash, 8 MB PSRAM): voice recorder with 200x200 1-bit e-paper UI
- Spec: `docs/SPEC.md` — read relevant section before implementing features
- ESP-IDF 5.5.x (`idf.py build`, `idf.py -p <port> flash monitor`), target `esp32s3`
- LVGL 9.3 via component manager (`main/idf_component.yml`); `managed_components/` is downloaded — never edit
- Layout:
  - `main/` — `app_main`, LVGL port/flush (`main.cpp`), board pins & constants (`user_config.h`)
  - `components/board_power_bsp` — EPD/audio/VBAT power rails
  - `components/epaper_driver_bsp` — SPI e-paper driver (full/partial refresh)
  - `components/ui_bsp` — NXP GUI Guider UI: `generated/` is tool output (don't hand-edit, lost on regeneration); put changes in `custom/`
  - `components/user_app` — app init and glue
- Config changes: prefer `sdkconfig.defaults`; don't hand-edit `sdkconfig` (use `idf.py menuconfig`). Partition table in `partitions.csv`

## Response Style

- Be extremely concise. No pleasantries, no filler.
- When asked to write code, return code only unless explanation is explicitly requested.
- No sycophantic preambles ("Sure!", "Great question!", "Absolutely!").
- No "Here's a function that..." preambles.
- Don't restate the question before answering.
- No "Note:", "Tip:", or "Remember:" appendices unless asked.
- No usage examples unless asked.
- No unsolicited suggestions or improvements beyond what was asked.
- Use short variable names where meaning is clear from context.

## Code Style

- C++ (drivers/app, `.cpp`) and C (`ui_bsp`, `.c`); ESP-IDF + FreeRTOS APIs, GCC Xtensa toolchain
- `app_main` is `extern "C"`; C headers included from C++ need `extern "C"` guards
- Each component has its own `CMakeLists.txt` (`idf_component_register`); declare deps in `REQUIRES`/`PRIV_REQUIRES`
- Pins and board constants go in `main/user_config.h`; don't hardcode GPIOs elsewhere
- Use fixed-width types from `<stdint.h>` (`uint8_t`, `int32_t`), not `int`/`long`
- `static` for file-local functions and variables; no globals unless shared across translation units (`extern` in header)
- Check `esp_err_t` returns (`ESP_ERROR_CHECK` at init, `ESP_RETURN_ON_ERROR` / log + handle elsewhere); don't ignore them
- Allocate at init only; large buffers in PSRAM via `heap_caps_malloc(..., MALLOC_CAP_SPIRAM)`; no allocation in hot paths. DMA buffers need `MALLOC_CAP_DMA`
- LVGL is not thread-safe: call `lv_*` only from the LVGL task or while holding the LVGL mutex
- ISRs: `IRAM_ATTR`, short — set flags / notify tasks, defer work; shared ISR variables must be `volatile`
- No busy-wait or long `vTaskDelay` in audio/record paths; e-paper refresh must not stall audio (use separate tasks/queues)
- Logging via `ESP_LOGx` with a file-local `TAG`
- Header guards via `#ifndef FILE_NAME_H` / `#define FILE_NAME_H`, matching existing headers
- Minimal inline comments — only for genuinely complex logic or hardware quirks (register timing, errata)

## Context Retrieval Policy

Always retrieve the smallest amount of information necessary.
Escalate only when necessary. Stop escalating as soon as sufficient information has been obtained.

Preferred order:

1. Need code structure? `graph.json`.
2. Need symbols? `ast-grep`.
3. Need repository metadata? `rtk`.
4. Need implementation? Source files.
5. Use repository-wide search as last resort

Avoid reading entire directories or the whole repository unless explicitly requested.

## RTK (Rust Token Killer) - Token-Optimized CLI

`rtk` is a CLI proxy that filters and compresses command outputs, saving 60-90% tokens.

**Always** prefix commands with `rtk`. If RTK has a dedicated filter, it uses it. If not, it passes through unchanged. This means RTK is always safe to use.

Instead of:

```
ls -la .
git status
git log -10
docker ps
```

Use:

```
rtk ls -la .
rtk git status
rtk git log -10
rtk docker ps
```

### RTK commands example

```bash
rtk ls <path>
rtk read <file>
rtk find <pattern>

rtk err <cmd>           # Filter errors only from any command
rtk log <file>          # Deduplicated logs with counts
rtk json <file>         # JSON structure without values

rtk curl <url>          # Compact HTTP responses

rtk docker ps           # Compact container list
rtk docker images       # Compact image list
rtk docker logs <c>     # Deduplicated logs
```

## Graphify - Codebase context & knowledge graph protocol

A pre-computed AST knowledge graph (`graph.json`) is available at: `graphify-out/graph.json`
Use `graph.json` before searching or reading multiple source files.

Workflow:

1. Read `graph.json`.
2. Identify the relevant symbols, files, and dependency paths.
3. Read only the source files required for the task.
4. Avoid scanning unrelated files.

Use the graph for:

- dependency tracing
- call-path discovery
- identifying high-centrality modules
- impact analysis before refactoring
- locating symbol definitions

Never read an entire directory simply to locate a symbol. Use `graph.json` to locate the symbol first, then read only the relevant files.

The graph is generated from AST extraction and represents extracted structural relationships only. Treat graph edges as authoritative for structural relationships.
Do not infer dependencies that are absent from the graph.

## ast-grep

Prefer `ast-grep` over `grep` whenever searching source code.

Use `ast-grep` for:

- locating function definitions
- locating class definitions
- locating method implementations
- finding imports
- matching AST patterns
- performing structural search

Use `grep` only for:

- Markdown
- JSON
- YAML
- log files
- generated text
- plain-text configuration

Avoid using `grep` to search programming language source code when `ast-grep` can answer the query.
