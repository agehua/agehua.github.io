---
layout: post
title:  MAC下用iTerm2连接远程主机
category: accumulation
tags:
  - linux
  - iterm
  - scp
keywords: iterm, linux scp
banner: http://obxk8w81b.bkt.clouddn.com/Arles%20View%20from%20the%20Wheat%20Fields.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Arles%20View%20from%20the%20Wheat%20Fields.jpg
toc: true
---

### MAC下用iTerm2连接远程主机
iTerm2是MAC的一个终端工具。
简单介绍一个小功能，使用iTerm2连接远程主机：

- 1.先打开一个iTerm2窗口，选择菜单profiles中的Open Profiles，然后选择Edit Profiles中创建一个新的Profile，随便命名成容易记的。
- 2.选择command，输入ssh -A -p xxx root@输入远程计算机的IP地址（xxx表示端口）。
- 3.然后选择这个新的profile，New一个window就可以连接远程服务器了。

### SCP命令使用说明
<!--more-->
scp用来在网络上不同的主机之间复制文件，它使用ssh安全协议传输数据，具有和ssh一样的验证机制，从而安全的远程拷贝文件。

下面是一个简单例子：

首先创建一个本地文件
~~~ C++
echo hello, world > a-file.txt
~~~

copy本地文件到服务器的命令如下：
~~~ C++
scp <local file> <remote user>@<remote machine>:<remote path>
~~~

如果想Copy远程文件到本地，则是：
~~~ C++
scp <remote user>@<remote machine>:<remote path> <local file>
~~~

如果想复制目录也是可以的：
~~~ C++
scp -r local_folder remote_username@remote_ip:remote_folder
~~~

比如下面命令：
~~~ C++
scp -r /home/space/jdk-8u121-linux-x64.tar.gz root@107.182.178.94:/home/root/others/
~~~

参数说明：

[root@tank test]# scp --help
usage: scp [-1246BCpqrv] [-c cipher] [-F ssh_config] [-i identity_file] [-l limit] [-o ssh_option] [-P port] [-S program] [[user@]host1:]file1 [...] [[user@]host2:]file2  

-1                        强制scp命令使用协议ssh1
-2                        强制scp命令使用协议ssh2
-4                        强制scp命令只使用IPv4寻址
-6                        强制scp命令只使用IPv6寻址
-B                        使用批处理模式（传输过程中不询问传输口令或短语）
-C                        允许压缩。（将-C标志传递给ssh，从而打开压缩功能）
-p                         保留原文件的修改时间，访问时间和访问权限。
-q                         不显示传输进度条。
-r                          递归复制整个目录。
-v                          详细方式显示输出。scp和ssh(1)会显示出整个过程的调试信息。这些信息用于调试连接，验证和配置问题。
-c cipher              以cipher将数据传输进行加密，这个选项将直接传递给ssh。
-F ssh_config      指定一个替代的ssh配置文件，此参数直接传递给ssh。
-i identity_file      从指定文件中读取传输时使用的密钥文件，此参数直接传递给ssh。
-l limit                    限定用户所能使用的带宽，以Kbit/s为单位。
-o ssh_option      如果习惯于使用ssh_config(5)中的参数传递方式，
-P port                  注意是大写的P, port是指定数据传输用到的端口号
-S program         指定加密传输时所使用的程序。此程序必须能够理解ssh(1)的选项。

转载自：http://blog.51yip.com/linux/1027.html


#### iTerm从mac传文件到linux

~~~
sudo scp -P 28547 ~/Downloads/nexus-3.2.1-01-unix.tar.gz root@107.182.178.94:/usr/local
~~~
-P 指定linux服务器的端口号，必须是大写
