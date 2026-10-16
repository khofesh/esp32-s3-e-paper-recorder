idf_component_register(
    SRCS "rtc_bsp.cpp"
    PRIV_REQUIRES main
    REQUIRES i2c_bus waveshare__pcf85063a
    INCLUDE_DIRS "./")
