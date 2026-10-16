#include "ui_pages.h"

#include "app_state.h"
#include "settings.h"
#include "storage_bsp.h"
#include "battery_bsp.h"
#include "rtc_bsp.h"
#include "touch_bsp.h"
#include "user_config.h"

#include "lvgl.h"
#include "esp_vfs_fat.h"
#include "esp_log.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- Layout ---- */
#define CONTENT_W 200
#define CONTENT_H 152
#define BAR_Y 152
#define BAR_H 48
#define SLOT_W 50
#define FILES_ROWS 5
#define SET_ROWS 5

/* ---- Home ---- */
static lv_obj_t *s_page[4];
static lv_obj_t *s_home_time, *s_home_date, *s_home_temp, *s_home_batt;

/* ---- Record ---- */
static lv_obj_t *s_rec_symbol, *s_rec_status, *s_rec_name;

/* ---- Files ---- */
static lv_obj_t *s_files_list, *s_files_header;
static lv_obj_t *s_row[FILES_ROWS];
static lv_obj_t *s_row_name[FILES_ROWS];
static lv_obj_t *s_row_size[FILES_ROWS];
static storage_file_t s_files[STORAGE_MAX_FILES];
static int s_files_count;
static int s_files_page;
static lv_obj_t *s_player, *s_player_title, *s_player_info, *s_player_btn_txt;
static char s_player_path[320];

/* ---- Settings ---- */
static lv_obj_t *s_set_list;
static lv_obj_t *s_set_row[SET_ROWS];
static lv_obj_t *s_set_val[SET_ROWS];
static lv_obj_t *s_set_editor, *s_set_ed_title, *s_set_ed_value;
static int s_set_offset;
static int s_time_field;
static rtc_datetime_t s_time_edit;

/* ---- Bottom bar ---- */
static lv_obj_t *s_slot[4];
static lv_obj_t *s_slot_txt[4];

static int s_page_idx;
static uint32_t s_last_version;
static app_state_t s_last_state;

static const char *SYM_HOME = LV_SYMBOL_HOME;
static const char *SYM_SET = LV_SYMBOL_SETTINGS;
static const char *SYM_PREV = LV_SYMBOL_LEFT;
static const char *SYM_NEXT = LV_SYMBOL_RIGHT;

/* nav[page][slot] = target page, -1 = blank */
static const int8_t s_nav[4][4] = {
    {-1, 3, -1, 1}, /* Home */
    {0, 3, 0, 2},   /* Record */
    {0, 3, 1, 3},   /* Files */
    {0, -1, 2, 0},  /* Settings */
};

static void refresh_page(int idx);

/* ---------------- helpers ---------------- */

static lv_obj_t *make_label(lv_obj_t *parent, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, color, 0);
    return l;
}

/* Avoid LVGL's free+malloc churn when the text has not changed. */
static void ui_set_text(lv_obj_t *obj, const char *text)
{
    const char *cur = lv_label_get_text(obj);
    if (cur == NULL || strcmp(cur, text) != 0) {
        lv_label_set_text(obj, text);
    }
}

