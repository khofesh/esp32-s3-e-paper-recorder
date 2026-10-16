idf_component_register(
  SRCS "user_app.cpp"
  PRIV_REQUIRES main
  REQUIRES
  board_power_bsp
  epaper_driver_bsp
  ui_bsp
  i2c_bus
  touch_bsp
  rtc_bsp
  sensor_bsp
  battery_bsp
  storage_bsp
  audio_bsp
  app_state
  settings
  esp_driver_gpio
  esp_hw_support
  esp_timer
  heap
  INCLUDE_DIRS ".")
