import lvgl_esp32

# Adapt these values for your own configuration
spi = lvgl_esp32.SPI(
    2,
    baudrate=80_000_000,
    sck=7,
    mosi=6,
    miso=8,
)
spi.init()

display = lvgl_esp32.Display(
    spi=spi,
    width=296,
    height=240,
    swap_xy=True,
    mirror_x=False,
    mirror_y=False,
    invert=False,
    bgr=True,
    reset=48,
    dc=4,
    cs=5,
    pixel_clock=20_000_000,
)
display.init()
