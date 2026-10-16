idf_component_register(
    SRCS "audio_bsp.cpp"
    PRIV_REQUIRES main
    REQUIRES codec_board esp_codec_dev
    INCLUDE_DIRS "./")
