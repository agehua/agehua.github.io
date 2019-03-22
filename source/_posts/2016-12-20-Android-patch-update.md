---
layout: post
title:  Android 增量更新整理
category: accumulation
tags:
  - ANDROID
  - Patch Update
keywords: Android Patch Update
banner: http://cdn.conorlee.top/A%20Girl%20in%20the%20Street,%20Two%20Coaches%20in%20the%20Background.jpg
thumbnail: http://cdn.conorlee.top/A%20Girl%20in%20the%20Street,%20Two%20Coaches%20in%20the%20Background.jpg
toc: true
---

### 背景
在前几年，整体移动网络环境相比现在差很多，加之流量费用又相对较高，因此每当我们发布新版本的时候，一些用户升级并不是很积极，这就造成了新版本的升级率并不高。而google为了解决了这个问题，提出了Smart App Update，即增量更新（也叫做差分升级）。

增量更新与热修复完全不是一个东西。增量更新需要重新安装apk，而热修复不需要。热修复也能完成部分增量更新的功能。


### 增量更新的流程
增量更新的具体流程是：用户手机上安装着某个应用，下载了增量包，手机上的apk和增量包合并形成新的包，然后再次安装（注意这个过程是要重新安装的，当然部分应用市场有root权限你可能感知不到）。

<!--more-->

### 增量更新的原理
增量更新的原理也很简单，就是将手机上已安装的旧版本apk与服务器端新版本apk进行二进制对比，并得到差分包（patch），用户在升级更新应用时，只需要下载差分包，然后在本地使用差分包与旧版的apk合成新版apk，然后进行安装。差分包文件的大小，那就远比APK小得多了，这样也便于用户进行应用升级。

那么增量更新的流程可以细化为几个关键点：

- 1.用户手机上提取当前安装应用的apk
- 2.如何利用old.apk和new.apk生成增量文件（差分包）
- 3.增加文件与1.中的old.apk合并，然后安装

旧版的APK可以在**/data/app/%packagename%/**下找到。

也可以参考下面的代码：

~~~ JavaScript
public class ApkExtract {
    public static String extract(Context context) {
        context = context.getApplicationContext();
        ApplicationInfo applicationInfo = context.getApplicationInfo();
        String apkPath = applicationInfo.sourceDir;
        Log.d("hongyang", apkPath);
        return apkPath;
    }
}
~~~

这部分可以参考[张鸿洋的博客](http://blog.csdn.net/lmj623565791/article/details/52761658)

#### 关于生成差分包
制作差分包的工具为[bsdiff](http://www.daemonology.net/bsdiff/bsdiff-4.3.tar.gz)，

网址：

http://www.daemonology.net/bsdiff/

下载地址：

http://www.daemonology.net/bsdiff/bsdiff-4.3.tar.gz

这是一个非常牛的二进制查分工具，bsdiff源代码在Android的源码目录下 ”\\external\\bsdiff“”这边也可以找到。另外还需要依赖[bzlib](http://www.bzip.org/downloads.html)来进行打包。在安全性方面，补丁和新旧版APK最好都要进行MD5验证，以免被篡改。

关于这个工具可以使用别人编译好的so库，[这里](https://github.com/hongyangAndroid/BsDiff_And_Patch/tree/master/so-dist)；熟悉NDK开发的也可以自己编译，[请看这里](http://blog.csdn.net/lmj623565791/article/details/52761658)。这里就不在考虑这个工具问题了。

- 生成增量文件
~~~ JavaScript
./bsdiff old.apk new.apk old-to-new.patch
~~~
这样就生成了一个增量文件old-to-new.patch

#### 关于合并差分包
生成差分包肯定是在服务端，合并差分包才是Android客户端需要做的：

- 增量文件和old.apk合并成新的apk
~~~ JavaScript
./bspatch old.apk new2.apk old-to-new.patch
~~~
这样就生成一个new2.apk

生成后，要查看下两个文件的md5值。如果两个文件md5值一致，那么几乎可以肯定两个文件时一模一样的。


### 增量更新存在的不足
- 1、增量升级是以两个应用版本之间的差异来生成补丁的，但是我们无法保证用户每次的及时升级到最新，也就是在更新前，新版和旧版只差一个版本，所以必须对你所发布的每一个版本都和最新的版本作差分，以便使所有版本的用户都可以差分升级，这样相对就比较繁琐了。解决方法也有，可以通过Shell脚本来实现批量生成。

- 2.增量升级能成功的前提是，从手机端能够获得旧版APK，并且与服务端的APK签名是一样的，所以像那些破解的APP就无法实现更新。前面也提到了，为了安全性，防止补丁合成错误，最好在补丁合成前对旧版本的apk进行sha1或者MD5校验，保证基础包的一致性，这样才能顺利的实现增量升级。

想要封装一套增量更新的工具，请看这篇文章：[打造Android万能的软件更新库，修改只要一点点](http://blog.csdn.net/huang_cai_yuan/article/details/52927630)
