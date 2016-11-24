#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>


#define RW_R8	  0
#define RW_R16	  1
#define RW_R32	  2
#define RW_W8	  3
#define RW_W16	  4
#define RW_W32	  5

void print_usage(char *filename)
{
	printf("./%s <r8 | r16 | r32> <addr> [num]\n",filename);
	printf("./%s <w8 | w16 | w32> <addr> <val>\n",filename);
}

int main(int argc,char **argv)
{
	int fd,i;
	unsigned int buf[3];
	unsigned int num;

	if((argc != 3) && (argc != 4))
	{
		print_usage(argv[0]);
		return -1;
	}

	fd = open("/dev/registertools", O_RDWR);
	if(fd < 0)
	{
		printf("cannot open /dev/registertools\n");
		return -2;
	}
	buf[1] = strtoul(argv[3], NULL, 0);
	if(argc == 4)
	{
	buf[1] = strtoul(argv[3], NULL, 0);
		num  = buf[1];
	}
	else
		num = 1;
	//addr	
	buf[0] = strtoul(argv[2], NULL, 0);
	
	if(!strcmp(argv[1],"r8"))
	{
		for(i = 0; i < num; i++)
		{
			ioctl(fd, RW_R8, buf);
			printf("%d : addr:0x%x = 0x%x\n", i, buf[0], (unsigned char)buf[1]);
			buf[0] += 1;
		}
	}
	else if(!strcmp(argv[1],"r16"))
	{
		for(i = 0; i < num; i++)
		{
			ioctl(fd, RW_R16, buf);
			printf("%d : addr:0x%x = 0x%x\n", i, buf[0], (unsigned short)buf[1]);
			buf[0] += 2;
		}
	}
	else if(!strcmp(argv[1],"r32"))
	{
		for(i = 0; i < num; i++)
		{
			ioctl(fd, RW_R32, buf);
			printf("%d : addr:0x%x = 0x%x\n", i, buf[0], (unsigned int)buf[1]);
			buf[0] += 4;
		}
	}
	else if(!strcmp(argv[1],"w8"))
	{
		ioctl(fd, RW_W8, buf);
	}
	
	else if(!strcmp(argv[1],"w16"))
	{
		ioctl(fd, RW_W16, buf);
	}
	
	else if(!strcmp(argv[1],"w32"))
	{
		ioctl(fd, RW_W32, buf);
	}
	else
	{
		print_usage(argv[0]);
		return -3;
	}
	return 0;
}


