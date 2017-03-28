---
layout: post
title:  linux(centos) 搭建SVN服务器
category: accumulation
tags: svn server
keywords: linux, svn服务器
banner: http://obxk8w81b.bkt.clouddn.com/Bank%20of%20the%20Oise%20at%20Auvers.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Bank%20of%20the%20Oise%20at%20Auvers.jpg
toc: true
---

### linux(centos) 搭建SVN服务器

安装步骤如下：

#### 第一步：安装SVN
~~~ Java
yum install subversion
~~~
#### 第二步：创建SVN版本库目录
~~~ Java
mkdir -p /var/svn/svnrepos
~~~
#### 第三步：创建版本库
<!--more-->
~~~ Java
svnadmin create /var/svn/svnrepos
~~~
#### 第四步：进入conf目录（该SVN版本库配置文件）

authz文件是权限控制文件
passwd是帐号密码文件
svnserve.confSVN服务配置文件

#### 第五步：修改svnserve.conf文件

vi svnserve.conf打开下面的几个注释：

~~~ Java
anon-access = read #匿名用户可读
auth-access = write #授权用户可写
password-db = passwd #使用哪个文件作为账号文件
authz-db = authz #使用哪个文件作为权限文件
realm = /var/svn/svnrepos # 认证空间名，版本库所在目录
~~~

> svnserve.conf、passwd、authz文件中各配置项前不要有空格

#### 第六步：设置帐号密码
~~~ Java
vi passwd
~~~
在[users]块中添加用户和密码，格式：帐号=密码，如binjoo = 123456。

~~~ Java
[users]
binjoo = 123456
~~~
#### 第七步：设置权限

~~~ Java
vi authz
~~~
在末尾添加如下代码：

~~~ Java
[groups]
[/]
binjoo=rw
~~~
意思是版本库的根目录binjoo对其有读写权限。

#### 第八步：启动svn版本库
~~~ Java
svnserve -d -r /var/svn/svnrepos
~~~
链接方式svn://127.0.0.1

### 遇到问题
在客户端试图 svn merge 总是报svn: E220001: 遇到不可读的路径；拒绝访问。这个错误

提示 : SVN 遇到不可读的路径；拒绝访问。 英文是: Unreadable path encountered; access denied;

既然看不到日志又无法merge等操作. GOOGLE了一下，下面的方法解决了问题。

后面才发现是配置问题.

在项目的conf/svnserve.conf 中, 设置 anon-access = none 即可. 然后重启Subversion服务.

如果本地SVN客户端查看过日志会有缓存, 需要在 设置->日志缓存->缓存的版本库 中删除有问题的版本缓存 再重新查看日志就好了.

### linux svn启动和关闭
- 1，启动SVN
~~~ Java
sudo svnserve -d -r /var/svn/svnrepos
~~~
其中 -d 表示守护进程， -r 表示在后台执行
/var/svn/svnrepos  为svn的安装目录


- 2，关闭SVN
这里采取linux杀死进程的方式处理的
~~~ Java
ps -ef|grep svnserve
root      4967     1  0 Aug23 ?        00:00:00 svnserve -d -r repository/  
~~~
这里  kill -9 4967杀死进程， 此4967为进程号
