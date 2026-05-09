#include <stdio.h>
#include "freertos/FreeRTOS.h"  
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/semphr.h"
#include "i2c_lcd.h"
#include "ntc_sensor.h"
#include "ldr_logic.h"
#include "motor_control.h"
#include "pump_system.h"

// Biến toàn cục lấy từ ldr_logic.c sang
extern int g_lcd_max_light;
extern bool g_lcd_is_tracking;

static const char *TAG = "MAIN_SYSTEM";

// --- CẬP NHẬT NGƯỠNG NHIỆT ĐỘ (VÙNG TRỄ CHỐNG CHÁY BƠM) ---
#define TEMP_TURN_ON   40.0f    // Trên 40 độ -> Bật bơm
#define TEMP_TURN_OFF  38.0f    // Dưới 38 độ -> Tắt bơm
#define LIGHT_CHECK_MS  3000    // Kiểm tra ánh sáng mỗi 3 giây

SemaphoreHandle_t adc_mutex;
TaskHandle_t ldr_task_handle = NULL;
static adc_oneshot_unit_handle_t g_adc_handle;

// ─── BIẾN TOÀN CỤC CHO HỆ THỐNG LÀM MÁT ───
static bool is_pumping = false; 
static float avg_temp = 25.0f; 
static bool is_first_read = true; 

// ─── TIMER 1: Cooling – chạy mỗi 5 giây ────────────────────────────
static void cooling_timer_callback(TimerHandle_t xTimer) {
    float current_temp = 0.0f;

    // Đọc ADC có Mutex bảo vệ
    if (xSemaphoreTake(adc_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        current_temp = ntc_read_temp(g_adc_handle);
        xSemaphoreGive(adc_mutex);
    }

    // Bỏ qua tính toán nếu lỗi cảm biến
    if (current_temp == -99.0f) {
        ESP_LOGW(TAG, "Lỗi đọc cảm biến NTC, bỏ qua chu kỳ này.");
        return; 
    }

    // --- BỘ LỌC CHỐNG NHIỄU NTC ---
    if (is_first_read) {
        avg_temp = current_temp; // Lấy giá trị đầu tiên làm gốc
        is_first_read = false;
    } else {
        // Lấy 80% giá trị cũ + 20% giá trị mới để ủi phẳng nhiễu
        avg_temp = (avg_temp * 0.8f) + (current_temp * 0.2f);
    }

    // --- LOGIC ĐIỀU KHIỂN BƠM (Chỉ dùng nhiệt độ) ---
    // Áp dụng Vùng trễ (Hysteresis) 
    if (!is_pumping && avg_temp > TEMP_TURN_ON) {
        is_pumping = true;
      pump_control(true);
        ESP_LOGI(TAG, "Nhiệt độ cao (%.1f°C), KÍCH HOẠT phun sương...", avg_temp);
    } 
    else if (is_pumping && avg_temp < TEMP_TURN_OFF) {
        is_pumping = false;
        pump_control(false);
        ESP_LOGI(TAG, "Đã làm mát xong (%.1f°C), TẮT bơm.", avg_temp);
    }
}

// ─── TIMER 2: Check ánh sáng – resume ldr_task nếu trời sáng lại ───
static void light_check_callback(TimerHandle_t xTimer) {
    int val = 0;

    // Đọc thử 1 kênh LDR bất kỳ để check ánh sáng
    if (xSemaphoreTake(adc_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        adc_oneshot_read(g_adc_handle, LDR_XP_CH, &val);
        xSemaphoreGive(adc_mutex);
    }

    if (val >= SUN_LIMIT) {
        // Trời sáng rồi → resume ldr_task nếu đang bị suspend
        if (eTaskGetState(ldr_task_handle) == eSuspended) {
            ESP_LOGI(TAG, "Trời sáng (val=%d), resume LDR Task!", val);
            vTaskResume(ldr_task_handle);
        }
    }
}

// ─── TASK LCD: Hiển thị thông số ────────────────────────────────────
void lcd_task(void *pvParameters) {
    ESP_LOGI("LCD", "Khởi tạo LCD I2C...");
    
    i2c_master_init(); // Khởi tạo chân I2C
    lcd_init();        // Gửi lệnh khởi động màn hình
    lcd_clear();
    
    char buffer[17]; // Buffer chứa 16 ký tự + \0 của LCD 16x2
    
    while (1) {
        // --- Dòng 1: Nhiệt độ & Bơm (Ví dụ: "T:40.5C Pmp:ON ") ---
        // Dùng %5.1f để cố định độ rộng 5 khoảng trống cho nhiệt độ
snprintf(buffer, sizeof(buffer), "T:%5.1fC P:%-3s", avg_temp, is_pumping ? "ON" : "OFF");
        lcd_put_cur(0, 0);
        lcd_send_string(buffer);
        
        // --- Dòng 2: Ánh sáng & Trạng thái dò (Ví dụ: "L:2500 Trk:RUN ") ---
        sprintf(buffer, "L:%-4d Trk:%s ", g_lcd_max_light, g_lcd_is_tracking ? "RUN" : "OFF");
        lcd_put_cur(1, 0);
        lcd_send_string(buffer);
        
        // Cập nhật màn hình 1 giây/lần
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Khởi động Hệ thống Tối ưu Pin Năng lượng Mặt trời...");
    
    motor_init();
    pump_init();
   
    
    adc_oneshot_unit_init_cfg_t init_config = { .unit_id = ADC_UNIT_1 };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &g_adc_handle));
    
    adc_oneshot_chan_cfg_t adc_config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten    = ADC_ATTEN_DB_12,
    };
    
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_adc_handle, NTC_ADC_CHANNEL, &adc_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_adc_handle, LDR_XP_CH,       &adc_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_adc_handle, LDR_XM_CH,       &adc_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_adc_handle, LDR_YP_CH,       &adc_config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(g_adc_handle, LDR_YM_CH,       &adc_config));
    
    adc_mutex = xSemaphoreCreateMutex();
    assert(adc_mutex != NULL);
    
    xTaskCreate(ldr_task, "Sun_Tracking", 4096, (void*)g_adc_handle, 5, &ldr_task_handle);
    
    xTaskCreate(lcd_task, "LCD_Task", 4096, NULL, 2, NULL);
    
    // Timer 1: Cooling mỗi 5 giây
    TimerHandle_t cooling_timer = xTimerCreate(
        "CoolingTimer", pdMS_TO_TICKS(5000), pdTRUE, NULL, cooling_timer_callback
    );
    assert(cooling_timer != NULL);
    xTimerStart(cooling_timer, 0);

    // Timer 2: Check ánh sáng mỗi 3 giây để resume ldr_task
    TimerHandle_t light_timer = xTimerCreate(
        "LightCheck", pdMS_TO_TICKS(LIGHT_CHECK_MS), pdTRUE, NULL, light_check_callback
    );
    assert(light_timer != NULL);
    xTimerStart(light_timer, 0);
    
    ESP_LOGI(TAG, "Hệ thống đã sẵn sàng.");
    
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
