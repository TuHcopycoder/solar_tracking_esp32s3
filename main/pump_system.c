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
    
    // Đảm bảo bơm tắt khi vừa khởi động (Mức 1 là tắt đối với module kích mức thấp)
    pump_control(false); 
}

void pump_control(bool state) {
    /* 
       LƯU Ý: Nếu bạn dùng Module Relay có Jumper set ở 'L' (Low):
       - state = true  (Bật) -> Xuất mức 0 (LOW)
       - state = false (Tắt) -> Xuất mức 1 (HIGH)
    */
    gpio_set_level(PUMP_PIN, state ? 1 : 0); 
}    
