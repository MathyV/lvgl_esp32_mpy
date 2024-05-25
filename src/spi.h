#ifndef __LVGL_ESP32_SPI_H__
#define __LVGL_ESP32_SPI_H__

#include "py/obj.h"

#include "hal/spi_types.h"

typedef struct lvgl_esp32_SPI_obj_t
{
    mp_obj_base_t base;

    spi_host_device_t spi_host_device;
    uint32_t baudrate;

    uint8_t sck;
    uint8_t mosi;
    uint8_t miso;

    bool bus_initialized;
    bool needs_deinit;

    // This is a very hacky solution that will only work as long as the SPI devices are internal to this module
    // If this PR gets merged and released, this hack should be removed
    // https://github.com/espressif/esp-idf/pull/13856
    uint8_t device_count;
} lvgl_esp32_SPI_obj_t;

void lvgl_esp32_SPI_internal_deinit(lvgl_esp32_SPI_obj_t* self);

extern const mp_obj_type_t lvgl_esp32_SPI_type;

#endif /* __LVGL_ESP32_SPI_H__ */
