#include "ntc_sensor.h"
#include <math.h>

// Thông số kỹ thuật của cảm biến nhiệt độ NTC 10k phổ thông
#define R_NOMINAL   10000.0f  // Trở kháng 10k ở 25 độ C
#define T_NOMINAL   298.15f   // Nhiệt độ chuẩn Kelvin (25 + 273.15)
#define BETA        3950.0f   // Hệ số Beta của NTC
#define R_REF       10000.0f  // Điện trở phân áp trên mạch (thường là 10k)
#define ADC_MAX     4095.0f   // Phân giải ADC 12-bit của ESP32-S3

float ntc_read_temp(adc_oneshot_unit_handle_t adc_handle) {
    int raw_adc;
    adc_oneshot_read(adc_handle, NTC_ADC_CHANNEL, &raw_adc);

    if (raw_adc == 0 || raw_adc >= 4095) {
        return -99.0f; // Lỗi không đọc được hoặc đứt dây
    }

    // --- Tính toán điện trở thực tế của NTC ---
    // (Giả định module của bạn nối theo chuẩn: 3.3V -> NTC -> Trở 10k -> GND)
    float r_ntc = R_REF * (ADC_MAX / (float)raw_adc - 1.0f);

    // --- Tính nhiệt độ ---
    float temp;
    temp = r_ntc / R_NOMINAL;         // (R / Ro)
    temp = log(temp);                 // ln(R / Ro)
    temp /= BETA;                     // 1/B * ln(R / Ro)
    temp += 1.0f / T_NOMINAL;         // + (1 / To)
    temp = 1.0f / temp;               // Nghịch đảo ra Kelvin
    temp -= 273.15f;                  // Đổi Kelvin sang Celsius

    return temp;
}