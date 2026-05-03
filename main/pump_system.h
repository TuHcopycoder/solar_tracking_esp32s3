#ifndef PUMP_SYSTEM_H
#define PUMP_SYSTEM_H

#include <stdbool.h>

// Định nghĩa chân (Sửa lại số chân theo thực tế của bạn)
#define PUMP_PIN 15 

void pump_init(); // Phải có dòng này
void pump_control(bool state);

#endif