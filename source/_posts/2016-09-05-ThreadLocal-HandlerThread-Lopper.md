---
layout: post
title:  ThreadLocal、HandlerThread、Lopper区别
category: accumulation
tags: accumulation
keywords: ThreadLocal, HandlerThread, Lopper
banner: http://obxk8w81b.bkt.clouddn.com/A%20Lane%20in%20the%20Public%20Garden%20at%20Arles.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/A%20Lane%20in%20the%20Public%20Garden%20at%20Arles.jpg
---


### 1.前言

    Android中非UI线程（WorkThread）不能操作UI线程（MainThread）

handler 发送Message 给MessageQueue，Looper 来轮询消息，如果有Message，然后再发送给Handler，Handler 拿到消息就可以所在的线程执行了。

### 2.ThreadLocal<T>

Thread这个类有一个变量：ThreadLocal.ThreadLocalMap threadLocals ，这是一个map的数据结构，里面的元素的key就是ThreadLocal，value就是我们自定义的一些目标类。我们可以在自己的多线程类中定义好几个ThreadLocal，然后每一个ThreadLocal put一个特定的目标类，然后以后可以用ThreadLocal get到目标类（用自己作为Thread里map的key），因为每个Thread有自己独自的map，所以这样可以实现每个线程有自己的LocalThread，并且一个Thread里可以有多个LocalThread。

<!--more-->

简单理解就是每个线程维护一个map，然后可以用一定的关键字取出这个map里的目标类（比如一个bean），这个“一定的关键字”说的就是这个ThreadLocal 。

ThreadLocal隔离了各个线程，让各线程之间没有什么共享的问题。

