/*
 * bme280.c - BME280 センサードライバ
 *
 * Bosch BME280 の補正アルゴリズムをデータシートに基づいて実装
 * 32bit 整数演算のみ使用 (FPU 不要)
 */

#include "soft_i2c.h"
#include "bme280.h"

/* レジスタアドレス */
#define BME280_REG_ID           0xD0
#define BME280_REG_RESET        0xE0
#define BME280_REG_CTRL_HUM     0xF2
#define BME280_REG_STATUS       0xF3
#define BME280_REG_CTRL_MEAS    0xF4
#define BME280_REG_CONFIG       0xF5
#define BME280_REG_PRESS_MSB    0xF7
#define BME280_REG_CALIB00      0x88
#define BME280_REG_CALIB26      0xE1

#define BME280_CHIP_ID          0x60

/* I2C アドレス (bme280_init で設定) */
static unsigned char g_addr;

/* 補正パラメータ */
static unsigned short dig_T1;
static short          dig_T2, dig_T3;
static unsigned short dig_P1;
static short          dig_P2, dig_P3, dig_P4, dig_P5;
static short          dig_P6, dig_P7, dig_P8, dig_P9;
static unsigned char  dig_H1, dig_H3;
static short          dig_H2, dig_H4, dig_H5;
static signed char    dig_H6;

/* t_fine: 温度補正で使用するグローバル変数 */
static long t_fine;

/* --- レジスタ読み書き --- */

static int reg_read(unsigned char reg, unsigned char *buf, int len)
{
    return i2c_write_read(g_addr, &reg, 1, buf, len);
}

static int reg_write(unsigned char reg, unsigned char val)
{
    unsigned char buf[2];
    buf[0] = reg;
    buf[1] = val;
    return i2c_write(g_addr, buf, 2);
}

/* --- 補正パラメータ読み出し --- */

static int read_calibration(void)
{
    unsigned char buf[26];
    unsigned char buf2[7];

    /* calib00-25 (0x88-0xA1) */
    if (reg_read(BME280_REG_CALIB00, buf, 26) != 0)
        return -1;

    dig_T1 = (unsigned short)(buf[1] << 8 | buf[0]);
    dig_T2 = (short)(buf[3] << 8 | buf[2]);
    dig_T3 = (short)(buf[5] << 8 | buf[4]);

    dig_P1 = (unsigned short)(buf[7] << 8 | buf[6]);
    dig_P2 = (short)(buf[9] << 8 | buf[8]);
    dig_P3 = (short)(buf[11] << 8 | buf[10]);
    dig_P4 = (short)(buf[13] << 8 | buf[12]);
    dig_P5 = (short)(buf[15] << 8 | buf[14]);
    dig_P6 = (short)(buf[17] << 8 | buf[16]);
    dig_P7 = (short)(buf[19] << 8 | buf[18]);
    dig_P8 = (short)(buf[21] << 8 | buf[20]);
    dig_P9 = (short)(buf[23] << 8 | buf[22]);

    dig_H1 = buf[25];

    /* calib26-32 (0xE1-0xE7) */
    if (reg_read(BME280_REG_CALIB26, buf2, 7) != 0)
        return -1;

    dig_H2 = (short)(buf2[1] << 8 | buf2[0]);
    dig_H3 = buf2[2];
    dig_H4 = (short)((buf2[3] << 4) | (buf2[4] & 0x0F));
    dig_H5 = (short)((buf2[5] << 4) | (buf2[4] >> 4));
    dig_H6 = (signed char)buf2[6];

    return 0;
}

/* --- 補正演算 (Bosch データシートより) --- */

static long compensate_temp(long adc_T)
{
    long var1, var2, T;

    var1 = ((((adc_T >> 3) - ((long)dig_T1 << 1))) * ((long)dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((long)dig_T1)) *
              ((adc_T >> 4) - ((long)dig_T1))) >> 12) *
            ((long)dig_T3)) >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;  /* 温度 × 100 (例: 2530 = 25.30℃) */
}

