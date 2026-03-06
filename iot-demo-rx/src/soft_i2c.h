/*
 * soft_i2c.h - ソフトウェア I2C (GPIO ビットバング) ドライバ
 *
 * SCL = P12, SDA = P13  (GR-SAKURA RIIC0 標準ピン)
 * 速度: 約 100kHz (ソフトウェア遅延)
 */

#ifndef SOFT_I2C_H
#define SOFT_I2C_H

void    i2c_init(void);
int     i2c_write(unsigned char addr, const unsigned char *data, int len);
int     i2c_read(unsigned char addr, unsigned char *data, int len);
int     i2c_write_read(unsigned char addr,
                       const unsigned char *wdata, int wlen,
                       unsigned char *rdata, int rlen);

#endif /* SOFT_I2C_H */
