#include "heater_pwm.h"

void HeaterPwm::begin() {
    ledcSetup(HEATER_PWM_CH, HEATER_PWM_FREQ, HEATER_PWM_RES);
    ledcAttachPin(HEATER_PIN, HEATER_PWM_CH);
    ledcWrite(HEATER_PWM_CH, 0);
    duty_ = 0;
}

void HeaterPwm::set(uint8_t duty) {
    duty_ = duty;
    ledcWrite(HEATER_PWM_CH, duty);
}

void HeaterPwm::off() {
    set(0);
}
