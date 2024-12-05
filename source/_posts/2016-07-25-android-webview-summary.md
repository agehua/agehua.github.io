---
layout: post
title: Android Webview总结，不断更新中 :(
category: accumulation
tags:
  - ANDROID
  - webview
keywords: Android Webview, js漏洞
description: Android Webview总结，遇到问题就在这里记录 :(
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Cottages%20Reminiscence%20of%20the%20North.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Cottages%20Reminiscence%20of%20the%20North.jpg
---


### 1.Android Webview的坑

- 1.webview再次加载页面空白

  - 1.可以关闭掉硬件加速

  - 2.不关闭硬件加速的情况下：在关闭Acivity之前手动调用下面方法，

  <!--more-->

  ~~~ Java
   public void goFinish(){
        isLoadWithError =false;
        if (null!=jsCallBack)
            mWebView.removeJavascriptInterface("XXX");
        mWebView.setFocusable(true);
        mWebView.removeAllViews();
        try {
        	mWebView.clearHistory(); //webview没有历史记录，这里会抛出异常
        }catch(Exception e){
            e.printStackTrace();
        }
        mWebView.destroy();

        try {
        	//mWebView为WebView所在类的全局变量名，不可以混淆
            Field fieldWebView = this.getClass().getDeclaredField("mWebView");
            fieldWebView.setAccessible(true);
            WebView webView = (WebView) fieldWebView.get(this);
            webView.removeAllViews();
            webView.destroy();

        }catch (NoSuchFieldException e) {
            e.printStackTrace();

        }catch (IllegalArgumentException e) {
            e.printStackTrace();

        }catch (IllegalAccessException e) {
            e.printStackTrace();

        }catch(Exception e){
            e.printStackTrace();
        }
        this.finish();
    }
   ~~~

> 注意：因为用到了反射去清理webview，所以混淆时，**这个方法所在的类不能混淆**


- 2.部分手机h5 game屏幕闪烁

	在小米的某些手机上，会出现这种情况。抱歉，还没有好的解决办法。谁有好的解决办法可以邮件告知，多谢 :)

- 3.android Webview替代品Crosswalk

	Crosswalk解决4,2以下的手机浏览器的兼容性问题。对html5的支持更好

	[Crosswalk官网](https://crosswalk-project.org/)

	但Crosswalk也有缺点，将Crosswalk嵌入App中，会使APK增加大约20M。具体可以看这个[知乎提问](https://www.zhihu.com/question/26484511)

- 4.腾讯浏览服务

	[官网地址](http://x5.tencent.com/index)

### 2.WebView防止远程代码攻击

- 1.使用Android4.2以上的系统，通过在Java的远程方法上面声明一个@JavascriptInterface，可以预防改安全漏洞  

- 2.低于Android4.2的系统，如果系统自己添加了一个叫searchBoxJavaBridge_的Js接口，则需要把这个接口删除

  详情参见这篇文章：[Android WebView的Js对象注入漏洞解决方案](http://blog.csdn.net/leehong2005/article/details/11808557/)

  这里贴一个自己整理的webview类：[BaseWebView](https://gist.github.com/agehua/99233b40e05db29ee0ed4f50fb2c7530)

### 3.Android中WebView的JavaScript代码和本地代码交互的三种方式
来自姜维的博客:[Android中WebView的JavaScript代码和本地代码交互的三种方式](http://blog.csdn.net/jiangwei0910410003/article/details/52687530)
