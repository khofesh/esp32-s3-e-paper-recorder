idf_component_register(SRCS "main.cpp"
                    INCLUDE_DIRS "./")

# RTC sync from build time; knobs in <project>/.env (see .env.example)
set(RTC_SYNC 1)
set(RTC_SYNC_OFFSET_S 30)
set(ENV_FILE "${CMAKE_SOURCE_DIR}/.env")
if(EXISTS "${ENV_FILE}")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${ENV_FILE}")
    file(STRINGS "${ENV_FILE}" env_lines)
    foreach(l IN LISTS env_lines)
        if(l MATCHES "^[ \t]*(RTC_SYNC|RTC_SYNC_OFFSET_S)[ \t]*=[ \t]*([0-9]+)")
            set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
        endif()
    endforeach()
endif()

set(BUILD_TIME_H "${CMAKE_CURRENT_BINARY_DIR}/build_time.h")
add_custom_target(gen_build_time ALL
    COMMAND ${CMAKE_COMMAND} -DOUT=${BUILD_TIME_H} -DRTC_SYNC=${RTC_SYNC}
            -DRTC_SYNC_OFFSET_S=${RTC_SYNC_OFFSET_S} -P ${CMAKE_CURRENT_SOURCE_DIR}/gen_build_time.cmake
    BYPRODUCTS ${BUILD_TIME_H}
    VERBATIM)
add_dependencies(${COMPONENT_LIB} gen_build_time)
target_include_directories(${COMPONENT_LIB} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
