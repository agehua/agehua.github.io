---
layout: post
title:  apk中含有admob广告插件
category: accumulation
tags:
  - ANDROID
  - remove admob
keywords: admob, 去除广告插件
banner: http://cdn.conorlee.top/Blossoming%20Pear%20Tree.jpg
thumbnail: http://cdn.conorlee.top/Blossoming%20Pear%20Tree.jpg
---


### 问题出现

使用这个杀毒软件[antivirus-for-android](http://www.avg.com/no-en/antivirus-for-android)，检测我们团队的apk后，AVG提示警告unwanted software。

生成的报告说是：
Beware of Adware! 1 Types of Adware Detected
Last reported adware activity:	Sep 16
Days with adware in last 30:	1 days

见下图：

<!--more-->

![avg_warning](http://blog.conorlee.top/blogimages/2016/avg_warning.PNG)

### 排查

使用这个广告插件检测工具[全能工具箱](https://play.google.com/store/apps/details?id=imoblife.toolbox.full&hl=zh)，检测后，确实提示我们的apk含有广告“admob”

但是这个apk是我们团队自己开发的，有项目源码，AndroidManifest文件中没有网络上说的Google AdMob注册，更没有com.google.ads.AdView这个布局文件

代码中没有，可能在第三方库中？

结果就是在排查第三方库的过程中，发现google service的jar包中有名字是ads的文件夹。如下图：
![google_servcie_ads](http://blog.conorlee.top/blogimages/2016/google_servcie_ads.PNG)

删除后上图中的两个ads文件夹，再打包apk后，用软件检测，果然没有admob插件了。哈哈，恼人的问题解决了。
