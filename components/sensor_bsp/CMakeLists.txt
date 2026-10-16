idf_component_register(
    SRCS "sensor_bsp.cpp"
    PRIV_REQUIRES main
    REQUIRES i2c_bus esp_driver_i2c
    INCLUDE_DIRS "./")
