#include "wrapper.h"

#include "esp_log.h"
#include "py/runtime.h"

#define LVGL_TICK_PERIOD_MS     2

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

static void tick(void *arg)
{
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}


static mp_obj_t lvgl_esp32_Wrapper_init(mp_obj_t self_ptr)
{
    lvgl_esp32_Wrapper_obj_t *self = MP_OBJ_TO_PTR(self_ptr);

    ESP_LOGI(TAG, "Initializing LVGL Wrapper");

    if (!lv_is_initialized())
    {
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


    ESP_LOGI(TAG, "Installing LVGL tick timer");
    const esp_timer_create_args_t lvgl_tick_timer_args = {
        .callback = &tick,
        .dispatch_method = ESP_TIMER_ISR,
        .name = "lvgl_tick"
    };

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &self->timer_tick));
    ESP_ERROR_CHECK(esp_timer_start_periodic(self->timer_tick, LVGL_TICK_PERIOD_MS * 1000));

    return mp_obj_new_int_from_uint(0);
}
static MP_DEFINE_CONST_FUN_OBJ_1(lvgl_esp32_Wrapper_init_obj, lvgl_esp32_Wrapper_init);

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

    lvgl_esp32_Wrapper_obj_t *self = mp_obj_malloc(lvgl_esp32_Wrapper_obj_t, type);
    self->base.type = &lvgl_esp32_Wrapper_type;

    if (mp_obj_get_type(args[ARG_display].u_obj) != &lvgl_esp32_Display_type)
    {
        mp_raise_ValueError(MP_ERROR_TEXT("Expecting a Display object"));
    }

    self->display = (lvgl_esp32_Display_obj_t *) MP_OBJ_TO_PTR(args[ARG_display].u_obj);

    self->buf_size = 0;
    self->buf1 = NULL;
    self->buf2 = NULL;

    self->lv_display = NULL;
    self->timer_tick = NULL;

    return MP_OBJ_FROM_PTR(self);
}

static const mp_rom_map_elem_t lvgl_esp32_Wrapper_locals_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&lvgl_esp32_Wrapper_init_obj) }
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
