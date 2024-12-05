---
layout: post
title: IntentService和AsyncTask的区别
category: accumulation
tags:
    - IntentService
    - AsyncTask
keywords: IntentService, AsyncTask
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Enclosed%20Field%20with%20Rising%20Sun.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Enclosed%20Field%20with%20Rising%20Sun.jpg
toc: true
---

### IntentService和AsyncTask的区别

先说总结：
**Service**：Service只适合处理长期后台执行的，这里的后台是指与前台Activity无关的东西，但跟Activity同样运行在UI线程。同时Service开销比Thread大，Service本身不能解决多线程问题。
**同一个Service多次启动**，只会在第一次启动时回调onStart()或onBind()方法，并多次回调onStartCommand方法。
<!--more-->
**IntentService**：IntentService，是一个抽象类。继承自Service，这使它的优先级比单纯的线程要高很多，同时内部封装了一个HandlerThread 和 Handler，导致它可以执行异步任务，所以它是比较适合一些高优先级的后台任务。
`HandlerThread是在子线程使用Handler的方式，内部实现了Loop循环`

但是IntentService，内部队列是MessageQueue，**多次调用同一个IntentService，只能是阻塞式执行任务**，即当前个任务执行完才会取得下一个任务并执行。

IntentService 默认实现了 OnBind()，返回值为 null。在 onHandleIntent(Intent intent)回调中，处理耗时任务。

那么，用 IntentService 有什么好处呢？首先，可以省去了在 Service 中手动开线程的麻烦，第二，任务执行完后，IntentService 会自动停止，不用手动停止 Service。


**AsyncTask**也是一个抽象类，自定义一个类继承自AsyncTask。

> AsyncTask内容节选自郭霖大神的这篇文章：[Android AsyncTask完全解析，带你从源码的角度彻底理解](http://blog.csdn.net/guolin_blog/article/details/11711405)

注意，AsyncTask有3个泛型参数：

- 1.Params: 在执行AsyncTask时需要传入的参数，可用于在后台任务中使用。
- 2.Progress: 后台任务执行时，如果需要在界面上显示当前的进度，则使用这里指定的泛型作为进度单位。
- 3.Result:当任务执行完毕后，如果需要对结果进行返回，则使用这里指定的泛型作为返回值类型。

一个最简单的自定义AsyncTask就可以写成如下方式：
~~~ Java
class DownloadTask extends AsyncTask<Void, Integer, Boolean> {
    ……
}
~~~

AsyncTask在Android 3.0之前同一时刻能够运行的线程数为5个，线程池总大小为128。也就是说当我们启动了10个任务时，只有5个任务能够立刻执行，另外的5个任务则需要等待，当有一个任务执行完毕后，第6个任务才会启动，以此类推。而线程池中最大能存放的线程数是128个，当我们尝试去添加第129个任务时，程序就会崩溃。

而3.0之后的AsyncTask同时只能有1个任务在执行。为什么升级之后可以同时执行的任务数反而变少了呢？这是因为更新后的AsyncTask已变得更加灵活，如果不想使用默认的线程池，还可以自由地进行配置。比如使用如下的代码来启动任务：

~~~ Java
Executor exec = new ThreadPoolExecutor(15, 200, 10,  
        TimeUnit.SECONDS, new LinkedBlockingQueue<Runnable>());  
new DownloadTask().executeOnExecutor(exec); 
~~~
这样就可以使用我们自定义的一个Executor来执行任务，而不是使用SerialExecutor。上述代码的效果允许在同一时刻有15个任务正在执行，并且最多能够存储200个任务。


> 下面内容选自知乎里的一篇问答：[《安卓中AsyncTask和thread以及Service和IntentService的关系和区别？》](https://www.zhihu.com/question/51583930/answer/126856053)

### 什么是线程，同步和异步？

- 1、同步：在一个线程执行，先执行完了前面的代码，才会执行后面的代码，是阻塞的。
- 2、异步：开启一个新的线程执行，不会等前面的代码执行完，就会执行后面的代码，是非阻塞的。
- 3、什么是main（UI）线程：android启动的第一个线程。主要负责处理ui和事件的工作。

    【特别注意】
    - 1)、更新ui只能在ui线程进行，不可以在其他线程更新ui，否则会崩溃。
    **ActivityThread只是运行在UI线程，不等于UI线程。**
    - 2)、在ui线程不可以做耗时操作，比如网络请求等，如果做耗时操作，就会阻塞ui线程，就会导致界面卡顿。会出现ANR（application not response、应用无响应）。

