#include "pump_system.h"
#include "driver/gpio.h"

void pump_init(void) {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PUMP_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    
    // Đảm bảo bơm tắt khi vừa khởi động
    pump_control(false); 
}

void pump_control(bool state) {
    // Nếu Relay của bạn là loại kích mức THẤP (Low-level trigger) thì đổi ngược lại: state ? 0 : 1
    gpio_set_level(PUMP_PIN, state ? 1 : 0); 
}