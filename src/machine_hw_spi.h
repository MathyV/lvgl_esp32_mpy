#include "hal/spi_types.h"
#include "py/obj.h"

// This function is defined in MicroPython in ports/esp32/machine_hw_spi.c
extern spi_host_device_t machine_hw_spi_get_host(mp_obj_t in);
