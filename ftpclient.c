#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/socket.h>
#include "network.h"

NetWork* nw;
NetWork* data_nw;
int list_len = 0;
char buf[256] = {};
//unsigned char ip1,ip2,ip3,ip4,port1,port2;

typedef struct List
{
	char filename[40];
}List;

int main(int argc,char* argv[])
{	

	char c_ip[40] = {};
	strcpy(c_ip,argv[1]);

	nw = open_network('c',SOCK_STREAM,c_ip,21);
	if(NULL == nw)
	{
		printf("open network socket null!\n");
		return -1;
	}

	printf("Connected to %s.\n",c_ip);

	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);

	for(;;)
	{
		char user[20] = {};
		printf("Name (%s):",c_ip);
		gets(user);

		sprintf(buf,"USER %s\n",user);//写缓冲区
		nsend(nw,buf,strlen(buf));

		bzero(buf,sizeof(buf));//缓冲区清零
		nrecv(nw,buf,sizeof(buf));
		printf("%s",buf);

		char pw[20] = {};
		printf("Password:");
		struct termios old, new; 
		tcgetattr(0, &old);  // 获取终端属性
		new = old;	
		new.c_lflag &= ~(ECHO | ICANON);// 不使用标准的输出，不显示字符。 
 		tcsetattr(0, TCSANOW, &new);// 设置终端新的属性
		gets(pw);								
		tcsetattr(0, TCSANOW, &old);

		sprintf(buf,"PASS %s\n",pw);
		nsend(nw,buf,strlen(buf));

		bzero(buf,sizeof(buf));
		nrecv(nw,buf,sizeof(buf));
		printf("\n%s",buf);
		if(strstr(buf,"530") == NULL)
		{
			break;
		}
	}

	ex();
	
	char cmd[40] = {};
	while(1)
	{

		printf("ftp> ");
		gets(cmd);
		if(strcmp(cmd,"bye")==0)
		{
			break;
		}
		if(strcmp(cmd,"ls")==0)
		{
			ls();
		}

		char *cmd1 = malloc(20);
		char *path = malloc(100);
		sscanf(cmd,"%s %s",cmd1,path);
		if(strcmp(cmd1,"cd") == 0)
		{
			cd_to(path);
		}

		if(strcmp(cmd1,"get") == 0)
		{
			download(path);
		}
		
	}

	printf("221 Goodbye.\n");
}