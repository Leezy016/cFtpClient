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
typedef struct List
{
	char filename[40];
}List;

//配置ftp会话参数
void ex(void);
//显示当前工作目录名称
void pwd(void);
//显示当前工作目录下的内容
void ls(void);
//改变目录
void cd_to(char* cd);
//下载
void download(char* get);


int main(int argc,char* argv[])
{	
	//用c_ip存储用户输入的ftp服务器ip地址
	char c_ip[40] = {};
	strcpy(c_ip,argv[1]);

	//创建控制连接
	nw = open_network('c',SOCK_STREAM,c_ip,21);
	if(NULL == nw)
	{
		printf("open network socket null!\n");
		return -1;
	}

	printf("Connected to %s.\n",c_ip);

	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);

	//用户登陆
	for(;;)
	{
		//用user存储用户输入的用户名
		char user[20] = {};
		printf("Name (%s):",c_ip);
		gets(user);

		//将用户名发送给服务器
		sprintf(buf,"USER %s\n",user);//写缓冲区
		nsend(nw,buf,strlen(buf));

		//接受服务器消息
		bzero(buf,sizeof(buf));//缓冲区清零
		nrecv(nw,buf,sizeof(buf));
		printf("%s",buf);

		//用户名错误则重新输入
		if(strstr(buf,"530"))
		{
			continue;
		}

		//用pw存储用户输入的密码
		char pw[20] = {};
		printf("Password:");

		//设置终端属性，用户输入密码时不使用标准的输出，不显示字符
		struct termios old, new; 
		tcgetattr(0, &old);  //获取终端属性
		new = old;	
		new.c_lflag &= ~(ECHO | ICANON);//不使用标准的输出，不显示字符 
 		tcsetattr(0, TCSANOW, &new);//设置终端新的属性
		gets(pw);								
		tcsetattr(0, TCSANOW, &old);//将终端恢复原来属性

		//将密码发送给服务器
		sprintf(buf,"PASS %s\n",pw);
		nsend(nw,buf,strlen(buf));

		//接受服务器消息
		bzero(buf,sizeof(buf));
		nrecv(nw,buf,sizeof(buf));
		printf("\n%s",buf);

		//登陆成功跳出循环
		if(strstr(buf,"530") == NULL)
		{
			break;
		}
	}

	//配置ftp会话参数(UTF8编码)
	ex();	

	char cmd[40] = {};
	while(1)
	{
		//获取单个命令
		printf("ftp> ");
		gets(cmd);
		if(strcmp(cmd,"bye")==0)
			break;
		if(strcmp(cmd,"ls")==0)
			ls();
		if(strcmp(cmd,"pwd")==0)
			pwd();

		//获取命令+参数
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

//改变目录
void cd_to(char* cd)
{
	char *dir = cd;

	//如果用户输入“cd ..”, 将当前目录切换为上级目录
	if(strcmp(dir,"..")==0)
	{
		sprintf(buf,"CDUP %s\n",dir);
	}

	//否则，将当前目录切换为用户指定目录
	else
	{
		sprintf(buf,"CWD %s\n",dir);
	}
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//250
}

//显示当前工作目录名称
void pwd(void)
{
	sprintf(buf,"PWD\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);//257
}


//显示当前目录
void ls(void)
{
	//接收服务器ip地址和数据连接端口号
	sprintf(buf,"PASV\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	//puts(buf);//227

	printf("%s",buf);
	unsigned char ip1,ip2,ip3,ip4,port1,port2;


	sscanf(strchr(buf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);

	sprintf(buf,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);

	//创建数据连接
	NetWork* data_nw = open_network('c',SOCK_STREAM,buf,port1*256+port2);
	
	sprintf(buf,"LIST -al\n");
	nsend(nw,buf,strlen(buf));

	printf("200 PORT command successful. Consider using PASV.\n");

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//150
	
	//循环从数据连接接收当前目录的内容，并输出
	int ret = 0;
	bzero(buf,sizeof(buf));
	while(ret = nrecv(data_nw,buf,sizeof(buf)))
	{
		printf("%s",buf);
		bzero(buf,sizeof(buf));
	}

	//关闭数据连接
	close_network(data_nw);

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//226
}

//下载
void download(char* get)
{
	// 发送 LIST 命令获取文件列表
    sprintf(buf,"LIST\n");
    nsend(nw,buf,strlen(buf));
    bzero(buf,sizeof(buf));
    nrecv(nw,buf,sizeof(buf));

    // 检查文件是否存在于文件列表中
    if (strstr(buf, get) == NULL) {
        printf("File %s does not exist in the current directory on the server.\n", get);
        return;
    }

	//设置数据传输方式ASCLL
	char *filename = get;
	sprintf(buf,"TYPE A\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	//获取文件大小
	sprintf(buf,"SIZE %s\n",filename);
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	//获取文件最后修改时间
	sprintf(buf,"MDTM %s\n",filename);//文件最后修改时间
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);

	//接收服务器ip地址和数据连接端口号
	sprintf(buf,"PASV\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	puts(buf);
	
	unsigned char ip1,ip2,ip3,ip4,port1,port2;
	sscanf(strchr(buf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);
	sprintf(buf,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);

	//创建数据连接
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

	//循环从数据连接接收文件内容并写入本地文件
	int ret = 0;
	bzero(buf,sizeof(buf));
	while(ret = nrecv(data_nw,buf,sizeof(buf)))
	{
		write(fd,buf,ret);
		bzero(buf,sizeof(buf));
	}
	close(fd);

	//关闭数据连接
	close_network(data_nw);

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);//226
}