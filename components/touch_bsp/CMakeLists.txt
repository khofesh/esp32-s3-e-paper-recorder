idf_component_register(
    SRCS "touch_bsp.cpp"
    PRIV_REQUIRES main
    REQUIRES i2c_bus esp_driver_gpio
    INCLUDE_DIRS "./")
