#include "bme_reader.h"
#include <Wire.h>

bool BmeReader::begin() {
    Wire.begin(21, 22);
    if (bme_.begin(0x76)) {
        initialized_ = true;
        return true;
    }
    if (bme_.begin(0x77)) {
        initialized_ = true;
        return true;
    }
    return false;
}

BmeData BmeReader::read() {
    BmeData d;
    if (!initialized_) {
        d.valid = false;
        return d;
    }
    d.temp = bme_.readTemperature();
    d.humi = bme_.readHumidity();
    d.pres = bme_.readPressure() / 100.0f;
    d.valid = true;
    return d;
}
