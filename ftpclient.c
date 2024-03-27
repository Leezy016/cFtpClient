#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>//Linux终端输入输出
#include <arpa/inet.h>//ip地址转换操作
#include <sys/types.h>//Linux基本系统数据类型
#include <sys/stat.h>//Linux系统文件状态查询
#include <fcntl.h>//文件控制
#include <stdlib.h>
#include <sys/socket.h>//socket创建与操作
#include "network.h"

NetWork* nw;//命令通道
NetWork* data_nw=NULL;//数据通道
unsigned char ip1,ip2,ip3,ip4,port1,port2;
int list_len = 0;
char buf[256] = {};
typedef struct List
{
	char filename[40];
}List;

//清空buf,接收数据并输出
void bufr(void);
//服务器端设置UTF-8模式
void serUTF8(void);

//显示帮助
void help(void);
//显示工作目录名称
void pwd(void);
//显示工作目录下的内容
void ls(void);
//改变目录
void cd(char* dirName);
//下载文件
void get(char* fileName);


int main(int argc,char* argv[]){
	printf("Welcome, type \'help\' for help message.\n");
	printf("\n");
	//用c_ip存储用户输入的ftp服务器ip地址
	char c_ip[40] = {};
	strcpy(c_ip,argv[1]);

	//建立连接
	//
	nw = open_network('c',SOCK_STREAM,c_ip,21);
	if(NULL == nw)
	{
		printf("open network socket null!\n");
		return -1;
	}

	printf("Connected to %s\n",c_ip);
	bufr();

	//身份验证
	//
	for(;;)
	{
		//用user存储输入的用户名
		char user[20] = {};
		printf("Name (%s):",c_ip);
		gets(user);

		//将用户名发送给服务器
		sprintf(buf,"USER %s\n",user);//写缓冲区
		nsend(nw,buf,strlen(buf));
		bufr();

		//用户名错误则重新输入
		if(strstr(buf,"530"))
		{
			continue;
		}

		//用pw存储用户输入的密码
		char pw[20] = {};
		printf("Password:");

		//设置终端属性，隐藏密码输入
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
		printf("\n");
		bufr();

		//登陆成功跳出循环
		if(strstr(buf,"530") == NULL)
		{
			break;
		}
	}

	//数据传输
	//
	serUTF8();	
	char cmd[40] = {};
	while(1)
	{
		//判断命令输入正误
		int flag=1;
		printf("ftp> ");
		gets(cmd);
		//获取单命令
		if(strcmp(cmd,"bye")==0)//结束程序
			break;
		if(strcmp(cmd,"ls")==0){
			ls();
			flag=0;
		}
		if(strcmp(cmd,"pwd")==0){
			pwd();
			flag=0;
		}
		if(strcmp(cmd,"help")==0){
			help();
			flag=0;
		}
		//获取命令+1参数
		char *cmd1 = malloc(20);
		char *path = malloc(100);
		sscanf(cmd,"%s %s",cmd1,path);
		if(strcmp(cmd1,"cd") == 0){
			cd(path);
			flag=0;
		}
		if(strcmp(cmd1,"get") == 0){	
			get(path);
			flag=0;
		}
		//命令输入错误，显示提示信息
		if(flag){
			printf("unrecognized command\n");
			help();
		}
	}
	//结束连接
	//
	close_network(nw);
	printf("221 Goodbye.\n");
}

void bufr(void)
{
	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	printf("%s",buf);
}

void help(void)
{
	printf("--------------------------------\n");
	// printf("This is an FTP client program designed for a university computer network course.\n");
	// printf("If you have any problem during use, please:\n");
	// printf("A. read this massage again\n");
	// printf("B. contact me at leezy016@foxmail.com\n");
	// printf("C. use a real FTP client\n");
	// printf("THANK YOU\n");
	// printf("\n");
	printf("Commands and usage:\n");
	printf("ls:   Print subdirectories and files in the working directory\n");
	printf("pwd:  Print the name of the working directory\n");
	printf("cd:   Change directory\n");
	printf("        To a specified directory: cd /dirName\n");
	printf("        To the parent directory: cd ..\n");
	printf("        To the current directory: cd .\n");
	printf("get:  Download file, use as: get fileName\n");
	printf("help: Show this help message\n");
	printf("bye:  Exit client\n");
	printf("--------------------------------\n");
}

void serUTF8(void)
{
	sprintf(buf,"OPTS UTF8 ON\n");
	nsend(nw,buf,strlen(buf));
	bufr();
}

void cd(char* cd)
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
	bufr();
}

void pwd(void)
{
	sprintf(buf,"PWD\n");
	nsend(nw,buf,strlen(buf));
	bufr();
}


void ls(void)
{
	//接收服务器ip地址和数据连接端口号
	//被动模式，使用命令通道进行进行连接
	sprintf(buf,"PASV\n");
	nsend(nw,buf,strlen(buf));
	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	//227
	//puts(buf);

	unsigned char ip1,ip2,ip3,ip4,port1,port2;
	//分割ip和端口号
	sscanf(strchr(buf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);
	//ip转为点分十进制存储
	sprintf(buf,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);
	//创建数据连接，端口由2个8b到1个16b
	NetWork* data_nw = open_network('c',SOCK_STREAM,buf,port1*256+port2);
	
	//发送ls -al给服务器
	sprintf(buf,"LIST -al\n");
	nsend(nw,buf,strlen(buf));
	//printf("200 PORT command successful. Consider using PASV.\n");
	bufr();//150
	
	//循环接收当前目录的内容，并输出
	int ret = 0;
	bzero(buf,sizeof(buf));
	while(ret = nrecv(data_nw,buf,sizeof(buf)))
	{
		printf("%s",buf);
		bzero(buf,sizeof(buf));
	}
	bufr();//226
	
	//关闭数据连接
	close_network(data_nw);	
}

void get(char* fileName)
{
    //设置数据传输方式ASCLL
	sprintf(buf,"TYPE A\n");
	nsend(nw,buf,strlen(buf));
	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	//puts(buf);

	//获取文件大小
	sprintf(buf,"SIZE %s\n",fileName);
	nsend(nw,buf,strlen(buf));
	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	//puts(buf);

	if (strstr(buf, "550")!=NULL) {
        printf("File %s does not exist in the current directory on the server.\n", fileName);
        return;
    }

	//获取文件最后修改时间
	// sprintf(buf,"MDTM %s\n",filename);//文件最后修改时间
	// nsend(nw,buf,strlen(buf));
	// bzero(buf,sizeof(buf));
	// nrecv(nw,buf,sizeof(buf));
	//puts(buf);

	//接收服务器ip地址和数据连接端口号，被动模式
	sprintf(buf,"PASV\n");
	nsend(nw,buf,strlen(buf));
	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
	//puts(buf);
	//ip和端口号转换
	unsigned char ip1,ip2,ip3,ip4,port1,port2;
	sscanf(strchr(buf,'(')+1,"%hhu,%hhu,%hhu,%hhu,%hhu,%hhu",&ip1,&ip2,&ip3,&ip4,&port1,&port2);
	sprintf(buf,"%hhu.%hhu.%hhu.%hhu",ip1,ip2,ip3,ip4);

	//创建数据连接
	NetWork* data_nw = open_network('c',SOCK_STREAM,buf,port1*256+port2);
	//printf("connect success fd = %d\n",data_nw->fd);

	sprintf(buf,"RETR %s\n",fileName);//准备文件副本
	nsend(nw,buf,strlen(buf));
	bufr();

	int fd = open(fileName,O_WRONLY|O_CREAT|O_TRUNC,0644);
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
	bufr();//226
}
