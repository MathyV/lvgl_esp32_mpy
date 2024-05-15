#ifndef __LVGL_ESP32_LVGL_INIT__
#define __LVGL_ESP32_LVGL_INIT__

#include "display.h"

#include "lvgl.h"
#include "esp_timer.h"
#include "py/obj.h"

typedef struct lvgl_esp32_Wrapper_obj_t
{
    mp_obj_base_t base;
    lvgl_esp32_Display_obj_t *display;

    size_t buf_size;
    uint16_t *buf1;
    uint16_t *buf2;

    lv_display_t *lv_display;
    esp_timer_handle_t timer_tick;
} lvgl_esp32_Wrapper_obj_t;

extern const mp_obj_type_t lvgl_esp32_Wrapper_type;

#endif /* __LVGL_ESP32_LVGL_INIT__ */
