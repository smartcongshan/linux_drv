
#ifndef __I2C_H
#define __I2C_H

void i2c_init(void);
void i2c_write(unsigned int slvaddr, unsigned char *buf, int len);
void i2c_read(unsigned int slvaddr, unsigned char *buf, int len);
void i2cinthandle(void);

#endif
 