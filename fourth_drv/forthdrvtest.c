#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

int main(int argc, char **argv)
{
	int fd = 4;//这里对fd赋值对open没有影响
	unsigned char key_val;
	int ret;
	
	struct pollfd fds[1] = {fd,POLLIN};//这里这个4是任意写的，因为除了3其他的都是好使的,
	fd = open("/dev/buttons", O_RDWR);
	if (fd < 0)
	{
		printf("can't open!\n");
	}
	
	//fds[0].fd = fd;
	//fds[0].events = POLLIN;
	
	while(1)
	{
		ret = poll(fds.fd,1,2000);
		if(ret == 0)
		{
			printf("tume out\n");	
		}
		else
		{
		read(fd, &key_val, 1);
		printf("key_val = 0x%x\n", key_val);	
		}
	}
	return 0;
}