- 4、异步通讯：那么我们要请求网络或者其他耗时操作的时候怎么办？这就涉及到异步通讯或者叫线程通讯。 
    先在子线程加载数据，做耗时操作，然后把取得的数据传递给ui线程，让ui线程来更新ui。
- 5、线程通讯的方式：

    - **1)、Handler**

    **在主线程中使用方法：**
        - (1)、在主线程实例化一个Hanlder，复写handleMessage()方法，在里面做更新ui的操作；
        - (2)、让子线程持有handler的引用，使用handler来发送消息。
            传递参数： 获取消息，请使用handler.obtainMessage();
            what:用来区分消息的类型
            obj：传递复杂参数 
            arg1:传递简单的int参数
            arg2:传递简单的int参数
        ![图片来自：http://gityuan.com/android/](/images/blogimages/2018/handler_thread_commun.jpg)

    **Handler的原理：**
        - (1)、在主线线程新建一个handler，在子线程中调用这个Handler发送消息到消息队列，在发送的时候，message.target会保存发送它的Handler；
        - (2)、主线程只带一个looper循环，会不断的从消息队列中取出消息，如果没有消息，就阻塞，这里的阻塞并不会导致主线程，因为这里的阻塞底层采用的是Linux的pipe与epoll机制；
        - (3)、在主线程中调用message.target（发送他的Handler）的DispatchMessage，间接调用handlerMeaage来处理消息。

    **在子线程中使用Handler：**
        因为使用Handler需要消息循环，子线程中没有消息循环，所以，这里有2中方法：
        - (1)、使用主线程的loop（getMainLooper()），然后在创建Handler的时候，把这个获取的主线程的loop传进去；但是，这种方法，子线程中的Handler还是在主线程处理消息。因为用的主线程的循环。
        - (2)、给子线程建立消息循环：
        先调用 Looper.prepare();建立消息循环，消息队列等。
        在建立了Handler之后，调用Looper.loop()，开始loop循环，如果不掉用，循环就不会开始运行，就不会处理消息。 

        代码如下：

        ~~~ Java
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.i(TAG, "run: ");
                //建立消息循环，建立消息队列
                Looper.prepare();
                han=new Handler(){
                    @Override
                    public void handleMessage(Message msg) {
                        Log.i(TAG, "handleMessage: "+Thread.currentThread().getName());
                    }
                };
                //开始loop循环
                Looper.loop();
            }
        }).start();
        ~~~

    **Handler的其他使用**           
        - (1)、hander.post()方法可以直接把一个代码post到主线程执行；

        ~~~ Java
            handler.post(new Runnable() {
                @Override
                public void run() {
                    //这里在主线程执行
                    }
                });
        ~~~
        - (2)、activity的runOnUITHread（）；

        ~~~ Java
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    //这里在主线程执行
                }
            });
        ~~~
        - (3)、任何控件的Post方法（）；

        ~~~ Java
            tv.post(new Runnable() {
                @Override
                public void run() {
                    //这里在主线程执行
                    }
            });
        ~~~
        - (4)、延时执行：

        ~~~ Java
            handler.postDelayed(new Runnable() {
                @Override
                public void run() {
                    //需要延时执行的代码
                    }
                },1000);
        ~~~

    - **2)、HandlerThread: 支持Handler的线程，内部实现了Loop循环**

        使用方法：

        ~~~ Java
            //新建一个代loop的子线程
            final HandlerThread handlerThread=new HandlerThread("zixiancheng");
            //让子线程开始执行
            handlerThread.start();
            //把线程的looper传给Hander运行，这样，这个Hander就在zixiancheng中执行了。
            handler=new Handler(handlerThread.getLooper())
        ~~~

        【 注意！！】
        使用了这个HandlerThread，他会开启一个循环一直跑，所以，在退出程序的时候，一定要调用quit()，退出循环。

        ~~~ Java
            @Override
            protected void onDestroy() {
                handlerThread.quit();
                super.onDestroy();
            }
        ~~~

    - **3)、AsyncTask:异步任务**
    继承自AsyncTask，写一个自己的异步任务，注意，AsyncTask有3个泛型。
        从做到又分别是：参数的类型，进度的类型，返回结果的类型。
        execute()：执行这个异步任务，必须在主线程中调用
        onPreExecute()：在执行异步任务之前调用
        doInBackground()：执行任务的函数，比如去请求网络
        publishProgress()：在请求的过程中，更新一下进度，会调用onProgressUpdate
        onProgressUpdate()：更新进度，在主线程中运行
        onPostExecute()：请求完了以后，可以保存结果
        cancel()：取消任务的时候调用。

