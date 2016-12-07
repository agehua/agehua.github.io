---
layout: post
title:  Hexo 出错情况收集
category: accumulation
tags: gradle
keywords: Android,gradle
banner: http://obxk8w81b.bkt.clouddn.com/Couple%20Out%20for%20a%20Stroll.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Couple%20Out%20for%20a%20Stroll.jpg
---

#### Error: Cannot find module './build/Release/DTraceProviderBindings'

我在更新hexo，准备安装别的module时，不小心按到了 ctrl+c，结果发现使用hexo命令会报如下错误：
~~~ Java
{ [Error: Cannot find module './build/Release/DTraceProviderBindings'] code: 'MODULE_NOT_FOUND' }    
{ [Error: Cannot find module './build/default/DTraceProviderBindings'] code: 'MODULE_NOT_FOUND' }
{ [Error: Cannot find module './build/Debug/DTraceProviderBindings'] code: 'MODULE_NOT_FOUND' }
~~~
<!--more-->
网上一大堆的解决办法都不好用，这里特别记录下我找到的一个解决办法，亲测可用

**1. 重新安装Hexo，需要手动删除隐藏的hexo文件**

- 需要删除 /usr/local/lib/node_modules/ 目录下的**hexo文件夹**
- /usr/local/bin/ 下的**hexo文件**. 直接删掉就好了.
> 注意这两个都是隐藏文件，要想在finder中显示隐藏文件，需要在Terminal中执行：
defaults write com.apple.finder AppleShowAllFiles -bool true

**2. 重新执行命令：npm install -g hexo --save**

这样就可以了  :)
