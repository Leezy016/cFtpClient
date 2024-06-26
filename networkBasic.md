# IP/TCP
TCP/IP协议组包括四层体系结构：网络接口层、网络层、传输层和应用层  
传输层主要包括TCP协议和UDP协议  
应用层则包括 **FTP**、HTTP、TELNET、SMTP、DNS等协议  

# TCP
TCP在OSI模型中位于第四层——传输层  
### TCP的主要特点：
**可靠性**：TCP通过序号、确认应答（ACK）、重传机制和校验和等手段，保证了数据的可靠传输。  
**面向连接**：TCP在数据传输前，需要先建立连接，传输结束后需要断开连接，这两个过程通常被称为“三次握手”和“四次挥手”。  
**基于字节流**：TCP看待应用层传下来的数据为一连串无结构的字节流。  
### TCP和FTP的关系
**建立连接**：FTP使用TCP来建立客户端和服务器之间的连接。  
**数据传输**：FTP使用TCP来保证文件数据的可靠传输。无论是文件的上传还是下载，都是通过TCP连接进行的。  
**命令控制**：FTP使用TCP连接来发送和接收命令。客户端通过这个连接向服务器发送命令，服务器通过这个连接向客户端发送响应。  


# Socket
Socket是网络编程的一个抽象概念，通常用于描述打开的网络连接。在创建Socket连接时，可以指定使用的传输层协议，Socket可以支持不同的传输层协议（TCP或UDP），当使用TCP协议进行连接时，该Socket连接就是一个TCP连接。  
Socket是应用层与传输层之间的一个抽象层。它把传输层的很多复杂操作封装成一些简单的接口，来让应用层调用以此来实现进程在网络中的通信。因此，Socket并不直接属于OSI模型或TCP/IP模型的任何一层，而是作为一个中间层存在，使得应用层可以更容易地使用传输层的服务  


# FTP
### 工作流程：
**1）建立连接**  
FTP连接需要FTP服务器和客户端两方在网络上建立通信。建立FTP连接时会有两个不同的通信通道。一个被称为命令通道，它的作用是发出和响应指令。另一个为数据通道，用于客户端和服务器端进行数据交互。  
**2）身份验证**  
使用FTP传输文件时，用户需要通过向FTP服务器提供凭据来获得文件传输许可。这些凭据通常是明文的用户名和密码，但如果服务器配置允许，用户也可以匿名连接。  
**3）数据传输**  
FTP可以在主动模式或被动模式下运行，这决定了数据连接是如何建立的。在主动模式下，客户端开始监听来自服务器的数据连接。在被动模式下，服务器使用命令通道向客户端发送开启数据通道所需的信息。  
**4）结束连接**  
数据传输完成后，数据连接会被关闭，但命令连接在整个FTP会话中都保持打开状态。  


##### 被动模式
1）建立命令连接：FTP客户端连接到FTP服务器的21端口，发送用户名和密码进行登录。  
2）发送PASV命令：登录成功后，当需要列出文件列表或读取数据时，客户端发送PASV命令到FTP服务器。  
3）服务器开放端口：服务器在本地随机开放一个端口（通常大于1024），然后把开放的端口告诉客户端。  
4）建立数据连接：客户端再连接到服务器开放的端口进行数据传输。  
被动模式的主要优点是，由于所有连接都是由客户端发起的，因此更容易通过防火墙。此外，由于服务器不需要连接到客户端的数据端口，因此客户端无需开放额外的端口。  

##### 安全性：
为了保护用户名和密码的传输安全，并对内容进行加密，FTP通常使用SSL/TLS进行安全化（被称为FTPS），或者被SSH文件传输协议（SFTP）所取代。
