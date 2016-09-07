

#include <stdio.h>
#include "s3c24xx.h"
#include "i2c.h"

#define WRDATA      (1)
#define RDDATA      (2)

void Delay(int time);

struct tI2C{
	unsigned char *pDATA;		//数据缓冲区
	volatile int   Datacount;	//数据长度
	volatile int   Status;		//状态
	volatile int   Mode;		//模式
	volatile int   pt;			//发送数据中的位置
};
typedef struct tI2C s3c24xx_i2c;
static s3c24xx_i2c g_s3c24xx_i2c;

void i2c_init(void)
{
	GPEUP  |= 0xc000;	//禁止内部上拉
	GPECON |= 0xa0000000; //IICSDA IICSCL

	INTMSK = ~(BIT_IIC);

	/*
	 * Acknowledge generation    [7]   : 1使能应答
	 * Tx clock source selection [6]   : 0: IICCLK = fPCLK /16
	 * Tx/Rx Interrupt           [5]   : 1: Enable
	 * Transmit clock value      [3:0] : 0xf
	 */
	IICCON = (1<<7)|(0<<6)|(1<<5)|(0xf);

	IICSTAT = 0x10;
}

void i2c_write(unsigned int slvaddr, unsigned char *buf, int len)
{
	g_s3c24xx_i2c.Mode = WRDATA;
	g_s3c24xx_i2c.Datacount = len;
	g_s3c24xx_i2c.pDATA = buf;
	g_s3c24xx_i2c.pt = 0;
	
	IICDS = slvaddr;
	IICSTAT = 0xf0;

	while(g_s3c24xx_i2c.Datacount != -1);
}

void i2c_read(unsigned int slvaddr, unsigned char *buf, int len)
{
	g_s3c24xx_i2c.Mode = RDDATA;
	g_s3c24xx_i2c.Datacount = len;
	g_s3c24xx_i2c.pDATA = buf;
	g_s3c24xx_i2c.pt = -1;

	IICDS = slvaddr;
	IICSTAT = 0xb0;

	while(g_s3c24xx_i2c.Datacount != 0);
}

void I2CIntHandle(void)
{
	unsigned int iicst;
	unsigned int i;
	SRCPND = BIT_IIC;
	INTPND = BIT_IIC;

	iicst = IICSTAT;

	if(iicst & 0x8)
	{printf("Bus arbitration failed\n");}

	switch(g_s3c24xx_i2c.Mode)
	{
		case WRDATA:
		{
			if((g_s3c24xx_i2c.Datacount--) == 0)
			{
				IICSTAT = 0xd0;
				IICCON  = 0xaf;
				Delay(10000);
				break;
			}
			IICDS = g_s3c24xx_i2c.pDATA[g_s3c24xx_i2c.pt++];
			for(i = 0;i < 10;i++);

			IICCON = 0xaf;
			break;
		}
			
		case RDDATA:
		{
			if(g_s3c24xx_i2c.pt == -1)//刚开始接收数据
			{
				g_s3c24xx_i2c.pt = 0;
				if(g_s3c24xx_i2c.pt == 1)
					IICCON = 0x2f; //无应答
				else
					IICCON = 0xaf;	
				break;
			}
				g_s3c24xx_i2c.pDATA[g_s3c24xx_i2c.pt++] = IICDS;
				g_s3c24xx_i2c.Datacount--;

			if(g_s3c24xx_i2c.Datacount == 0)
			{
				IICSTAT = 0x90;                
				IICCON  = 0xaf;
				Delay(10000);
				break;
			}
			else
			{
					if(g_s3c24xx_i2c.pt == 1)
					IICCON = 0x2f; //无应答
					else
					IICCON = 0xaf;	
			}
			
			break;
		}
		default:
			break;
	}
	
}
void Delay(int time)
{    
	for (; time > 0; time--);
}
 
