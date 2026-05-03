#ifndef LDR_LOGIC_H
#define LDR_LOGIC_H
#define SUN_LIMIT   400
#include "esp_adc/adc_oneshot.h"

/**
 * Cấu hình Kênh ADC cho ESP32-S3 (ADC1)
 * Lưu ý: Channel != GPIO Number
 */
// ldr_logic.h — SỬA LẠI
#define LDR_XP_CH  ADC_CHANNEL_3  // GPIO 4  (Trục X+, Phải) — giữ nguyên
#define LDR_XM_CH  ADC_CHANNEL_4  // GPIO 5  (Trục X-, Trái) — giữ nguyên
#define LDR_YP_CH  ADC_CHANNEL_6  // GPIO 7  (Trục Y+, Trên) ← ĐỔI
#define LDR_YM_CH  ADC_CHANNEL_7  // GPIO 8  (Trục Y-, Dưới) ← ĐỔI

void ldr_task(void *pvParameters); 

#endif