static unsigned long compensate_press(long adc_P)
{
    long var1, var2;
    unsigned long p;

    var1 = (t_fine >> 1) - 64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((long)dig_P6);
    var2 = var2 + ((var1 * ((long)dig_P5)) << 1);
    var2 = (var2 >> 2) + (((long)dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
            ((((long)dig_P2) * var1) >> 1)) >> 18;
    var1 = ((32768 + var1) * ((long)dig_P1)) >> 15;

    if (var1 == 0)
        return 0;

    p = (unsigned long)(((long)1048576 - adc_P) - (var2 >> 12)) * 3125;
    if (p < 0x80000000UL)
        p = (p << 1) / ((unsigned long)var1);
    else
        p = (p / (unsigned long)var1) * 2;

    var1 = (((long)dig_P9) * ((long)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
    var2 = (((long)(p >> 2)) * ((long)dig_P8)) >> 13;

    p = (unsigned long)((long)p + ((var1 + var2 + (long)dig_P7) >> 4));
    return p;  /* Pa 単位 */
}

static unsigned long compensate_hum(long adc_H)
{
    long v_x1_u32r;

    v_x1_u32r = t_fine - 76800;
    v_x1_u32r = (((((adc_H << 14) - (((long)dig_H4) << 20) -
                    (((long)dig_H5) * v_x1_u32r)) + 16384) >> 15) *
                 (((((((v_x1_u32r * ((long)dig_H6)) >> 10) *
                      (((v_x1_u32r * ((long)dig_H3)) >> 11) + 32768)) >> 10) +
                    2097152) * ((long)dig_H2) + 8192) >> 14));
    v_x1_u32r = v_x1_u32r - (((((v_x1_u32r >> 15) *
                                 (v_x1_u32r >> 15)) >> 7) *
                                ((long)dig_H1)) >> 4);
    if (v_x1_u32r < 0)
        v_x1_u32r = 0;
    if (v_x1_u32r > 419430400)
        v_x1_u32r = 419430400;

    return (unsigned long)(v_x1_u32r >> 12);  /* Q22.10 固定小数点 */
}

/* --- 公開 API --- */

int bme280_init(unsigned char addr)
{
    unsigned char id;

    g_addr = addr;

    i2c_init();

    /* チップ ID 確認 */
    if (reg_read(BME280_REG_ID, &id, 1) != 0)
        return -1;
    if (id != BME280_CHIP_ID)
        return -2;

    /* ソフトリセット */
    reg_write(BME280_REG_RESET, 0xB6);
    {
        volatile int i;
        for (i = 0; i < 50000; i++) __asm("nop");
    }

    /* 補正パラメータ読み出し */
    if (read_calibration() != 0)
        return -3;

    /* 設定: オーバーサンプリング ×1, ノーマルモード */
    reg_write(BME280_REG_CTRL_HUM, 0x01);   /* 湿度 ×1 */
    reg_write(BME280_REG_CONFIG, 0xA0);      /* スタンバイ 1000ms, フィルタ OFF */
    reg_write(BME280_REG_CTRL_MEAS, 0x27);   /* 温度×1, 気圧×1, ノーマルモード */

    return 0;
}

int bme280_read(bme280_data_t *data)
{
    unsigned char buf[8];
    long adc_T, adc_P, adc_H;

    /* 8 バイト一括読み出し: press[3] + temp[3] + hum[2] */
    if (reg_read(BME280_REG_PRESS_MSB, buf, 8) != 0)
        return -1;

    adc_P = ((long)buf[0] << 12) | ((long)buf[1] << 4) | (buf[2] >> 4);
    adc_T = ((long)buf[3] << 12) | ((long)buf[4] << 4) | (buf[5] >> 4);
    adc_H = ((long)buf[6] << 8) | buf[7];

    /* 補正演算 (温度を先に計算: t_fine が必要) */
    data->temp_x100 = compensate_temp(adc_T);

    {
        unsigned long p = compensate_press(adc_P);
        /* Pa → hPa × 100: p(Pa) / 100 * 100 = p そのまま...
         * compensate_press は Pa を返す。hPa にするには /100。
         * hPa × 100 にするには Pa のまま。 */
        data->press_x100 = (long)p;
    }

    {
        unsigned long h = compensate_hum(adc_H);
        /* Q22.10 → % × 100: h / 1024 * 100 = h * 100 / 1024 */
        data->hum_x100 = (long)((h * 100) / 1024);
    }

    return 0;
}
