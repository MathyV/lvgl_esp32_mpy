from .hardware import display

import lvgl as lv
import lvgl_esp32

wrapper = lvgl_esp32.Wrapper(display)
wrapper.init()

screen = lv.screen_active()
screen.set_style_bg_color(lv.color_hex(0x003a57), lv.PART.MAIN)

label = lv.label(screen)
label.set_text("Hello world from MicroPython")
label.set_style_text_color(lv.color_hex(0xffffff), lv.PART.MAIN)
label.align(lv.ALIGN.CENTER, 0, 0)

a = lv.anim_t()
a.init()
a.set_var(label)
a.set_values(10, 50)
a.set_duration(1000)
a.set_playback_delay(100)
a.set_playback_duration(300)
a.set_repeat_delay(500)
a.set_repeat_count(lv.ANIM_REPEAT_INFINITE)
a.set_path_cb(lv.anim_t.path_ease_in_out)
a.set_custom_exec_cb(lambda _, v: label.set_y(v))
a.start()

while True:
    lv.timer_handler_run_in_period(5)
