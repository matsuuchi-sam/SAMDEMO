/*
 * bme280.h - BME280 センサードライバ
 *
 * I2C アドレス: 0x76 (SDO=GND) または 0x77 (SDO=VDD)
 * 温度・湿度・気圧を読み取り
 */

#ifndef BME280_H
#define BME280_H

/* センサーデータ (整数×100 で表現) */
typedef struct {
    long temp_x100;       /* 温度 × 100  (例: 2530 = 25.30℃) */
    long hum_x100;        /* 湿度 × 100  (例: 6010 = 60.10%)  */
    long press_x100;      /* 気圧 × 100  (例: 101320 = 1013.20 hPa) */
} bme280_data_t;

int  bme280_init(unsigned char addr);
int  bme280_read(bme280_data_t *data);

#endif /* BME280_H */
