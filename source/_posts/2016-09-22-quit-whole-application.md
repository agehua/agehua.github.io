---
layout: post
title:  Android 退出整个应用程序解决方案
category: technology
tags:
  - ANDROID
  - exitApplication
keywords: Android,退出整个应用
banner: http://obxk8w81b.bkt.clouddn.com/Enclosed%20Field%20with%20Ploughman.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Enclosed%20Field%20with%20Ploughman.jpg
---


关于这个功能，每个公司项目都有自己的解决方案

搜索了一下，网上大概有以下几种方法解决这个问题：

- (1)System.exit(0);

- (2)Process.killProcess(Process.myPid());

- (3)ActivityManager activityManager = (ActivityManager) this.getSystemService(Context.ACTIVITY_SERVICE);

    activityManager.restartPackage("packagename");


但这些貌似不是万能的，有的说在2.2版本后就失效了，那我们怎么办呢？

<!--more-->

### 1、通过广播

在起初的一个项目里我用了一个比较笨的方法，我用一个类来专门管理所有的Activity。这个类里有一个List，每打开一个Activity我就把这个Activity放到这个List中，当退出时再将List中所有的Activity一个一个的关闭。

在后来的项目中发现了一个更好地方法，就是通过广播来完成退出功能，具体实现过程是这样的：在每个Activity创建时（onCreate时）给Activity注册一个广播接收器，当退出时发送该广播即可。大概的代码如下：

~~~ Java
@Override
protected void onCreate(Bundle savedInstanceState) {

       super.onCreate(savedInstanceState);
       IntentFilter filter = new IntentFilter();
       filter.addAction("finish");
       registerReceiver(mFinishReceiver, filter);
       ……
}

private BroadcastReceiver mFinishReceiver = new BroadcastReceiver() {

    @Override
    public void onReceive(Context context, Intent intent) {
           if("finish".equals(intent.getAction())) {
              Log.e("#########", "I am " + getLocalClassName()
                     + ",now finishing myself...");
              finish();
       }
    }
};
~~~

相信聪明的大家会把上面的代码写在一个基类里面，因为如果你的项目中Activity很多的话，写起来很麻烦，而且也不符合代码规范。

在退出时执行以下代码即可关闭所有界面完全退出程序：

getApplicationContext().sendBroadcast(new Intent("finish"));

转载：http://www.cnblogs.com/wader2011/archive/2011/10/10/2205161.html

### 2、使用退出类

~~~ Java
public class CloseActivity
{
    private static LinkedList<Activity> acys = new LinkedList<Activity>();

    public static Activity curActivity;

    public static void add(Activity acy)
    {
        acys.add(acy);
    }

    public static void remove(Activity acy) {
        acys.remove(acy);
    }

    public static void close()
    {
        Activity acy;
        while (acys.size() != 0)
        {
            acy = acys.poll();
            if (!acy.isFinishing())
            {
                acy.finish();
            }
        }
//        android.os.Process.killProcess(android.os.Process.myPid());
    }
}
~~~

### 3.利用activity stack的原理
我们知道Android的窗口类提供了历史栈，我们可以通过stack的原理来巧妙的实现，
这里我们在D窗口打开A窗口时在Intent中直接加入标志Intent.FLAG_ACTIVITY_CLEAR_TOP，再次开启A时将会清除该进程空间的所有Activity。

在D中使用下面的代码:

~~~ Java
Intent intent = new Intent();
intent.setClass(D.this, A.class);
intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);  //注意本行的FLAG设置
startActivity(intent);
finish();
~~~

关掉自己，在A中加入代码：

~~~ Java
@Override
protected void onNewIntent(Intent intent) { // TODO Auto-generated method stub
    super.onNewIntent(intent);
        //退出
    if ((Intent.FLAG_ACTIVITY_CLEAR_TOP & intent.getFlags()) != 0) {
         finish();
    }
}
~~~

      A的Manifest.xml配置成android:launchMode="singleTop"

原理总结： 一般A是程序的入口点，从D起一个A的activity，加入标识Intent.FLAG_ACTIVITY_CLEAR_TOP这个过程中会把栈中B，C，都清理掉。因为A是android:launchMode="singleTop" 不会调用oncreate(),而是响应onNewIntent（）这时候判断Intent.FLAG_ACTIVITY_CLEAR_TOP，然后把A finish（）掉。 栈中A,B,C,D全部被清理。所以整个程序退出了。


### 4.JNI实现守护进程
请看这篇博客[Android 通过JNI实现守护进程](http://blog.csdn.net/yyh352091626/article/details/50542554)
