#ifndef UI_PAGES_H
#define UI_PAGES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Builds the 4 pages and registers the touch input device.
 * Must be called with the LVGL mutex held. */
void ui_pages_init(void);

/* Drains pending state changes into the visible page.
 * Must be called from the LVGL task with the mutex held. */
void ui_pages_poll(void);

#ifdef __cplusplus
}
#endif

#endif
