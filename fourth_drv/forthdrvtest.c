#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

int main(int argc, char **argv)
{
	int fd = 4;//�����fd��ֵ��openû��Ӱ��
	unsigned char key_val;
	int ret;
	
	struct pollfd fds[1] = {fd,POLLIN};//�������4������д�ģ���Ϊ����3�����Ķ��Ǻ�ʹ��,
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