参考：[Android 中 Handler，Looper，HandlerThread 的使用](http://www.jianshu.com/p/08cb3665972f)


### 3.Looper
Looper是Android handler机制的重要组成部分，Looper这个名字起的很形象，翻译过来是：打环的人，就是维护一个循环的人。
Looper里有一个静态变量：private static final ThreadLocal sThreadLocal = new ThreadLocal();
这是典型的Android里用到ThreadLocal的一个情况，调用Looper.prepare的时候，唯一做的事情就是把sThreadLocal作为key，把一个new出来的looper对象作为value put到相应线程的map里。然后以后用到Looper.loop的时候，就从这个sThreadLocal里取出这个Looper，然后死循环（阻塞循环）MessageQueue，取出Message并执行message指向的Handler。

### 4.Handler
SDK中关于Handler的说明如下：

> A Handler allows you to sendand process Messageand Runnable objects associated with a thread's MessageQueue.Each Handler instance is associated with a single thread and that thread'smessage queue. When you create a new Handler, it is bound to the thread /message queue of the thread that is creating it -- from that point on, it willdeliver messages and runnables to that message queue and execute them as theycome out of the message queue.

#### 4.1 Handler的作用

> There are two main uses for aHandler: (1) to schedule messages and runnables to be executed as some point inthe future; and (2) to enqueue an action to be performed on a different thread than your own.

在线程中实例化Handler需要保证线程当中包含Looper(注意：UI-Thread默认包含Looper)。

#### 4.2 不是所有的Handler都能更新UI

Handler处理消息总是在创建Handler的线程里运行。而我们的消息处理中，不乏更新UI的操作，不正确的线程直接更新UI将引发异常。因此，需要时刻关心Handler在哪个线程里创建的。如何更新UI才能不出异常呢？SDK告诉我们，有以下4种方式可以从其它线程访问UI线程(也即线程间通信)：

- Activity.runOnUiThread(Runnable)
- View.post(Runnable)
- View.postDelayed(Runnable, long)
- 在UI线程中创建的Handler

几点小结
- Handler的处理过程运行在创建Handler的线程里
- 一个Looper对应一个MessageQueue，一个线程对应一个Looper，一个Looper可以对应多个Handler
- 不确定当前线程时，更新UI时尽量调用View.post方法
- handler应该由处理消息的线程创建。
- handler与创建它的线程相关联，而且也只与创建它的线程相关联。handler运行在创建它的线程中，所以，如果在handler中进行耗时的操作，会阻塞创建它的线程。
- Android的线程分为有消息循环的线程和没有消息循环的线程，有消息循环的线程一般都会有一个Looper。主线程（UI线程）就是一个消息循环的线程。
- Looper.myLooper();      //获得当前的Looper
  Looper.getMainLooper() //获得UI线程的Lopper
- Handle的初始化函数（构造函数），如果没有参数，那么他就默认使用的是当前的Looper，如果有Looper参数，就是用对应的线程的Looper。
- 如果一个线程中调用Looper.prepare()，那么系统就会自动的为该线程建立一个消息队列，然后调用Looper.loop();之后就进入了消息循环，这个之后就可以发消息、取消息、和处理消息。

### 5.消息的发送与处理

我们简单地看一下消息的循环过程：

#### 5.1 消息的生成
~~~ Java
Message msg =mHandler.obtainMessage();
msg.what = what;
msg.sendToTarget();
~~~

#### 5.2 消息的发送

~~~ Java
MessageQueue queue= mQueue;
  if (queue != null){
  msg.target =this;
  sent =queue.enqueueMessage(msg, uptimeMillis);
}
~~~
在Handler.java的sendMessageAtTime(Messagemsg, long uptimeMillis)方法中，我们看到，它找到它所引用的MessageQueue，然后将Message的target设定成自己（目的是为了在处理消息环节，Message能找到正确的Handler），再将这个Message纳入到消息队列中。

#### 5.3 消息的抽取
~~~ Java
Looper me =myLooper();
MessageQueue queue= me.mQueue;
while (true) {
  Message msg =queue.next(); // might block
  if (msg !=null) {
    if(msg.target == null) {
      // Notarget is a magic identifier for the quit message.
      return;
    }
    msg.target.dispatchMessage(msg);
    msg.recycle();
  }
}
~~~

在Looper.java的loop()函数里，我们看到，这里有一个死循环，不断地从MessageQueue中获取下一个（next方法）Message，然后通过Message中携带的target信息，交由正确的Handler处理（dispatchMessage方法）。

#### 5.4 消息的处理
~~~ Java
if (msg.callback!= null) {
  handleCallback(msg);
} else {
  if (mCallback!= null) {
    if(mCallback.handleMessage(msg)) {
      return;
    }
  }
 handleMessage(msg);
}
~~~
在Handler.java的dispatchMessage(Messagemsg)方法里，其中的一个分支就是调用handleMessage方法来处理这条Message，而这也正是我们在职责处描述使用Handler时需要实现handleMessage(Messagemsg)的原因。
至于dispatchMessage方法中的另外一个分支，我将会在后面的内容中说明。
至此，我们看到，一个Message经由Handler的发送，MessageQueue的入队，Looper的抽取，又再一次地回到Handler的怀抱。而绕的这一圈，也正好帮助我们将同步操作变成了异步操作。

参考上面的消息的发送与处理，这里再解释一下View.post(Runnable)方法。

- 在post(Runnableaction)方法里，View获得当前线程（即UI线程）的Handler，然后将action对象post到Handler里。
- 在Handler里，它将传递过来的action对象包装成一个Message（Message的callback为action），然后将其投入UI线程的消息循环中。
- 在 Handler再次处理该Message时，有一条分支就是为它所设，直接调用mCallback.handleMessage的方法，返回到runnable的run方法。
- 而此时，已经路由到UI线程里，因此，我们可以毫无顾虑的来更新UI。


### 5.HandlerThread
HandlerThread就是在普通的Thread基础上加上了Looper的支持，让用户不必自己去创建Looper了，同时方便了Handler的使用。

创建HandlerThread时需要把它启动了，即调用start()方法。然后创建Handler时将HandlerThread中的looper对象传入。

~~~ Java
HandlerThread thread = new HandlerThread("MyHandlerThread");
thread.start();
mHandler = new Handler(thread.getLooper());
mHandler.post(new Runnable(){...});
~~~

那么这个Handler对象就是与HandlerThread这个线程绑定了（这时就不再是与UI线程绑定了，这样在Handler中处理耗时操作将不会阻塞UI）。

如果想让HandlerThread退出，则需要调用handlerThread.quit()。

具体可以看下面代码：

~~~ Java
public class HandlerThreadActivity extends Activity {
    private static final String TAG = "HandlerThreadActivity";
    private HandlerThreadmHandlerThread;
    private MyHandler mMyHandler;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
       // TODO Auto-generatedmethod stub
       super.onCreate(savedInstanceState);
       TextView text = new TextView(this);
       text.setText("HandlerThreadActivity");
       setContentView(text);

       Log.d(TAG, "The mainthread id = " + Thread.currentThread().getId());

       //生成一个HandlerThread对象，实现了使用Looper来处理消息队列的功能，
       //这个类由Android应用程序框架提供
       mHandlerThread = new HandlerThread("handler_thread");

       //在使用HandlerThread的getLooper()方法之前，必须先调用该类的start();
       mHandlerThread.start();
       //即这个Handler是运行在mHandlerThread这个线程中
       mMyHandler = new MyHandler(mHandlerThread.getLooper());

       mMyHandler.sendEmptyMessage(1);
    }

    private class MyHandler extends Handler {

       public MyHandler(Looper looper) {
           super(looper);
       }

       @Override
       public void handleMessage(Message msg) {
           Log.d(TAG, "MyHandler-->handleMessage-->threadid = " + Thread.currentThread().getId());
           super.handleMessage(msg);
       }
    }
}
~~~
