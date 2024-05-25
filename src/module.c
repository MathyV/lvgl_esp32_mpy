#include "display.h"
#include "wrapper.h"
#include "spi.h"

static const mp_rom_map_elem_t lvgl_esp32_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_lvgl_esp32) },
    { MP_ROM_QSTR(MP_QSTR_SPI), MP_ROM_PTR(&lvgl_esp32_SPI_type) },
    { MP_ROM_QSTR(MP_QSTR_Display), MP_ROM_PTR(&lvgl_esp32_Display_type) },
    { MP_ROM_QSTR(MP_QSTR_Wrapper), MP_ROM_PTR(&lvgl_esp32_Wrapper_type) },
};
static MP_DEFINE_CONST_DICT(lvgl_esp32_globals, lvgl_esp32_globals_table);

const mp_obj_module_t lvgl_esp32_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t * ) & lvgl_esp32_globals,
};

MP_REGISTER_MODULE(MP_QSTR_lvgl_esp32, lvgl_esp32_module);
