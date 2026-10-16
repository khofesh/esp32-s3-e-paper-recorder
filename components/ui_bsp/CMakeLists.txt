file(GLOB_RECURSE scrld ./*.c)
idf_component_register(
  SRCS ${scrld}
  REQUIRES lvgl app_state settings storage_bsp battery_bsp rtc_bsp touch_bsp fatfs
  PRIV_REQUIRES main
  INCLUDE_DIRS "custom" "generated" "generated/guider_customer_fonts")
