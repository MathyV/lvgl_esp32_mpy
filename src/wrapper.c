#include "wrapper.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "py/runtime.h"

static const char *TAG = "lvgl_esp32_wrapper";

static void flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *data)
{
    lvgl_esp32_Wrapper_obj_t *self = (lvgl_esp32_Wrapper_obj_t *) lv_display_get_user_data(display);;

    // Correct byte order
    lv_draw_sw_rgb565_swap(data, self->buf_size);

    // Blit to the screen
    lvgl_esp32_Display_draw_bitmap(self->display, area->x1, area->y1, area->x2 + 1, area->y2 + 1, data);
}

static void transfer_done_cb(void *user_data)
{
    lvgl_esp32_Wrapper_obj_t *self = (lvgl_esp32_Wrapper_obj_t *) user_data;
    lv_disp_flush_ready(self->lv_display);
}

static uint32_t tick_get_cb()
{
    return esp_timer_get_time() / 1000;
}

static mp_obj_t lvgl_esp32_Wrapper_init(mp_obj_t self_ptr)
{
    lvgl_esp32_Wrapper_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_LOGI(TAG, "Initializing LVGL Wrapper");

    if (!lv_is_initialized())
    {
        ESP_LOGI(TAG, "Initializing LVGL library");
        lv_init();
    }

    ESP_LOGI(TAG, "Initializing LVGL display with size %dx%d", self->display->width, self->display->height);
    self->lv_display = lv_display_create(self->display->width, self->display->height);

    ESP_LOGI(TAG, "Creating display buffers");
    self->buf_size = self->display->width * 20;
    self->buf1 = heap_caps_malloc(self->buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(self->buf1);
    self->buf2 = heap_caps_malloc(self->buf_size * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(self->buf2);

    // initialize LVGL draw buffers
    lv_display_set_buffers(self->lv_display, self->buf1, self->buf2, self->buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    ESP_LOGI(TAG, "Registering callback functions");
    self->display->transfer_done_cb = transfer_done_cb;
    self->display->transfer_done_user_data = (void *) self;
    lv_display_set_flush_cb(self->lv_display, flush_cb);
    lv_display_set_user_data(self->lv_display, self);
    lv_tick_set_cb(tick_get_cb);

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_Wrapper_init_obj, lvgl_esp32_Wrapper_init);

static mp_obj_t lvgl_esp32_Wrapper_deinit(mp_obj_t self_ptr)
{
    lvgl_esp32_Wrapper_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_LOGI(TAG, "Deinitializing LVGL Wrapper");

    ESP_LOGI(TAG, "Disabling callback functions");
    lv_tick_set_cb(NULL);
    self->display->transfer_done_cb = NULL;
    self->display->transfer_done_user_data = NULL;

    if (self->lv_display != NULL)
    {
        ESP_LOGI(TAG, "Deleting LVGL display");
        lv_display_delete(self->lv_display);
        self->lv_display = NULL;
    }

    self->buf_size = 0;
    if (self->buf1 != NULL)
    {
        ESP_LOGI(TAG, "Freeing first display buffer");
        heap_caps_free(self->buf1);
        self->buf1 = NULL;
    }
    if (self->buf2 != NULL)
    {
        ESP_LOGI(TAG, "Freeing second display buffer");
        heap_caps_free(self->buf2);
        self->buf2 = NULL;
    }

    if (lv_is_initialized())
    {
        ESP_LOGI(TAG, "Deinitializing LVGL");
        lv_deinit();
    }

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_Wrapper_deinit_obj, lvgl_esp32_Wrapper_deinit);

static mp_obj_t lvgl_esp32_Wrapper_make_new(
    const mp_obj_type_t *type,
    size_t n_args,
    size_t n_kw,
    const mp_obj_t *all_args
)
{
    enum
    {
        ARG_display,      // a display instance
    };

    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_display, MP_ARG_OBJ | MP_ARG_REQUIRED },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    lvgl_esp32_Wrapper_obj_t *self = mp_obj_malloc_with_finaliser(lvgl_esp32_Wrapper_obj_t, &lvgl_esp32_Wrapper_type);

    if (mp_obj_get_type(args[ARG_display].u_obj) != &lvgl_esp32_Display_type)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("Expecting a Display object"));
    }

    self->display = (lvgl_esp32_Display_obj_t *) MP_OBJ_TO_PTR(args[ARG_display].u_obj);

    self->buf_size = 0;
    self->buf1 = NULL;
    self->buf2 = NULL;

    self->lv_display = NULL;

    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t lvgl_esp32_Wrapper_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&lvgl_esp32_Wrapper_init_obj) },
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&lvgl_esp32_Wrapper_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&lvgl_esp32_Wrapper_deinit_obj) },
};

static MP_DEFINE_CONST_DICT(lvgl_esp32_Wrapper_locals, lvgl_esp32_Wrapper_locals_table);

MP_DEFINE_CONST_OBJ_TYPE(
    lvgl_esp32_Wrapper_type,
    MP_QSTR_Wrapper,
    MP_TYPE_FLAG_NONE,
    make_new,
    lvgl_esp32_Wrapper_make_new,
    locals_dict,
    &lvgl_esp32_Wrapper_locals
);
