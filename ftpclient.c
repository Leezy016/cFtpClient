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

//配置ftp会话参数
void ex(void);
//发送数据
int nsend(NetWork* nw,void* buf,uint32_t len);
//接收数据
int nrecv(NetWork* nw,void* buf,uint32_t len);
//关闭网络连接
void close_network(NetWork* nw);
//显示当前目录
void ls(void);
//改变目录
void cd_to(char* cd);
//下载
void download(char* get);


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

//配置ftp会话参数
void ex(void)
{
	sprintf(buf,"OPTS UTF8 ON\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
}

//发送数据
int nsend(NetWork* nw,void* buf,uint32_t len)
{
	if(SOCK_STREAM == nw->type)
	{
		return send(nw->fd,buf,len,0);
	}
	else if(SOCK_DGRAM == nw->type)
	{
		return sendto(nw->fd,buf,len,0,(SP)&nw->addr,nw->len);
	}
	return -1;
}

//接收数据
int nrecv(NetWork* nw,void* buf,uint32_t len)
{
	if(SOCK_STREAM == nw->type)
	{
		return recv(nw->fd,buf,len,0);
	}
	else if(SOCK_DGRAM == nw->type)
	{
		return recvfrom(nw->fd,buf,len,0,(SP)&nw->addr,&nw->len);
	}
	return -1;
}

//关闭网络连接
void close_network(NetWork* nw)
{
	if(close(nw->fd))
	{
		perror("network close");
	}
	free(nw);
}

//显示当前目录
void ls(void)
{
	sprintf(buf,"PWD\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);//257

	sprintf(buf,"PASV\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);//227

	printf("%s",buf);
	unsigned char ip1,ip2,ip3,ip4,port1,port2;


	//if(strchr(buf,'(')!=NULL){
		sscanf(strchr(buf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);
	//}

	sprintf(buf,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);

	NetWork* data_nw = open_network('c',SOCK_STREAM,buf,port1*256+port2);
	//printf("connect success fd = %d\n",data_nw->fd);
	
	sprintf(buf,"LIST -al\n");
	nsend(nw,buf,strlen(buf));

	printf("200 PORT command successful. Consider using PASV.\n");

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//150
	
	int ret = 0;
	bzero(buf,sizeof(buf));
	while(ret = nrecv(data_nw,buf,sizeof(buf)))
	{
		printf("%s",buf);
		bzero(buf,sizeof(buf));
	}
	close_network(data_nw);

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//226
}

//改变目录
void cd_to(char* cd)
{
	char *dir = cd;
	if(strcmp(dir,"..")==0)
	{
		sprintf(buf,"CDUP %s\n",dir);
	}
	else
	{
		sprintf(buf,"CWD %s\n",dir);
	}
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//250
}

//下载
void download(char* get)
{
	bzero(buf,sizeof(buf));
	char *filename = get;
	sprintf(buf,"TYPE A\n");//设置数据传输方式ASCLL
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	sprintf(buf,"SIZE %s\n",filename);
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	sprintf(buf,"MDTM %s\n",filename);//文件最后修改时间
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	sprintf(buf,"PASV\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);
	
	unsigned char ip1,ip2,ip3,ip4,port1,port2;
	sscanf(strchr(buf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);
	sprintf(buf,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);

	NetWork* data_nw = open_network('c',SOCK_STREAM,buf,port1*256+port2);
	printf("connect success fd = %d\n",data_nw->fd);

	sprintf(buf,"RETR %s\n",filename);//准备文件副本
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	int fd = open(filename,O_WRONLY|O_CREAT|O_TRUNC,0644);
	//尝试以只写方式打开filename指定的文件。如果文件不存在，则创建它。如果文件已经存在，则将其内容清空。
	if(0 > fd)
	{
		perror("open");
		return;
	}
	int ret = 0;
	bzero(buf,sizeof(buf));
	while(ret = nrecv(data_nw,buf,sizeof(buf)))
	{
		write(fd,buf,ret);
		bzero(buf,sizeof(buf));
	}
	close(fd);

	close_network(data_nw);

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//226
}