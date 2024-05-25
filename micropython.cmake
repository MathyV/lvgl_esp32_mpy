# Make sure LVGL gets built
include(${CMAKE_CURRENT_LIST_DIR}/binding/binding.cmake)

add_library(usermod_lvgl_esp32 INTERFACE)

target_sources(usermod_lvgl_esp32 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}/src/spi.c
        ${CMAKE_CURRENT_LIST_DIR}/src/display.c
        ${CMAKE_CURRENT_LIST_DIR}/src/wrapper.c
        ${CMAKE_CURRENT_LIST_DIR}/src/module.c
)

target_include_directories(usermod_lvgl_esp32 INTERFACE
        ${IDF_PATH}/components/esp_lcd/include/
        ${CMAKE_CURRENT_LIST_DIR}/binding/lvgl
        ${CMAKE_CURRENT_LIST_DIR}/binding/lvgl/src
)

target_link_libraries(usermod_lvgl_esp32 INTERFACE lvgl_interface)

target_link_libraries(usermod INTERFACE usermod_lvgl_esp32)
