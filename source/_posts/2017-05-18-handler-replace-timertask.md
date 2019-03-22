---
layout: post
title:  Handler替代TimerTask
category: accumulation
tags:
  - TimerTask
  - handler
keywords: handler, TimerTask
banner: http://cdn.conorlee.top/Blossoming%20Almond%20Branch%20in%20a%20Glass%20with%20a%20Book.jpg
thumbnail: http://cdn.conorlee.top/Blossoming%20Almond%20Branch%20in%20a%20Glass%20with%20a%20Book.jpg
toc: true
---

### Handler替代TimerTask
原文来自：http://www.mopri.de/2010/timertask-bad-do-it-the-android-way-use-a-handler/

作者文中提到，使用TimerTask更新GUI，debug看起来可以，但实际上根本不起作用。
同时作者发现，使用handler可以得到更好的表现。

下面是一个例子，使用handler在100ms后启动一个Runnable:
~~~ Java
private Handler handler = new Handler();
handler.postDelayed(runnable, 100);
~~~

下面代码里有一个小技巧，实现每隔100ms运行一次Runnable，就像TimerTask的scheduleAtFixedRate()方法:

<!--more-->
~~~ Java
private Runnable runnable = new Runnable() {
   @Override
   public void run() {
      /* do what you need to do */
      foobar();
      /* and here comes the "trick" */
      handler.postDelayed(this, 100);
   }
};
~~~
如何取消运行Runnable呢？只需要调用handler.removeCallback(runnable)就可以了。

这样做还有另一个有点，就是不用总是new Timer(Task)了，可以重复使用上面代码中的handler和runnable。
