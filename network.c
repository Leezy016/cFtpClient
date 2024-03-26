#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "network.h"


// 创建网络连接
NetWork* open_network(char c_or_s,int type,char* ip,uint16_t port)
{
	// 在堆上创建NetWork结构
	NetWork* nw = malloc(sizeof(NetWork));
	if(NULL == nw)
	{
		perror("network malloc");
		return NULL;
	}

	// 创建socket对象
	nw->fd = socket(AF_INET,type,0);
	if(0 > nw->fd)
	{
		perror("network socket");
		free(nw);
		return NULL;
	}

	// 准备通信地址
	nw->addr.sin_family = AF_INET;
	nw->addr.sin_port = htons(port);
	nw->addr.sin_addr.s_addr = inet_addr(ip);
	nw->len = sizeof(nw->addr);
	nw->type = type;

	//下面这个if段代码还没看
	if('s' == c_or_s)
	{
		if(bind(nw->fd,(SP)&nw->addr,nw->len))
		{
			perror("network bind");
			free(nw);
			return NULL;
		}

		if(SOCK_STREAM == type && listen(nw->fd,50))
		{
			perror("network listen");
			free(nw);
			return NULL;
		}
	}
	else if(SOCK_STREAM == type)
	{
		if(connect(nw->fd,(SP)&nw->addr,nw->len))
		{
			perror("network connect");
			free(nw);
			return NULL;
		}	
	}

	return nw;
}

// TCP的server专用
NetWork* accept_network(NetWork* nw)
{
	if(SOCK_STREAM != nw->type)
	{
		printf("network accept socket type error!\n");
		return NULL;
	}

	NetWork* clinw = malloc(sizeof(NetWork));
	if(NULL == clinw)
	{
		perror("network accept malloc");
		return NULL;
	}
	
	clinw->type = nw->type;
	clinw->len = sizeof(clinw->addr);
	clinw->fd = accept(nw->fd,(SP)&clinw->addr,&clinw->len);
	if(0 > clinw->fd)
	{
		perror("network accept");
		free(clinw);
		return NULL;
	}

	return clinw;
}

// 发送数据
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

// 接收数据
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

// 关闭网络连接
void close_network(NetWork* nw)
{
	if(close(nw->fd))
	{
		perror("network close");
	}
	free(nw);
}

//配置ftp会话参数
void ex(void)
{
	sprintf(buf,"OPTS UTF8 ON\n");
	nsend(nw,buf,strlen(buf));

	bzero(buf,sizeof(buf));
	nrecv(nw,buf,sizeof(buf));
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