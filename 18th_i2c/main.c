#include "stdio.h"
#include "serial.h"
#include "i2c.h"
#include "s3c24xx.h"



int main()
{
	char c;
	int data;
	char str[200];
	int i;
	int address;
	
	uart0_init();
	i2c_init();
	while(1)
	{
		printf("\r\n##### AT24CXX Menu #####\r\n"); 
		printf("[R] Read AT24CXX\r\n"); 
		printf("[W] Write AT24CXX\r\n");
		printf("Enter your selection: ");

		c = getc();
		putc(c);
		printf("\r\n");
		switch(c)
		{
			case 'r':
			case 'R':
			{
				  i = 0;
				  printf("Enter address: ");
				  do
				  {
				  	  c = getc();
					  str[i++] = c;
					  putc(c);
				  }while(c != '\n' && c != '\r');
				  printf("\r\n");
				  str[i] = '\0';
				  
				  while(--i >= 0)
				  {
					 if(str[i] < '0' || str[i] > '9')
					 	str[i] = ' ';
				  }
				  sscanf(str,"%d",&address);
				  printf("\r\nread address = %d\r\n", address);
				  data = at24cxx_read(address);
				  printf("data = %d\r\n",data);
					break;
				  
			}
			
			case 'w':
			case 'W':
			{
				printf("Enter address: ");
				do                
				{                    
						c = getc(); 
						str[i++] = c;
						putc(c);
				} while(c != '\n' && c != '\r');
				str[i] = '\0';
				printf("\r\n");
				while(--i >= 0)
				{
					if (str[i] < '0' || str[i] > '9')
					str[i] = ' ';
				}
				sscanf(str,"%d",&address);
				printf("Enter data: ");
				i = 0;
				do                
				{                    
						c = getc(); 
						str[i++] = c;
						putc(c);
				} while(c != '\n' && c != '\r');
				str[i] = '\0';
				printf("\r\n");
				while(--i >= 0)
				{
					if (str[i] < '0' || str[i] > '9')
					str[i] = ' ';
				}
				sscanf(str,"%d",&data);
				printf("write address %d with data %d\r\n", address, data);
				
				at24cxx_write(address, data);
					break;
			}
		
			default:break;
		}
	}
	return 0;
}

