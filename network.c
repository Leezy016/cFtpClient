#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>//系统调用
#include <arpa/inet.h>//ip地址转换操作
#include <sys/types.h>//Linux基本系统数据类型
#include <sys/socket.h>//socket创建与操作
#include "network.h"


// 创建网络连接
NetWork* open_network(char c_or_s,int type,char* ip,uint16_t port)
{
	// 创建NetWork结构的对象nw
	NetWork* nw = malloc(sizeof(NetWork));
	if(NULL == nw)
	{
		perror("network malloc");
		return NULL;
	}

	//创建TCP socket
	nw->fd = socket(AF_INET,type,0);
	if(0 > nw->fd)
	{
		perror("network socket");
		free(nw);
		return NULL;
	}

	// 准备通信地址
	//
	nw->addr.sin_family = AF_INET;
	//端口号由主机字节顺序（intel x86，小端）转为TCP/IP网络字节顺序（大端）
	nw->addr.sin_port = htons(port);
	//地址由点分十进制到unsigned long int
	nw->addr.sin_addr.s_addr = inet_addr(ip);
	nw->len = sizeof(nw->addr);
	nw->type = type;
	if(connect(nw->fd,(SP)&nw->addr,nw->len))
	{
		perror("network connect");
		free(nw);
		return NULL;
	}
	return nw;
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

//发送数据
int nsend(NetWork* nw,void* buf,uint32_t len)
{
	return send(nw->fd,buf,len,0);
}

//接收数据
int nrecv(NetWork* nw,void* buf,uint32_t len)
{
	return recv(nw->fd,buf,len,0);
}