以上，是关于线程通讯。

### Service介绍：

- 1、startService：        
    特点：            
    仅仅需要长时间运行,不需要和其数据交互
    使用stopService/stopself()停止服务
    多次调用，onCreate方法只执行一次,onStartCommand多次执行
    不与当前的Activity绑定，即使当前activity已经关闭了，服务也会继续运行 

    使用方法：
    继承Service写一个类，实现自己的业务需要（onCreate、onStartCommand）；
    然后在清单文件中申明；
    在activity里面通过startService来启动这个服务；
- 2、bindService： 
    特点：
    需要和其进行数据交互,使用Binder类            
    使用unbindService停止服务            
    当activity销毁之后对应绑定的service自动停止、销毁            
    不能跨进程通信绑定
    不能在广播中调用

    使用方式：
    继承Service写一个类，实现自己的业务需要（onCreate、onBind）；
    然后在清单文件中申明；
    在activity里面通过bindService来启动这个服务，这里还需要一个参数，就是serviceConnection对象，通过这个对象来处理activity和Service绑定的事件；

    如果要**Activity和Service相互通讯**，还需要使用Binder：
    继承自Binder来实现一个自己的Binder，在service的onBind里面返回一个自己的Binder实例；

    ~~~ Java
    public class MsgService extends Service {

        //.....

        /** 
         * 返回一个Binder对象 
         */
        @Override
        public IBinder onBind(Intent intent) {
            return new MsgBinder();
        }  

        public class MsgBinder extends Binder{  
            /** 
             * 获取当前Service的实例 
             * @return 
             */  
            public MsgService getService(){  
                return MsgService.this;  
            }
        }
    }
    ~~~

    在activity的ServiceConnection里面的onServiceConnected会把service里面的Binder的对象传递回来；我们可以在Binder里面，得到目标service的实例，这样，就可以和Service交互了。

    这里的Activity和Service相互通讯，有三种方式：
    1.得到Service引用后，直接调用MyService的具体方法。
    2.使用接口，在Service内定义接口类，在Activity内得到MyService引用后，注册接口监听。
    3.可以使用广播，当然Activity要注册相应的接收器。比如Service要向多个Activity发送同样的消息的话，用这种方法就更好

- 3、IntentService：

    特点：
    会自动开启一个子线程，子线程任务结束以后，会自动stop，不需要手动去stopService或者stopself()；
    如果多次启动，会在一个队列中，依次的运行;

    使用方法：
    继承IntentService，实现自己的服务，然后重写onHandleIntent(),在里面做耗时操作；
    intentService在启动的时候，需要一个默认的构造函数；

### Service实现IPC
Service可以结合Messenger实现IPC，当然也可以使用AIDL的方式实现进程间通信。
以Messenger为例，实现IPC分以下几个步骤：

- 1.service 内部需要有一个 Handler 的实现，它被用来处理从每一个 client 发送过的来请求
~~~ Java
    //MyService类内
    class ServiceHandler extends Handler {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {

                default:
                    super.handleMessage(msg);
            }
        }
    }
~~~
- 2.通过这个 Handler ，来生成一个 Messenger
~~~ Java
//MyService类内
final Messenger mMessenger = new Messenger(new ServiceHandler());
~~~
- 3.在 service 的onBind() 方法中，需要向 client 返回由该 Messenger 生成的一个 IBinder 实例
~~~ Java
    @Override
    public IBinder onBind(Intent intent) {
        //返回给客户端一个IBinder实例
        return mMessenger.getBinder();
    }
~~~
- 4.client 使用从 service 返回的 IBinder 实例来初始化一个 Messenger， 然后使用该 Messenger 与 service 进行通信
~~~ Java
    //Activity类内部
    private ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder service) {
            //接收onBind()传回来的IBinder，并用它构造Messenger
            mService = new Messenger(service);
            mBound = true;
        }

        public void onServiceDisconnected(ComponentName className) {
            mService = null;
            mBound = false;
        }
    };

~~~
- 5.service 通过它自身内部的 Handler 实现（ServiceHandler 的 handleMessage() 方法中）来处理从 client 发送过来的请求
~~~ Java
//发送消息
Message msg = Message.obtain();
//fill your info into msg
messenger.send(msg);
~~~

 


