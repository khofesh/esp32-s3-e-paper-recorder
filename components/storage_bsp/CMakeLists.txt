idf_component_register(
    SRCS "storage_bsp.cpp"
    PRIV_REQUIRES main
    REQUIRES fatfs sdmmc esp_driver_sdmmc esp_driver_gpio
    INCLUDE_DIRS "./")
