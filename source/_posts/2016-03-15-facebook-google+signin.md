---
layout: post
title: 近期总结facebook google+ Twitter sign-in fragment使用
category: accumulation
tags: accumulation
keywords: android, google map, fragments
description: google map2.0使用总结
banner: http://obxk8w81b.bkt.clouddn.com/Cottages%20with%20a%20Woman%20Working%20in%20the%20Foreground.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Cottages%20with%20a%20Woman%20Working%20in%20the%20Foreground.jpg
---


### 1.相关资料
blog: [关于Google+以及Facebook第三方登录实现的一点总结](http://www.cnblogs.com/lngg057/p/5020192.html)

g+官方教程: [G+ start](https://developers.google.com/identity/sign-in/android/start)

facebook官方教程: [Facebook start](https://developers.facebook.com/docs/android/getting-started)

iCCP: [Not recognizing known sRGB profile](http://www.bigademo.com/iccp-not-recognizing-known-srgb-profile/)

<!--more-->

- 今天做分享的时候遇到了这个问题：

    [2016-04-01 11:24:04 - Dex Loader] Unable to execute dex: method ID not in [0, 0xffff]: 65536
    [2016-04-01 11:24:04 - VIVAT_SHARESDK] Conversion to Dalvik format failed: Unable to execute dex: method ID not in [0, 0xffff]: 65536

[大项目中遇到的问题看这个博客](http://www.cnblogs.com/yaozhongxiao/p/3521428.html)

- iCCP: Not recognizing known sRGB profile 删除png图片内嵌的iCCP profile sRGB报错

今天有碰见一个坑，改其他代码，然后在编译的时候就出现这个问题，对就是这个问题。网上查了资料，也就这个资料最全面，大家可以去看见http://my.oschina.net/1pei/blog/479162?fromerr=ARrUPlGS

处理这个问题我使用了一种方法，记录下来以便以后使用

步骤1:下载Image Magick http://www.imagemagick.com.cn/download.html.如果是windows的，请下载含dll的

步骤2： 在要处理的文件夹使用如下命令 ，一定要在要处理的文件夹使用

~~~ C++
//WINDOWS使用
set fn=E:\Program Files\ImageMagick-6.9.0-Q16\convert.exe  
for /f "tokens=*" %i in ('dir/s/b *.png') do "%fn%" "%i" -strip "%i"
（因为是window的，所以把%%i改为%i）
~~~

~~~ C++
//LINUX使用
 set fn=E:\Program Files\ImageMagick-6.9.0-Q16\convert.exe  
for /f "tokens=*" %%i in ('dir/s/b *.png') do "%fn%" "%%i" -strip "%%i"
~~~

### 2.遇到问题

- 1.“This client application's callback url has been locked”.

    使用Twitter signin时遇到了这个问题，这个错误信息是在logcat中找到的，原因是在[Twitter](https://apps.twitter.com)的Settings里勾选了“Enable Callback Locking (It is recommended to enable callback locking to ensure apps cannot overwrite the callback url)”选项，这个选项表示不允许app本地更改callback url。也可看这个[页面](https://twittercommunity.com/t/callback-url-is-locked/59481)

### 3.add() vs. replace()
- 只有在Fragment数量大于等于2的时候，调用add()还是replace()的区别才能体现出来。

    当通过add()连续两次添加Fragment的时候，每个Fragment生命周期中的onAttach()-onResume()都会被各调用一次，而且两个Fragment的View会被同时attach到containerView。

    同样，退出Activty时，每个Fragment生命周期中的onPause()-onDetach()也会被各调用一次。

    但当使用replace()来添加Fragment的时候，第二次添加会导致第一个Fragment被销毁，即执行第二个Fragment的onAttach()方法之前会先执行第一个Fragment的onPause()-onDetach()方法，同时containerView会detach第一个Fragment的View。

- 调用show() & hide()方法时.

    Fragment的生命周期方法并不会被执行，仅仅是Fragment的View被显示或者​隐藏。而且，尽管Fragment的View被隐藏，但它在父布局中并未被detach，仍然是作为containerView的childView存在着。相比较下，attach() & detach()做的就更彻底一些。一旦一个Fragment被detach()，它的onPause()-onDestroyView()周期都会被执行。

    同时Fragment的View也会被detach。在重新调用attach()后，onCreateView()-onResume()周期也会被再次执行。

- remove()

    其实看完上面的分析，remove()方法基本也就明白了。相对应add()方法执行onAttach()-onResume()的生命周期，remove()就是完成剩下的onPause()-onDetach()周期。


### 4.FragmentTransaction add 和 replace 区别
使用 FragmentTransaction 的时候，它提供了这样两个方法，一个 add ， 一个 replace .add 和 replace 影响的只是界面，而控制回退的，是事务。

- add 是把一个fragment添加到一个容器 container 里。

    Add a fragment to the activity state. This fragment may optionally also have its view (if Fragment.onCreateView returns non-null) into a container view of the activity.

~~~ Java
public abstract FragmentTransaction add (int containerViewId, Fragment fragment, String tag)
~~~


- replace 是先remove掉相同id的所有fragment，然后在add当前的这个fragment。

    Replace an existing fragment that was added to a container. This is essentially the same as calling remove(Fragment) for all currently added fragments that were added with the same containerViewId and then add(int, Fragment, String) with the same arguments given here.

~~~ Java
public abstract FragmentTransaction replace (int containerViewId, Fragment fragment, String tag)
~~~



在大部分情况下，这两个的表现基本相同。因为，一般，咱们会使用一个FrameLayout来当容器，而每个Fragment被add 或者 replace 到这个FrameLayout的时候，都是显示在最上层的。所以你看到的界面都是一样的。但是，使用add的情况下，这个FrameLayout其实有2层，多层肯定要比一层的来得浪费，所以还是推荐使用replace。当然有时候还是需要使用add的。比如要实现轮播图的效果，每个轮播图都是一个独立的Fragment，而他的容器FrameLayout需要add多个Fragment，这样他就可以根据提供的逻辑进行轮播了。

而至于返回键的时候，这个跟事务有关，跟使用add还是replace没有任何关系。

###	5.要想fragment完整地执行生命周期
fragment跳转是要使用replace()方法，并一定要指定tag，否则有些方法不会执行（比如onResume），例如：

~~~ Java
getFragmentManager()
 	.beginTransaction()
    .replace(R.id.base_container,
    	 inputVerifyCodeFragment,"tag_code")
    .addToBackStack(null).commit();
~~~

### 6.fragment事件穿透
如果发现fragment2的点击事件可以被fragment栈下一层的fragment1获取到，可以在fragment2布局的根部加上：android:clickable="true"。问题解决
