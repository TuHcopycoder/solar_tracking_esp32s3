#include "ntc_sensor.h"
#include <math.h>
#include "esp_log.h"

static const char *TAG = "NTC_SENSOR";

// Thông số kỹ thuật của cảm biến nhiệt độ NTC 10k phổ thông
#define R_NOMINAL    10000.0f  // Trở kháng 10k ở 25 độ C
#define T_NOMINAL    298.15f   // Nhiệt độ chuẩn Kelvin (25 + 273.15)
#define BETA         3950.0f   // Hệ số Beta của NTC
#define R_REF        10000.0f  // Điện trở phân áp 10k
#define ADC_MAX      4095.0f   // Phân giải ADC 12-bit của ESP32-S3

float ntc_read_temp(adc_oneshot_unit_handle_t adc_handle) {
    int raw_adc;
    // Đọc giá trị ADC thô
    esp_err_t ret = adc_oneshot_read(adc_handle, NTC_ADC_CHANNEL, &raw_adc);

    if (ret != ESP_OK || raw_adc <= 0 || raw_adc >= 4095) {
        ESP_LOGW(TAG, "Lỗi đọc ADC hoặc đứt dây/ngắn mạch (ADC: %d)", raw_adc);
        return -99.0f; 
    }

    // --- TÍNH TOÁN CHO SƠ ĐỒ: 3.3V -> R_REF -> NTC -> GND ---
    // Điện áp tại chân ADC tỉ lệ thuận với điện trở NTC
    // V_out = V_in * (R_ntc / (R_ref + R_ntc))
    // => R_ntc = R_ref / ( (ADC_MAX / raw_adc) - 1 )
    float r_ntc = R_REF / ((ADC_MAX / (float)raw_adc) - 1.0f);

    // --- Tính nhiệt độ theo công thức Steinhart-Hart ---
    float temp;
    temp = r_ntc / R_NOMINAL;          // (R / Ro)
    temp = log(temp);                  // ln(R / Ro)
    temp /= BETA;                      // 1/B * ln(R / Ro)
    temp += 1.0f / T_NOMINAL;          // + (1 / To)
    temp = 1.0f / temp;                // Nghịch đảo ra Kelvin
    temp -= 273.15f;                   // Đổi Kelvin sang Celsius

    ESP_LOGI(TAG, "ADC Raw: %d | R_NTC: %.1f | Temp: %.2f C", raw_adc, r_ntc, temp);
    return temp;
}
