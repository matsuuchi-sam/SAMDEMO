#ifndef HEATER_PWM_H
#define HEATER_PWM_H

#include <Arduino.h>

#define HEATER_PIN      26
#define HEATER_PWM_CH   0
#define HEATER_PWM_FREQ 25000
#define HEATER_PWM_RES  8

class HeaterPwm {
public:
    void begin();
    void set(uint8_t duty);
    uint8_t get() const { return duty_; }
    void off();
private:
    uint8_t duty_ = 0;
};

#endif
