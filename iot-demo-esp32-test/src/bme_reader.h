#ifndef BME_READER_H
#define BME_READER_H

#include <Adafruit_BME280.h>

struct BmeData {
    float temp;
    float humi;
    float pres;
    bool valid;
};

class BmeReader {
public:
    bool begin();
    BmeData read();
private:
    Adafruit_BME280 bme_;
    bool initialized_ = false;
};

#endif
