#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#define INPUT_REPLAY   0
#define INPUT_TAG	   1

void print_usage(char *file)
{
	printf("%s write <file>\n",file);
	printf("%s tag <string>\n",file);
	printf("%s replay\n",file);
}

int main(int argc, char **argv)
{
	int fd;
	int fd_data;
	int len,buf[100];

	if(argc != 2 && argc != 3)
	{
		printf("if(argc != 2 || argc != 3)\n");
		print_usage(argv[0]);
		return -1;
	}
	fd = open("/dev/input_replay", O_RDWR);
	if(fd < 0)
	{
		printf("can not open /dev/input_replay\n");
		return -1;
	}
	if(strcmp(argv[1],"replay") == 0)
		ioctl(fd, INPUT_REPLAY);
	else if(strcmp(argv[1],"write") == 0)
	{
		if(argc != 3)
		{
			print_usage(argv[0]);
			return -1;
		}

		fd_data = open(argv[2], O_RDONLY);
		if(fd_data < 0)
		{
			printf("can not open %s", argv[2]);
			return -1;
		}

		while(1)
		{
			/* 这里面不需要写驱动程序的read因为我们只是用户空间的读，一次读100字节，直到读完为止 */
			len = read(fd_data, buf, 100);
			if(len == 0)
			{
				printf("write ok\n");
				break;
			}
			else
			{
				write(fd, buf, len);
			}
			
		}
	}
	else if(strcmp(argv[1], "tag") == 0)
	{
		if (argc != 3)
		{
			print_usage(argv[0]);
			return -1;
		}
		ioctl(fd, INPUT_TAG, argv[2]);
	}
	else
	{
		print_usage(argv[0]);
			return -1;
	}
	
	
	return 0;
}