static lv_obj_t *make_button(lv_obj_t *parent, const lv_font_t *font)
{
    lv_obj_t *b = lv_button_create(parent);
    lv_obj_set_style_radius(b, 4, 0);
    lv_obj_set_style_border_width(b, 1, 0);
    lv_obj_set_style_border_color(b, lv_color_black(), 0);
    lv_obj_set_style_bg_color(b, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_shadow_width(b, 0, 0);
    lv_obj_set_style_pad_all(b, 2, 0);
    lv_obj_remove_flag(b, LV_OBJ_FLAG_SCROLLABLE);
    (void)font;
    return b;
}

static lv_obj_t *make_container(lv_obj_t *parent, int x, int y, int w, int h)
{
    lv_obj_t *c = lv_obj_create(parent);
    lv_obj_set_pos(c, x, y);
    lv_obj_set_size(c, w, h);
    lv_obj_set_style_bg_color(c, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(c, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(c, 0, 0);
    lv_obj_set_style_pad_all(c, 0, 0);
    lv_obj_set_style_radius(c, 0, 0);
    lv_obj_remove_flag(c, LV_OBJ_FLAG_SCROLLABLE);
    return c;
}

/* ---------------- navigation ---------------- */

static void show_page(int idx, bool force)
{
    if (idx < 0 || idx > 3) {
        return;
    }
    if (idx != s_page_idx || force) {
        if (s_page_idx == 2 && idx != 2) {
            if (app_state_state() == APP_PLAYING) {
                app_state_post(EVT_PLAY_STOP, NULL);
            }
            app_snapshot_t *s = app_state_begin();
            s->in_player = false;
            app_state_commit();
        }
    }
    s_page_idx = idx;
    for (int i = 0; i < 4; i++) {
        if (i == idx) {
            lv_obj_remove_flag(s_page[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(s_page[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
    for (int slot = 0; slot < 4; slot++) {
        int target = s_nav[idx][slot];
        if (target < 0) {
            lv_obj_add_flag(s_slot[slot], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_remove_flag(s_slot[slot], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_user_data(s_slot[slot], (void *)(intptr_t)target);
        }
    }
    if (idx == 2) {
        s_files_page = 0;
        lv_obj_add_flag(s_player, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_files_list, LV_OBJ_FLAG_HIDDEN);
        app_snapshot_t *sp = app_state_begin();
        sp->in_player = false;
        app_state_commit();
        refresh_page(2);
    }
    refresh_page(idx);
}

static void bar_clicked(lv_event_t *e)
{
    int target = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    show_page(target, false);
}

/* ---------------- home ---------------- */

static void refresh_home(void)
{
    app_snapshot_t s;
    app_state_read(&s);
    app_settings_t *cfg = settings();

    char buf[32];
    if (s.home_valid) {
        int h = s.hour;
        if (cfg->hour12) {
            const char *ap = h < 12 ? "AM" : "PM";
            int hh = h % 12;
            if (hh == 0) hh = 12;
            snprintf(buf, sizeof(buf), "%d:%02d %s", hh, s.minute, ap);
        } else {
            snprintf(buf, sizeof(buf), "%02d:%02d", h, s.minute);
        }
    } else {
        snprintf(buf, sizeof(buf), "--:--");
    }
    ui_set_text(s_home_time, buf);

    if (s.home_valid) {
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d", s.year, s.month, s.day);
    } else {
        snprintf(buf, sizeof(buf), "no time");
    }
    ui_set_text(s_home_date, buf);

    if (s.sensor_ok) {
        float t = s.temp_c + cfg->temp_offset;
        if (cfg->temp_f) {
            t = t * 9.0f / 5.0f + 32.0f;
        }
        snprintf(buf, sizeof(buf), "%d\xC2\xB0""%c   %d%%", (int)t, cfg->temp_f ? 'F' : 'C', (int)s.humidity);
    } else {
        snprintf(buf, sizeof(buf), "--");
    }
    ui_set_text(s_home_temp, buf);

    snprintf(buf, sizeof(buf), "%d%%", (int)s.battery);
    ui_set_text(s_home_batt, buf);
}

/* ---------------- record ---------------- */

static void set_rec_symbol(bool recording)
{
    lv_obj_t *o = s_rec_symbol;
    lv_obj_set_style_radius(o, recording ? 2 : LV_RADIUS_CIRCLE, 0);
    lv_obj_set_size(o, recording ? 52 : 64, recording ? 52 : 64);
    lv_obj_align(o, LV_ALIGN_TOP_MID, 0, 26);
}

static void refresh_record(void)
{
    app_snapshot_t s;
    app_state_read(&s);
    bool rec = (s.state == APP_RECORDING);
    set_rec_symbol(rec);
    if (rec) {
        ui_set_text(s_rec_status, "REC");
        ui_set_text(s_rec_name, s.rec_name);
    } else if (s.state == APP_PLAYING) {
        ui_set_text(s_rec_status, "Playing");
        ui_set_text(s_rec_name, "");
    } else {
        ui_set_text(s_rec_status, s.status);
        ui_set_text(s_rec_name, "");
    }
}

static void rec_symbol_clicked(lv_event_t *e)
{
    (void)e;
    app_state_t st = app_state_state();
    if (st == APP_RECORDING) {
        app_state_post(EVT_REC_STOP, NULL);
    } else if (st == APP_PLAYING) {
        app_state_set_status("Busy");
    } else if (!storage_mounted()) {
        app_state_set_status("No SD");
    } else if (battery_level() < 10) {
        app_state_set_status("Battery low");
    } else {
        app_state_post(EVT_REC_START, NULL);
    }
}

/* ---------------- files ---------------- */

static void load_files(void)
{
    storage_lock();
    s_files_count = storage_list(s_files, STORAGE_MAX_FILES);
    storage_unlock();
    int pages = (s_files_count + FILES_ROWS - 1) / FILES_ROWS;
    if (pages == 0) pages = 1;
    if (s_files_page >= pages) s_files_page = pages - 1;
}

static void render_files(void)
{
    int pages = (s_files_count + FILES_ROWS - 1) / FILES_ROWS;
    if (pages == 0) pages = 1;
    char buf[32];
    snprintf(buf, sizeof(buf), "Files  %d/%d", s_files_page + 1, pages);
    ui_set_text(s_files_header, buf);

    for (int i = 0; i < FILES_ROWS; i++) {
        int fi = s_files_page * FILES_ROWS + i;
        if (fi < s_files_count) {
            storage_file_t *f = &s_files[fi];
            lv_obj_remove_flag(s_row[i], LV_OBJ_FLAG_HIDDEN);
            ui_set_text(s_row_name[i], f->name);
            if (f->size >= 1024 * 1024) {
                snprintf(buf, sizeof(buf), "%d.%dM", (int)(f->size / (1024 * 1024)),
                         (int)((f->size / (1024 * 102)) % 10));
            } else {
                snprintf(buf, sizeof(buf), "%dK", (int)(f->size / 1024));
            }
            ui_set_text(s_row_size[i], buf);
            lv_obj_set_user_data(s_row[i], (void *)(intptr_t)fi);
        } else {
            lv_obj_add_flag(s_row[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void file_row_clicked(lv_event_t *e)
{
    int fi = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (fi < 0 || fi >= s_files_count || !s_files[fi].is_wav) {
        return;
    }
    snprintf(s_player_path, sizeof(s_player_path), "%s/%s", SD_MOUNT_POINT, s_files[fi].name);
    ui_set_text(s_player_title, s_files[fi].name);

    uint32_t data_bytes = 0, rate = 0;
    uint16_t ch = 0, bits = 0;
    storage_lock();
    esp_err_t ret = wav_read_info(s_player_path, &data_bytes, &rate, &ch, &bits);
    storage_unlock();
    char info[48];
    if (ret == ESP_OK) {
        uint32_t secs = rate ? data_bytes / (rate * ch * bits / 8) : 0;
        snprintf(info, sizeof(info), "%d:%02d  %d Hz", (int)(secs / 60), (int)(secs % 60), (int)rate);
    } else {
        snprintf(info, sizeof(info), "Unsupported");
    }
    ui_set_text(s_player_info, info);
    ui_set_text(s_player_btn_txt, LV_SYMBOL_PLAY);

    lv_obj_add_flag(s_files_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_player, LV_OBJ_FLAG_HIDDEN);

    app_snapshot_t *s = app_state_begin();
    s->in_player = true;
    app_state_commit();
}

static void player_btn_clicked(lv_event_t *e)
{
    (void)e;
    if (app_state_state() == APP_PLAYING) {
        app_state_post(EVT_PLAY_STOP, NULL);
    } else {
        app_state_post(EVT_PLAY_START, s_player_path);
    }
}

static void player_list_clicked(lv_event_t *e)
{
    (void)e;
    if (app_state_state() == APP_PLAYING) {
        app_state_post(EVT_PLAY_STOP, NULL);
    }
    lv_obj_add_flag(s_player, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_files_list, LV_OBJ_FLAG_HIDDEN);
    app_snapshot_t *s = app_state_begin();
    s->in_player = false;
    app_state_commit();
}

static void files_up_clicked(lv_event_t *e)
{
    (void)e;
    if (s_files_page > 0) {
        s_files_page--;
        render_files();
    }
}

static void files_down_clicked(lv_event_t *e)
{
    (void)e;
    int pages = (s_files_count + FILES_ROWS - 1) / FILES_ROWS;
    if (s_files_page + 1 < pages) {
        s_files_page++;
        render_files();
    }
}

static void refresh_player(void)
{
    bool playing = (app_state_state() == APP_PLAYING);
    ui_set_text(s_player_btn_txt, playing ? LV_SYMBOL_STOP : LV_SYMBOL_PLAY);
}

/* ---------------- settings ---------------- */

typedef enum {
    SI_TIME, SI_TZ, SI_HOUR12, SI_TEMPUNIT, SI_TEMPOFF, SI_FULLREF, SI_RATE,
    SI_MICGAIN, SI_VOLUME, SI_SLEEP, SI_SD, SI_BATT, SI_ABOUT, SI_COUNT
} si_t;

static const char *const s_si_name[SI_COUNT] = {
    "Set time", "Timezone", "12h/24h", "Temp unit", "Temp offset", "Full refresh",
    "Sample rate", "Mic gain", "Volume", "Auto sleep", "SD card", "Battery", "About"
};

static bool si_readonly(int i)
{
    return i == SI_SD || i == SI_BATT || i == SI_ABOUT;
}

static void si_value(int i, char *buf, size_t n)
{
    app_settings_t *cfg = settings();
    switch (i) {
    case SI_TIME: {
        rtc_datetime_t t = {};
        rtc_get(&t);
        if (rtc_ok() && t.year > 2000) {
            snprintf(buf, n, "%04d-%02d-%02d %02d:%02d", t.year, t.month, t.day, t.hour, t.min);
        } else {
            snprintf(buf, n, "not set");
        }
        break;
    }
    case SI_TZ: snprintf(buf, n, "UTC%+d", cfg->tz_offset); break;
    case SI_HOUR12: snprintf(buf, n, "%s", cfg->hour12 ? "12h" : "24h"); break;
    case SI_TEMPUNIT: snprintf(buf, n, "%s", cfg->temp_f ? "F" : "C"); break;
    case SI_TEMPOFF: snprintf(buf, n, "%+d", cfg->temp_offset); break;
    case SI_FULLREF: snprintf(buf, n, "%d", cfg->full_refresh); break;
    case SI_RATE: snprintf(buf, n, "%d Hz", cfg->sample_rate); break;
    case SI_MICGAIN: snprintf(buf, n, "%d", cfg->mic_gain); break;
    case SI_VOLUME: snprintf(buf, n, "%d", cfg->volume); break;
    case SI_SLEEP:
        if (cfg->auto_sleep_s == 0) snprintf(buf, n, "off");
        else snprintf(buf, n, "%ds", cfg->auto_sleep_s);
        break;
    case SI_SD: {
        uint64_t total = 0, free = 0;
        if (storage_mounted() && esp_vfs_fat_info(SD_MOUNT_POINT, &total, &free) == ESP_OK) {
            snprintf(buf, n, "%lluM/%lluM", (unsigned long long)(free >> 20), (unsigned long long)(total >> 20));
        } else {
            snprintf(buf, n, "No SD");
        }
        break;
    }
    case SI_BATT: snprintf(buf, n, "%d.%dV %d%%", (int)battery_voltage(), (int)(battery_voltage() * 10) % 10, battery_level()); break;
    case SI_ABOUT: snprintf(buf, n, "rec v0.1"); break;
    default: snprintf(buf, n, "-"); break;
    }
}

static void refresh_settings(void)
{
    for (int r = 0; r < SET_ROWS; r++) {
        int i = s_set_offset + r;
        if (i < SI_COUNT) {
            char buf[48];
            si_value(i, buf, sizeof(buf));
            lv_obj_remove_flag(s_set_row[r], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_user_data(s_set_row[r], (void *)(intptr_t)i);
            lv_obj_t *name = lv_obj_get_child(s_set_row[r], 0);
            ui_set_text(name, s_si_name[i]);
            ui_set_text(s_set_val[r], si_readonly(i) ? buf : buf);
            if (si_readonly(i)) {
                lv_obj_set_style_text_color(s_set_val[r], lv_color_hex(0x404040), 0);
            } else {
                lv_obj_set_style_text_color(s_set_val[r], lv_color_black(), 0);
            }
        } else {
            lv_obj_add_flag(s_set_row[r], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void time_field_label(char *buf, size_t n)
{
    switch (s_time_field) {
    case 0: snprintf(buf, n, "Year %d", s_time_edit.year); break;
    case 1: snprintf(buf, n, "Month %d", s_time_edit.month); break;
    case 2: snprintf(buf, n, "Day %d", s_time_edit.day); break;
    case 3: snprintf(buf, n, "Hour %d", s_time_edit.hour); break;
    default: snprintf(buf, n, "Min %d", s_time_edit.min); break;
    }
}

static void si_adjust(int i, int dir)
{
    app_settings_t *cfg = settings();
    switch (i) {
    case SI_TZ:
        cfg->tz_offset = (int8_t)(dir > 0 ? (cfg->tz_offset >= 14 ? -12 : cfg->tz_offset + 1)
                                          : (cfg->tz_offset <= -12 ? 14 : cfg->tz_offset - 1));
        break;
    case SI_HOUR12: cfg->hour12 = !cfg->hour12; break;
    case SI_TEMPUNIT: cfg->temp_f = !cfg->temp_f; break;
    case SI_TEMPOFF:
        cfg->temp_offset = (int8_t)(cfg->temp_offset + dir);
        if (cfg->temp_offset > 20) cfg->temp_offset = -20;
        if (cfg->temp_offset < -20) cfg->temp_offset = 20;
        break;
    case SI_FULLREF:
        cfg->full_refresh = (uint8_t)(cfg->full_refresh + dir);
        if (cfg->full_refresh < 1) cfg->full_refresh = 50;
        if (cfg->full_refresh > 50) cfg->full_refresh = 1;
        break;
    case SI_RATE: {
        static const uint16_t rates[] = {8000, 16000, 22050, 44100};
        int idx = 0;
        for (int k = 0; k < 4; k++) if (rates[k] == cfg->sample_rate) idx = k;
        idx = (idx + (dir > 0 ? 1 : 3)) % 4;
        cfg->sample_rate = rates[idx];
        break;
    }
    case SI_MICGAIN: {
        int g = (int)cfg->mic_gain + dir * 5;
        if (g < 0) g = 100;
        if (g > 100) g = 0;
        cfg->mic_gain = (uint8_t)g;
        break;
    }
    case SI_VOLUME: {
        int v = (int)cfg->volume + dir * 5;
        if (v < 0) v = 100;
        if (v > 100) v = 0;
        cfg->volume = (uint8_t)v;
        break;
    }
    case SI_SLEEP: {
        static const uint16_t vals[] = {0, 15, 30, 60, 120, 300};
        int idx = 0;
        for (int k = 0; k < 6; k++) if (vals[k] == cfg->auto_sleep_s) idx = k;
        idx = (idx + (dir > 0 ? 1 : 5)) % 6;
        cfg->auto_sleep_s = vals[idx];
        break;
    }
    default: break;
    }
    settings_save();
}

static void time_adjust(int dir)
{
    switch (s_time_field) {
    case 0:
        if (dir > 0) s_time_edit.year = (uint16_t)(s_time_edit.year >= 2099 ? 2024 : s_time_edit.year + 1);
        else s_time_edit.year = (uint16_t)(s_time_edit.year <= 2024 ? 2099 : s_time_edit.year - 1);
        break;
    case 1:
        if (dir > 0) s_time_edit.month = (uint8_t)(s_time_edit.month >= 12 ? 1 : s_time_edit.month + 1);
        else s_time_edit.month = (uint8_t)(s_time_edit.month <= 1 ? 12 : s_time_edit.month - 1);
        break;
    case 2:
        if (dir > 0) s_time_edit.day = (uint8_t)(s_time_edit.day >= 31 ? 1 : s_time_edit.day + 1);
        else s_time_edit.day = (uint8_t)(s_time_edit.day <= 1 ? 31 : s_time_edit.day - 1);
        break;
    case 3:
        if (dir > 0) s_time_edit.hour = (uint8_t)(s_time_edit.hour >= 23 ? 0 : s_time_edit.hour + 1);
        else s_time_edit.hour = (uint8_t)(s_time_edit.hour == 0 ? 23 : s_time_edit.hour - 1);
        break;
    default:
        if (dir > 0) s_time_edit.min = (uint8_t)(s_time_edit.min >= 59 ? 0 : s_time_edit.min + 1);
        else s_time_edit.min = (uint8_t)(s_time_edit.min == 0 ? 59 : s_time_edit.min - 1);
        break;
    }
}

static void editor_show(int i)
{
    lv_obj_set_user_data(s_set_editor, (void *)(intptr_t)i);
    ui_set_text(s_set_ed_title, s_si_name[i]);
    if (i == SI_TIME) {
        rtc_get(&s_time_edit);
        if (!rtc_ok() || s_time_edit.year < 2024) {
            s_time_edit.year = 2026;
            s_time_edit.month = 1;
            s_time_edit.day = 1;
            s_time_edit.hour = 0;
            s_time_edit.min = 0;
        }
        s_time_field = 0;
        char buf[32];
        time_field_label(buf, sizeof(buf));
        ui_set_text(s_set_ed_value, buf);
    } else {
        char buf[48];
        si_value(i, buf, sizeof(buf));
        ui_set_text(s_set_ed_value, buf);
    }
    lv_obj_set_user_data(s_set_ed_title, (void *)(intptr_t)i);
    lv_obj_add_flag(s_set_list, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_set_editor, LV_OBJ_FLAG_HIDDEN);
}

static void editor_close(void)
{
    lv_obj_add_flag(s_set_editor, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(s_set_list, LV_OBJ_FLAG_HIDDEN);
    refresh_settings();
}

static void setting_row_clicked(lv_event_t *e)
{
    int i = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_target(e));
    if (i >= SI_COUNT || si_readonly(i)) {
        return;
    }
    editor_show(i);
}

static void editor_minus(lv_event_t *e)
{
    (void)e;
    int i = (int)(intptr_t)lv_obj_get_user_data(s_set_editor);
    if (i == SI_TIME) {
        time_adjust(-1);
        char buf[32];
        time_field_label(buf, sizeof(buf));
        ui_set_text(s_set_ed_value, buf);
    } else {
        si_adjust(i, -1);
        char buf[48];
        si_value(i, buf, sizeof(buf));
        ui_set_text(s_set_ed_value, buf);
    }
}

static void editor_plus(lv_event_t *e)
{
    (void)e;
    int i = (int)(intptr_t)lv_obj_get_user_data(s_set_editor);
    if (i == SI_TIME) {
        time_adjust(1);
        char buf[32];
        time_field_label(buf, sizeof(buf));
        ui_set_text(s_set_ed_value, buf);
    } else {
        si_adjust(i, 1);
        char buf[48];
        si_value(i, buf, sizeof(buf));
        ui_set_text(s_set_ed_value, buf);
    }
}

static void editor_ok(lv_event_t *e)
{
    (void)e;
    int i = (int)(intptr_t)lv_obj_get_user_data(s_set_editor);
    if (i == SI_TIME) {
        if (s_time_field < 4) {
            s_time_field++;
            char buf[32];
            time_field_label(buf, sizeof(buf));
            ui_set_text(s_set_ed_value, buf);
            return;
        }
        s_time_edit.sec = 0;
        s_time_edit.dotw = 1;
        rtc_set(&s_time_edit);
    }
    editor_close();
}

static void set_up_clicked(lv_event_t *e)
{
    (void)e;
    if (s_set_offset > 0) {
        s_set_offset--;
        refresh_settings();
    }
}

static void set_down_clicked(lv_event_t *e)
{
    (void)e;
    if (s_set_offset + SET_ROWS < SI_COUNT) {
        s_set_offset++;
        refresh_settings();
    }
}

/* ---------------- page building ---------------- */

static void build_home(void)
{
    s_page[0] = make_container(lv_screen_active(), 0, 0, CONTENT_W, CONTENT_H);
    s_home_time = make_label(s_page[0], &lv_font_montserrat_48, lv_color_black());
    lv_obj_align(s_home_time, LV_ALIGN_TOP_MID, 0, 4);
    s_home_date = make_label(s_page[0], &lv_font_montserrat_16, lv_color_black());
    lv_obj_align(s_home_date, LV_ALIGN_TOP_MID, 0, 68);
    s_home_temp = make_label(s_page[0], &lv_font_montserrat_20, lv_color_black());
    lv_obj_align(s_home_temp, LV_ALIGN_TOP_MID, 0, 104);
    s_home_batt = make_label(s_page[0], &lv_font_montserrat_14, lv_color_black());
    lv_obj_align(s_home_batt, LV_ALIGN_TOP_RIGHT, -4, 4);
}

static void build_record(void)
{
    s_page[1] = make_container(lv_screen_active(), 0, 0, CONTENT_W, CONTENT_H);
    s_rec_symbol = lv_obj_create(s_page[1]);
    lv_obj_set_style_bg_color(s_rec_symbol, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(s_rec_symbol, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_rec_symbol, 0, 0);
    lv_obj_remove_flag(s_rec_symbol, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(s_rec_symbol, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_rec_symbol, rec_symbol_clicked, LV_EVENT_CLICKED, NULL);
    set_rec_symbol(false);

    s_rec_status = make_label(s_page[1], &lv_font_montserrat_20, lv_color_black());
    lv_obj_align(s_rec_status, LV_ALIGN_TOP_MID, 0, 108);
    s_rec_name = make_label(s_page[1], &lv_font_montserrat_12, lv_color_black());
    lv_obj_align(s_rec_name, LV_ALIGN_TOP_MID, 0, 134);
    lv_label_set_long_mode(s_rec_name, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(s_rec_name, CONTENT_W - 8);
}

static void build_files(void)
{
    s_page[2] = make_container(lv_screen_active(), 0, 0, CONTENT_W, CONTENT_H);

    s_files_list = make_container(s_page[2], 0, 0, CONTENT_W, CONTENT_H);
    s_files_header = make_label(s_files_list, &lv_font_montserrat_14, lv_color_black());
    lv_obj_align(s_files_header, LV_ALIGN_TOP_LEFT, 4, 4);

    lv_obj_t *up = make_button(s_files_list, &lv_font_montserrat_16);
    lv_obj_set_size(up, 32, 46);
    lv_obj_align(up, LV_ALIGN_TOP_RIGHT, 0, 2);
    lv_obj_t *upl = make_label(up, &lv_font_montserrat_16, lv_color_black());
    ui_set_text(upl, LV_SYMBOL_UP);
    lv_obj_center(upl);
    lv_obj_add_event_cb(up, files_up_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *down = make_button(s_files_list, &lv_font_montserrat_16);
    lv_obj_set_size(down, 32, 46);
    lv_obj_align(down, LV_ALIGN_BOTTOM_RIGHT, 0, -2);
    lv_obj_t *downl = make_label(down, &lv_font_montserrat_16, lv_color_black());
    ui_set_text(downl, LV_SYMBOL_DOWN);
    lv_obj_center(downl);
    lv_obj_add_event_cb(down, files_down_clicked, LV_EVENT_CLICKED, NULL);

    for (int i = 0; i < FILES_ROWS; i++) {
        lv_obj_t *r = make_container(s_files_list, 2, 22 + i * 26, CONTENT_W - 34, 24);
        lv_obj_set_style_border_width(r, 1, 0);
        lv_obj_set_style_border_color(r, lv_color_black(), 0);
        lv_obj_add_flag(r, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(r, file_row_clicked, LV_EVENT_CLICKED, NULL);
        s_row[i] = r;
        s_row_name[i] = make_label(r, &lv_font_montserrat_14, lv_color_black());
        lv_obj_align(s_row_name[i], LV_ALIGN_LEFT_MID, 2, 0);
        lv_label_set_long_mode(s_row_name[i], LV_LABEL_LONG_MODE_DOTS);
        lv_obj_set_width(s_row_name[i], 110);
        s_row_size[i] = make_label(r, &lv_font_montserrat_12, lv_color_black());
        lv_obj_align(s_row_size[i], LV_ALIGN_RIGHT_MID, -2, 0);
    }

    /* player view */
    s_player = make_container(s_page[2], 0, 0, CONTENT_W, CONTENT_H);
    s_player_title = make_label(s_player, &lv_font_montserrat_14, lv_color_black());
    lv_obj_align(s_player_title, LV_ALIGN_TOP_MID, 0, 6);
    lv_label_set_long_mode(s_player_title, LV_LABEL_LONG_MODE_DOTS);
    lv_obj_set_width(s_player_title, CONTENT_W - 8);
    s_player_info = make_label(s_player, &lv_font_montserrat_12, lv_color_black());
    lv_obj_align(s_player_info, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *pbtn = make_button(s_player, &lv_font_montserrat_20);
    lv_obj_set_size(pbtn, 64, 64);
    lv_obj_align(pbtn, LV_ALIGN_CENTER, 0, 8);
    s_player_btn_txt = make_label(pbtn, &lv_font_montserrat_20, lv_color_black());
    ui_set_text(s_player_btn_txt, LV_SYMBOL_PLAY);
    lv_obj_center(s_player_btn_txt);
    lv_obj_add_event_cb(pbtn, player_btn_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbtn = make_button(s_player, &lv_font_montserrat_14);
    lv_obj_set_size(lbtn, 60, 26);
    lv_obj_align(lbtn, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_t *ll = make_label(lbtn, &lv_font_montserrat_14, lv_color_black());
    ui_set_text(ll, "List");
    lv_obj_center(ll);
    lv_obj_add_event_cb(lbtn, player_list_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(s_player, LV_OBJ_FLAG_HIDDEN);
}

static void build_settings(void)
{
    s_page[3] = make_container(lv_screen_active(), 0, 0, CONTENT_W, CONTENT_H);

    s_set_list = make_container(s_page[3], 0, 0, CONTENT_W, CONTENT_H);
    lv_obj_t *title = make_label(s_set_list, &lv_font_montserrat_14, lv_color_black());
    ui_set_text(title, "Settings");
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 4, 4);

    lv_obj_t *up = make_button(s_set_list, &lv_font_montserrat_16);
    lv_obj_set_size(up, 32, 46);
    lv_obj_align(up, LV_ALIGN_TOP_RIGHT, 0, 2);
    lv_obj_t *upl = make_label(up, &lv_font_montserrat_16, lv_color_black());
    ui_set_text(upl, LV_SYMBOL_UP);
    lv_obj_center(upl);
    lv_obj_add_event_cb(up, set_up_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *down = make_button(s_set_list, &lv_font_montserrat_16);
    lv_obj_set_size(down, 32, 46);
    lv_obj_align(down, LV_ALIGN_BOTTOM_RIGHT, 0, -2);
    lv_obj_t *downl = make_label(down, &lv_font_montserrat_16, lv_color_black());
    ui_set_text(downl, LV_SYMBOL_DOWN);
    lv_obj_center(downl);
    lv_obj_add_event_cb(down, set_down_clicked, LV_EVENT_CLICKED, NULL);

    for (int i = 0; i < SET_ROWS; i++) {
        lv_obj_t *r = make_container(s_set_list, 2, 22 + i * 25, CONTENT_W - 34, 23);
        lv_obj_set_style_border_width(r, 1, 0);
        lv_obj_set_style_border_color(r, lv_color_black(), 0);
        lv_obj_add_flag(r, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(r, setting_row_clicked, LV_EVENT_CLICKED, NULL);
        s_set_row[i] = r;
        make_label(r, &lv_font_montserrat_14, lv_color_black());
        lv_obj_align(lv_obj_get_child(r, 0), LV_ALIGN_LEFT_MID, 2, 0);
        s_set_val[i] = make_label(r, &lv_font_montserrat_12, lv_color_black());
        lv_obj_align(s_set_val[i], LV_ALIGN_RIGHT_MID, -2, 0);
    }

    /* editor overlay */
    s_set_editor = make_container(s_page[3], 0, 0, CONTENT_W, CONTENT_H);
    lv_obj_set_style_border_width(s_set_editor, 2, 0);
    lv_obj_set_style_border_color(s_set_editor, lv_color_black(), 0);
    s_set_ed_title = make_label(s_set_editor, &lv_font_montserrat_14, lv_color_black());
    lv_obj_align(s_set_ed_title, LV_ALIGN_TOP_MID, 0, 8);
    s_set_ed_value = make_label(s_set_editor, &lv_font_montserrat_28, lv_color_black());
    lv_obj_align(s_set_ed_value, LV_ALIGN_CENTER, 0, -8);

    lv_obj_t *minus = make_button(s_set_editor, &lv_font_montserrat_20);
    lv_obj_set_size(minus, 48, 34);
    lv_obj_align(minus, LV_ALIGN_BOTTOM_LEFT, 6, -6);
    lv_obj_t *ml = make_label(minus, &lv_font_montserrat_20, lv_color_black());
    ui_set_text(ml, "-");
    lv_obj_center(ml);
    lv_obj_add_event_cb(minus, editor_minus, LV_EVENT_CLICKED, NULL);

    lv_obj_t *ok = make_button(s_set_editor, &lv_font_montserrat_16);
    lv_obj_set_size(ok, 56, 34);
    lv_obj_align(ok, LV_ALIGN_BOTTOM_MID, 0, -6);
    lv_obj_t *ol = make_label(ok, &lv_font_montserrat_16, lv_color_black());
    ui_set_text(ol, "OK");
    lv_obj_center(ol);
    lv_obj_add_event_cb(ok, editor_ok, LV_EVENT_CLICKED, NULL);

    lv_obj_t *plus = make_button(s_set_editor, &lv_font_montserrat_20);
    lv_obj_set_size(plus, 48, 34);
    lv_obj_align(plus, LV_ALIGN_BOTTOM_RIGHT, -6, -6);
    lv_obj_t *pl = make_label(plus, &lv_font_montserrat_20, lv_color_black());
    ui_set_text(pl, "+");
    lv_obj_center(pl);
    lv_obj_add_event_cb(plus, editor_plus, LV_EVENT_CLICKED, NULL);

    lv_obj_add_flag(s_set_editor, LV_OBJ_FLAG_HIDDEN);
}

static void build_bar(void)
{
    const char *syms[4] = {SYM_HOME, SYM_SET, SYM_PREV, SYM_NEXT};
    for (int i = 0; i < 4; i++) {
        lv_obj_t *slot = make_container(lv_screen_active(), i * SLOT_W, BAR_Y, SLOT_W, BAR_H);
        lv_obj_set_style_border_width(slot, 1, 0);
        lv_obj_set_style_border_color(slot, lv_color_hex(0x808080), 0);
        lv_obj_add_flag(slot, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(slot, bar_clicked, LV_EVENT_CLICKED, NULL);
        s_slot[i] = slot;
        s_slot_txt[i] = make_label(slot, &lv_font_montserrat_20, lv_color_black());
        ui_set_text(s_slot_txt[i], syms[i]);
        lv_obj_center(s_slot_txt[i]);
    }
}

static void refresh_page(int idx)
{
    switch (idx) {
    case 0: refresh_home(); break;
    case 1: refresh_record(); break;
    case 2:
        render_files();
        refresh_player();
        break;
    case 3: refresh_settings(); break;
    default: break;
    }
}

/* ---------------- indev ---------------- */

static void indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    static uint16_t lx, ly;
    static bool was_pressed;
    uint16_t x, y;
    if (touch_read_point(&x, &y)) {
        lx = x;
        ly = y;
        data->state = LV_INDEV_STATE_PRESSED;
        if (!was_pressed) {
            app_state_note_activity();
        }
        was_pressed = true;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
        was_pressed = false;
    }
    data->point.x = lx;
    data->point.y = ly;
    (void)indev;
}

/* ---------------- public ---------------- */

void ui_pages_init(void)
{
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    build_home();
    build_record();
    build_files();
    build_settings();
    build_bar();

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, indev_read_cb);

    s_page_idx = 0;
    s_set_offset = 0;
    load_files();
    show_page(0, true);
    s_last_version = app_state_version();
    s_last_state = app_state_state();
}

void ui_pages_poll(void)
{
    app_state_t st = app_state_state();
    bool state_changed = (st != s_last_state);

    uint32_t v = app_state_version();
    if (v == s_last_version && !state_changed) {
        return;
    }
    s_last_version = v;
    s_last_state = st;

    if (s_page_idx == 0) {
        refresh_home();
    } else if (s_page_idx == 1) {
        refresh_record();
    } else if (s_page_idx == 2) {
        if (state_changed) {
            load_files();
            render_files();
        }
        refresh_player();
    } else if (s_page_idx == 3) {
        refresh_settings();
    }
}
