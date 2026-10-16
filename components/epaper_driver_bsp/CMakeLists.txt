idf_component_register(
  SRCS "epaper_driver_bsp.cpp"
  PRIV_REQUIRES
  esp_driver_spi
  REQUIRES
  esp_driver_spi
  esp_driver_gpio
  INCLUDE_DIRS "./")
