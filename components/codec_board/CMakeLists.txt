set(COMPONENT_PRIV_REQUIRE driver esp_codec_dev fatfs esp_lcd esp_adc sdmmc)

if("${IDF_VERSION_MAJOR}.${IDF_VERSION_MINOR}" VERSION_GREATER_EQUAL "5.3")
list(APPEND COMPONENT_PRIV_REQUIRE esp_driver_i2c)
endif()

idf_component_register(SRCS  "cfg_parse.c" "codec_board.c" "codec_init.c" "dummy_codec.c"
                             "lcd_init.c" "drv/tca9554.c"
                       INCLUDE_DIRS ./include
                       PRIV_INCLUDE_DIRS drv
                       REQUIRES "${COMPONENT_PRIV_REQUIRE}" esp_adc esp_driver_i2s esp_driver_gpio esp_driver_spi
                       EMBED_TXTFILES "board_cfg.txt")
