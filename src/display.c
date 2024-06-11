#include "display.h"

#include "py/runtime.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_timer.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_err.h"
#include "esp_log.h"

static const char *TAG = "lvgl_esp32_display";

// Bit number used to represent command and parameter
#define LCD_CMD_BITS           8
#define LCD_PARAM_BITS         8

static bool on_color_trans_done_cb(
    esp_lcd_panel_io_handle_t panel_io,
    esp_lcd_panel_io_event_data_t *edata,
    void *user_ctx
)
{
    lvgl_esp32_Display_obj_t *self = (lvgl_esp32_Display_obj_t *) user_ctx;

    if (self->transfer_done_cb != NULL)
    {
        self->transfer_done_cb(self->transfer_done_user_data);
    }

    return false;
}

void lvgl_esp32_Display_draw_bitmap(
    lvgl_esp32_Display_obj_t *self,
    int x_start,
    int y_start,
    int x_end,
    int y_end,
    const void *data
)
{
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(self->panel, x_start, y_start, x_end, y_end, data));
}

static void clear(lvgl_esp32_Display_obj_t *self)
{
    ESP_LOGI(TAG, "Clearing screen");

    // Create a temporary empty buffer of only one line of pixels so this will also work on memory-constrained devices
    size_t buf_size = self->width;
    uint16_t *buf = heap_caps_calloc(1, buf_size * sizeof(uint16_t), MALLOC_CAP_DMA);

    assert(buf);

    // Blit lines to the screen
    for (int line = 0; line < self->height; line++)
    {
        lvgl_esp32_Display_draw_bitmap(self, 0, line, self->width, line + 1, buf);
    }

    // Release the buffer
    heap_caps_free(buf);
}

static mp_obj_t lvgl_esp32_Display_init(mp_obj_t self_ptr)
{
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_LOGI(TAG, "Setting up panel IO");
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = self->dc,
        .cs_gpio_num = self->cs,
        .pclk_hz = self->pixel_clock,
        .lcd_cmd_bits = LCD_CMD_BITS,
        .lcd_param_bits = LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = on_color_trans_done_cb,
        .user_ctx = self,
    };

    ESP_ERROR_CHECK(
        esp_lcd_new_panel_io_spi(
            (esp_lcd_spi_bus_handle_t) self->spi->spi_host_device,
            &io_config,
            &self->io_handle
        )
    );

    // HACK
    self->spi->device_count++;

    ESP_LOGI(TAG, "Setting up ST7789 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = self->reset,
        .rgb_ele_order = self->bgr ? LCD_RGB_ELEMENT_ORDER_BGR : LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(self->io_handle, &panel_config, &self->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(self->panel));
    ESP_ERROR_CHECK(esp_lcd_panel_init(self->panel));

    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(self->panel, self->invert));
    ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(self->panel, self->swap_xy));
	ESP_ERROR_CHECK(esp_lcd_panel_mirror(self->panel, self->mirror_x, self->mirror_y));

    clear(self);

    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(self->panel, true));

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_Display_init_obj, lvgl_esp32_Display_init);

static mp_obj_t lvgl_esp32_Display_deinit(mp_obj_t self_ptr)
{
    lvgl_esp32_Display_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    if(self->panel != NULL)
    {
        ESP_LOGI(TAG, "Deinitializing ST7789 panel driver");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(self->panel, false));
        ESP_ERROR_CHECK(esp_lcd_panel_del(self->panel));
        self->panel = NULL;
    }

    if(self->io_handle != NULL)
    {
        ESP_LOGI(TAG, "Deinitializing panel IO");
        ESP_ERROR_CHECK(esp_lcd_panel_io_del(self->io_handle));
        self->io_handle = NULL;

        // HACK
        self->spi->device_count--;

        // We call deinit on spi in case it was (unsuccessfully) deleted earlier
        lvgl_esp32_SPI_internal_deinit(self->spi);
    }

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_Display_deinit_obj, lvgl_esp32_Display_deinit);

static mp_obj_t lvgl_esp32_Display_make_new(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *all_args
)
{
    enum
    {
        ARG_width,          // width of the display
        ARG_height,         // height of the display
        ARG_spi,            // configured SPI instance
        ARG_reset,          // RESET pin number
        ARG_dc,             // DC pin number
        ARG_cs,             // CS pin number
        ARG_pixel_clock,    // Pixel clock in Hz
        ARG_swap_xy,        // swap X and Y axis
        ARG_mirror_x,       // mirror on X axis
        ARG_mirror_y,       // mirror on Y axis
        ARG_invert,         // invert colors
        ARG_bgr,            // use BGR element order
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_width, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_height, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_spi, MP_ARG_OBJ | MP_ARG_REQUIRED },
        { MP_QSTR_reset, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_dc, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_cs, MP_ARG_INT | MP_ARG_REQUIRED },
        { MP_QSTR_pixel_clock, MP_ARG_INT | MP_ARG_KW_ONLY, { .u_int = 20 * 1000 * 1000 }},
        { MP_QSTR_swap_xy, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_mirror_x, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_mirror_y, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_invert, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
        { MP_QSTR_bgr, MP_ARG_BOOL | MP_ARG_KW_ONLY, { .u_bool = false }},
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    lvgl_esp32_Display_obj_t *self = mp_obj_malloc_with_finaliser(lvgl_esp32_Display_obj_t, &lvgl_esp32_Display_type);

    self->width = args[ARG_width].u_int;
    self->height = args[ARG_height].u_int;

    self->spi = (lvgl_esp32_SPI_obj_t *) MP_OBJ_TO_PTR(args[ARG_spi].u_obj);
    self->reset = args[ARG_reset].u_int;
    self->dc = args[ARG_dc].u_int;
    self->cs = args[ARG_cs].u_int;
    self->pixel_clock = args[ARG_pixel_clock].u_int;

    self->swap_xy = args[ARG_swap_xy].u_bool;
    self->mirror_x = args[ARG_mirror_x].u_bool;
    self->mirror_y = args[ARG_mirror_y].u_bool;
    self->invert = args[ARG_invert].u_bool;
    self->bgr = args[ARG_bgr].u_bool;

    self->transfer_done_cb = NULL;
    self->transfer_done_user_data = NULL;

    self->panel = NULL;
    self->io_handle = NULL;

    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t lvgl_esp32_Display_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&lvgl_esp32_Display_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&lvgl_esp32_Display_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&lvgl_esp32_Display_deinit_obj) },
};

static MP_DEFINE_CONST_DICT(lvgl_esp32_Display_locals, lvgl_esp32_Display_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    lvgl_esp32_Display_type,
    MP_QSTR_Display,
    MP_TYPE_FLAG_NONE,
    make_new,
    lvgl_esp32_Display_make_new,
    locals_dict,
    &lvgl_esp32_Display_locals
);
