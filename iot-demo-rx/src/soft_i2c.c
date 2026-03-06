/*
 * soft_i2c.c - ソフトウェア I2C (GPIO ビットバング) ドライバ
 *
 * SCL = P12, SDA = P13
 * オープンドレイン動作: HIGH = 入力モード (プルアップ)
 *                       LOW  = 出力モード + LOW出力
 *
 * PCLKB = 50MHz → I2C_DELAY で約 100kHz を実現
 */

#include "iodefine.h"
#include "soft_i2c.h"

/* --- 遅延 (約 5us @ 50MHz → ~100kHz SCL) --- */
static void i2c_delay(void)
{
    volatile int i;
    for (i = 0; i < 25; i++) {
        __asm("nop");
    }
}

/* --- SCL/SDA 制御 --- */
/* HIGH: ピンを入力にして外部プルアップに任せる */
/* LOW:  ピンを出力にして LOW を出力 */

static void scl_high(void)
{
    PORT1.PDR.BIT.B2 = 0;   /* 入力 → Hi-Z → プルアップで HIGH */
}

static void scl_low(void)
{
    PORT1.PODR.BIT.B2 = 0;  /* LOW を出力 */
    PORT1.PDR.BIT.B2 = 1;   /* 出力モード */
}

static void sda_high(void)
{
    PORT1.PDR.BIT.B3 = 0;   /* 入力 → Hi-Z → プルアップで HIGH */
}

static void sda_low(void)
{
    PORT1.PODR.BIT.B3 = 0;  /* LOW を出力 */
    PORT1.PDR.BIT.B3 = 1;   /* 出力モード */
}

static int sda_read(void)
{
    PORT1.PDR.BIT.B3 = 0;   /* 入力モード */
    return PORT1.PIDR.BIT.B3;
}

/* --- I2C プリミティブ --- */

static void i2c_start(void)
{
    sda_high();
    scl_high();
    i2c_delay();
    sda_low();
    i2c_delay();
    scl_low();
    i2c_delay();
}

static void i2c_stop(void)
{
    sda_low();
    i2c_delay();
    scl_high();
    i2c_delay();
    sda_high();
    i2c_delay();
}

/* 1バイト送信。ACK=0 で成功、NACK=1 で失敗 */
static int i2c_send_byte(unsigned char data)
{
    int i, ack;

    for (i = 7; i >= 0; i--) {
        if (data & (1 << i))
            sda_high();
        else
            sda_low();
        i2c_delay();
        scl_high();
        i2c_delay();
        scl_low();
        i2c_delay();
    }

    /* ACK 読み取り */
    sda_high();  /* SDA 解放 */
    i2c_delay();
    scl_high();
    i2c_delay();
    ack = sda_read();
    scl_low();
    i2c_delay();

    return ack;  /* 0=ACK, 1=NACK */
}

/* 1バイト受信。ack=0 で ACK 送信、ack=1 で NACK 送信 */
static unsigned char i2c_recv_byte(int send_nack)
{
    unsigned char data = 0;
    int i;

    sda_high();  /* SDA 解放 (入力モード) */

    for (i = 7; i >= 0; i--) {
        i2c_delay();
        scl_high();
        i2c_delay();
        if (sda_read())
            data |= (1 << i);
        scl_low();
        i2c_delay();
    }

    /* ACK/NACK 送信 */
    if (send_nack)
        sda_high();
    else
        sda_low();
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    i2c_delay();
    sda_high();

    return data;
}

/* --- 公開 API --- */

void i2c_init(void)
{
    /* 初期状態: SDA, SCL ともに HIGH (入力モード) */
    PORT1.PODR.BIT.B2 = 0;
    PORT1.PODR.BIT.B3 = 0;
    scl_high();
    sda_high();
    i2c_delay();
}

int i2c_write(unsigned char addr, const unsigned char *data, int len)
{
    int i;

    i2c_start();

    if (i2c_send_byte((addr << 1) | 0)) {  /* Write */
        i2c_stop();
        return -1;
    }

    for (i = 0; i < len; i++) {
        if (i2c_send_byte(data[i])) {
            i2c_stop();
            return -1;
        }
    }

    i2c_stop();
    return 0;
}

int i2c_read(unsigned char addr, unsigned char *data, int len)
{
    int i;

    i2c_start();

    if (i2c_send_byte((addr << 1) | 1)) {  /* Read */
        i2c_stop();
        return -1;
    }

    for (i = 0; i < len; i++) {
        data[i] = i2c_recv_byte(i == len - 1);  /* 最後は NACK */
    }

    i2c_stop();
    return 0;
}

int i2c_write_read(unsigned char addr,
                   const unsigned char *wdata, int wlen,
                   unsigned char *rdata, int rlen)
{
    int i;

    /* Write フェーズ (レジスタアドレス送信) */
    i2c_start();

    if (i2c_send_byte((addr << 1) | 0)) {
        i2c_stop();
        return -1;
    }

    for (i = 0; i < wlen; i++) {
        if (i2c_send_byte(wdata[i])) {
            i2c_stop();
            return -1;
        }
    }

    /* Repeated Start → Read フェーズ */
    i2c_start();

    if (i2c_send_byte((addr << 1) | 1)) {
        i2c_stop();
        return -1;
    }

    for (i = 0; i < rlen; i++) {
        rdata[i] = i2c_recv_byte(i == rlen - 1);
    }

    i2c_stop();
    return 0;
}
