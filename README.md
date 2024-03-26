# cFtpClient

##### 环境
mac Zsh  
c+Linux  

##### 实现内容
参考 FTP 协议标准，使用TCP Socket的基础ftp客户端  
目前实现的功能：  
1）用户登录（用户名+密码）  
2）显示目录：ls  
3）转换目录：  
  到指定目录：cd /dirName  
  到上一级目录：cd ..  
  到当前目录：cd .  
4）下载文件：get fileName  
5）退出客户端：bye  


##### 运行方法
复制.c和.h文件到终端，命令行输入
```bash
//编译并生成可执行文件
gcc ftpClient.c network.c -o cli

//运行客户端
./cli xxx.xxx.xxx.xxx
```
