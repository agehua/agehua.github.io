---
layout: post
title:  Activity启动流程分析
category: accumulation
tags: AMS
keywords: AOSP, AMS, Binder
banner: http://obxk8w81b.bkt.clouddn.com/Cottages%20Reminiscence%20of%20the%20North.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Cottages%20Reminiscence%20of%20the%20North.jpg
toc: true
---

## Activity启动流程分析

本文主要介绍Activity的启动过程和过程中主要涉及的类。如果你跟随上一篇文章，成功编译了Android源码，可以动手跟本篇文章一步一步调试分析整个启动过程。没有编译源码，也可以看看[android.googlesource](https://android.googlesource.com/platform/frameworks/base/+/master/)，这里也有源码。

### 主要对象功能介绍

#### zygote进程和SystemServer进程
android是基于Linux系统的，而在linux中，所有的进程都是由init进程直接或者是间接fork出来的，**zygote** 进程也不例外。

> 每一个App其实都是
- 一个单独的dalvik虚拟机
- 一个单独的进程

Android系统开启新进程的方式，是通过fork第一个**zygote** 进程实现的。所以说，除了第一个zygote进程，其他应用所在的进程都是zygote的子进程。

**SystemServer** 也是一个进程，而且是由zygote进程fork出来的。
为什么说SystemServer非常重要呢？因为系统里面重要的服务都是在这个进程里面开启的，比如ActivityManagerService、PackageManagerService、WindowManagerService等等。

> 我们的App和AMS(SystemServer进程)还有zygote进程分属于三个独立的进程，他们之间如何通信呢？
App与AMS通过Binder进行IPC通信，AMS(SystemServer进程)与zygote通过Socket进行IPC通信。


#### 与Activity启动有关的类
我们下面的文章将围绕着这几个类进行介绍。可能你第一次看的时候，印象不深，不过没关系，当你跟随者我读完这篇文章的时候，相信那时候你会对这些类更加的熟悉和深刻。

- **ActivityManagerServices**，简称AMS，服务端对象，负责系统中所有Activity的生命周期，查看源码，[点击这里](https://android.googlesource.com/platform/frameworks/base/services/java/com/android/server/am/ActivityManagerService.java)

- **ActivityThread**，App的真正入口。当开启App之后，会调用main()开始运行，开启消息循环队列，这就是传说中的UI线程或者叫主线程。与ActivityManagerServices配合，一起完成Activity的管理工作

- **ApplicationThread**，用来实现ActivityManagerService与ActivityThread之间的交互。在ActivityManagerService需要管理相关Application中的Activity的生命周期时，通过ApplicationThread的代理对象与ActivityThread通讯。

- **ApplicationThreadProxy**，是ApplicationThread在服务器端的代理，负责和客户端的ApplicationThread通讯。AMS就是通过该代理与ActivityThread进行通信的。

- **Instrumentation**，每一个应用程序只有一个Instrumentation对象，每个Activity内都有一个对该对象的引用。Instrumentation可以理解为应用进程的管家，ActivityThread要创建或暂停某个Activity时，都需要通过Instrumentation来进行具体的操作。

- **ActivityStack**，Activity在AMS的栈管理，用来记录已经启动的Activity的先后关系，状态信息等。通过ActivityStack决定是否需要启动新的进程。

- **ActivityRecord**，ActivityStack的管理对象，每个Activity在AMS对应一个ActivityRecord，来记录Activity的状态以及其他的管理信息。其实就是服务器端的Activity对象的映像。

- **TaskRecord**，AMS抽象出来的一个“任务”的概念，是记录ActivityRecord的栈，一个“Task”包含若干个ActivityRecord。AMS用TaskRecord确保Activity启动和退出的顺序。如果你清楚Activity的4种launchMode，那么对这个概念应该不陌生。

#### App程序的入口
我们一般在启动Activity的时候都是使用系统提供的方法Activity.startActivity()操作的，本文就在此方法上分析整个过程:

> 这里我的app进程是：foo.bar.multi，后面深入源码后会在系统进程system_process和app进程之间切换。

~~~ Java
@Override
public void startActivity(Intent intent) {
    this.startActivity(intent, null);
}

@Override
public void startActivity(Intent intent, @Nullable Bundle options) {
    if (options != null) { //options这里为null
        startActivityForResult(intent, -1, options);
    } else {
        // Note we want to go through this call for compatibility with
        // applications that may have overridden the method.
        startActivityForResult(intent, -1);
    }
}

public void startActivityForResult(@RequiresPermission Intent intent, int requestCode,
        @Nullable Bundle options) {
    if (mParent == null) { //这里mParent不为空
        options = transferSpringboardActivityOptions(options);
        Instrumentation.ActivityResult ar =
            mInstrumentation.execStartActivity(
                this, mMainThread.getApplicationThread(), mToken, this,
                intent, requestCode, options);
        if (ar != null) {
            mMainThread.sendActivityResult(
                mToken, mEmbeddedID, requestCode, ar.getResultCode(),
                ar.getResultData());
        }

        // 代码省略....
    } else {
        // 代码省略....
    }
}
~~~
上面代码可以看到，这里用到了mMainThread变量去执行的操作，再看看这个变量，它是ActivityThread的一个实例。ActivityThread是一个应用非常关键的一个类，首先它是一个应用的主线程，其次就是他才是一个程序的入口（main方法）的地方：

~~~ Java
public static void main(String[] args) {
    //...代码省略
}
~~~
没有Android源码的，可以看这里：[android.googlesource](https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/app/ActivityThread.java)。后面介绍这个入口main方法什么时候执行。

上面的mInstrumentation对象，是在Activity的attach()方法中被赋值的

#### Instrumentation是什么？和ActivityThread是什么关系？

接着说上面的mInstrumentation.execStartActivity()方法，有三个同名的重载方法：
~~~ Java
public ActivityResult execStartActivity(
          Context who, IBinder contextThread, IBinder token, Activity target,
          Intent intent, int requestCode, Bundle options)


public ActivityResult execStartActivity(
         Context who, IBinder contextThread, IBinder token, String target,
         Intent intent, int requestCode, Bundle options)

public ActivityResult execStartActivity(
         Context who, IBinder contextThread, IBinder token, Activity target,
         Intent intent, int requestCode, Bundle options, UserHandle user)
~~~
考验眼力的时候到了，仔细看下，上面的mInstrumentation.execStartActivity()调用的其实是第一个重载方法：
~~~ Java
public ActivityResult execStartActivity(Context who, IBinder contextThread, IBinder token, Activity target,
        Intent intent, int requestCode, Bundle options) {
    IApplicationThread whoThread = (IApplicationThread) contextThread;
        //...ignore some code...
  try {
        intent.migrateExtraStreamToClipData();
        intent.prepareToLeaveProcess();
        int result = ActivityManagerNative.getDefault()
            .startActivity(whoThread, who.getBasePackageName(), intent,
                    intent.resolveTypeIfNeeded(who.getContentResolver()),
                    token, target != null ? target.mEmbeddedID : null,
                    requestCode, 0, null, options);
        checkStartActivityResult(result, intent);
    } catch (RemoteException e) {
    }
    return null;
}
~~~
可以看到execStartActivity()方法，调用了ActivityManagerNative的startActivity()方法，看到这个ActivityManagerNative类有没有感觉和之前一篇文章，[Android Binder机制分析（二）](https://agehua.github.io/2017/07/10/android-binder-principle2/)里的ServiceManagerNative有点类似，这里涉及到的是系统服务间的Binder机制，不了解的同学可以看看这篇文章。

稍后分析ActivityManagerNative，这里简单介绍下Instrumentation类。

Instrumentation意为“仪器”。每个Activity都持有Instrumentation对象的一个引用，但是整个进程只会存在一个Instrumentation对象。

Instrumentation这个类里面的方法大多数和Application和Activity有关，可以说是对Application和Activity初始化和生命周期的工具类。

举个例子，callActivityOnCreate()
~~~ Java
public void callActivityOnCreate(Activity activity, Bundle icicle) {
    prePerformCreate(activity);
    activity.performCreate(icicle);
    postPerformCreate(activity);
}
~~~
对activity.performCreate(icicle);这一行代码熟悉吗？这一行里面就调用了传说中的Activity的入口函数onCreate()，不信？接着往下看Activity.performCreate()
~~~ Java
final void performCreate(Bundle icicle) {
    onCreate(icicle);
    mActivityTransitionState.readState(icicle);
    performCreateCommon();
}
~~~
确实，onCreate在这里调用了。但是有一件事情必须说清楚，那就是这个Instrumentation类这么重要，为啥我在开发的过程中，没有发现他的踪迹呢？

是的，Instrumentation这个类很重要，对Activity生命周期方法的调用根本就离不开他，但他只负责Activity的内部管理。
外部想调整Activity的状态，必须通过ActivityThread。

ActivityThread你都没听说过？那你肯定听说过传说中的UI线程吧？是的，这就是UI线程。我们前面说过，App和AMS是通过Binder传递信息的，那么ActivityThread就是专门与AMS的外交工作的。Instrumentation相当于老板娘，处理家务，很少露面。ActivityThread是老板，处理对外事务。

所以说，AMS是董事会，负责指挥和调度的，ActivityThread是老板，虽然说家里的事自己说了算，但是需要听从AMS的指挥，而Instrumentation则是老板娘，负责家里的大事小事，但是一般不抛头露面，听一家之主ActivityThread的安排。

#### 简介AMS和ActivityThread之间的Binder通信
前面的两篇文章：[Android Binder机制分析（一）](https://agehua.github.io/2017/07/08/android-binder-principle/)和[Android Binder机制分析（二）](https://agehua.github.io/2017/07/10/android-binder-principle2/)。已经介绍了Android都是通过Binder机制调用远程的系统服务。这里在介绍下ActivityThread（App进程）是如何调用AMS服务的。

前面我们说到，在调用Activity.startActivity()的时候，实际上调用的是：
**mInstrumentation.execStartActivity()**
这个方法里面调用的是：
~~~ Java
ActivityManagerNative.getDefault().startActivity()
~~~
> 用鼠标定位一下，上面的startActivity()方法，会发现这个方法是IActivityManager接口里的方法。注意IActivityManager继承了IInterface接口，而这个接口就是AIDL接口类必须实现的接口。

再看，ActivityManagerNative.getDefault()返回的就是ActivityManagerService的远程接口的本地代理，即ActivityManagerProxy。
~~~ Java
public abstract class ActivityManagerNative extends Binder implements IActivityManager
{

 //从类声明上，我们可以看到ActivityManagerNative是Binder的一个子类，而且实现了IActivityManager接口
 static public IActivityManager getDefault() {
        return gDefault.get();
 }

//最终返回的还是一个ActivityManagerProxy对象
static public IActivityManager asInterface(IBinder obj) {
        if (obj == null) {
            return null;
        }
        IActivityManager in =
            (IActivityManager)obj.queryLocalInterface(descriptor);
        if (in != null) {
            return in;
        }

        //这里面的Binder类型的obj参数会作为ActivityManagerProxy的成员变量保存为mRemote成员变量，负责进行IPC通信
        return new ActivityManagerProxy(obj);
    }
}
~~~

其实ActivityManagerNative类就是远端服务的中间者Stub类，其实名字不叫Stub罢了，以后其实只要看到是继承了Binder类和实现了AIDL接口类型的就是Stub类，不要关心类名了。看到他的asInterface方法的时候也是和之前的其他系统服务都是类似的。
~~~ Java
//通过单例模式获取一个IActivityManager对象，这个对象通过asInterface(b)获得
private static final Singleton<IActivityManager> gDefault = new Singleton<IActivityManager>() {
       protected IActivityManager create() {
           IBinder b = ServiceManager.getService("activity");
           if (false) {
               Log.v("ActivityManager", "default service binder = " + b);
           }
           IActivityManager am = asInterface(b);
           if (false) {
               Log.v("ActivityManager", "default service = " + am);
           }
           return am;
       }
   };
}
~~~
在这里可以看到了，其实gDefalut借助Singleton实现的单例模式，而在内部可以看到先从ServiceManager中获取到AMS远端服务的Binder对象，然后使用asInterface方法转化成本地化对象（其实就是ActivityManagerProxy对象，这个对象是ActivityManagerNative的内部类）。然后我们在看看上面调用了startActivity方法，其实就是调用了ActivityManagerProxy对象的这个方法。

在这里面做的事情就是IPC通信，利用Binder对象，调用transact()，把所有需要的参数封装成Parcel对象，向AMS发送数据进行通信。

> 这个方法中添加一个断点，注意这里startActivity()是ActivityManagerProxy类的方法，这里的进程应该还是App进程：foo.bar.multi

~~~ Java
public int startActivity(IApplicationThread caller, String callingPackage, Intent intent,
        String resolvedType, IBinder resultTo, String resultWho, int requestCode,
        int startFlags, ProfilerInfo profilerInfo, Bundle options) throws RemoteException {
    Parcel data = Parcel.obtain();
    Parcel reply = Parcel.obtain();
    data.writeInterfaceToken(IActivityManager.descriptor);
    data.writeStrongBinder(caller != null ? caller.asBinder() : null);
    data.writeString(callingPackage);
    intent.writeToParcel(data, 0);
    //...省略部分代码
    mRemote.transact(START_ACTIVITY_TRANSACTION, data, reply, 0);
    reply.readException();
    int result = reply.readInt();
    reply.recycle();
    data.recycle();
    return result;
}
~~~
      注意，这里先别放开断点

> Binder本质上只是一种底层通信方式，和具体服务没有关系。为了提供具体服务，Server必须提供一套接口函数以便Client通过远程访问使用各种服务。这时通常采用Proxy设计模式：将接口函数定义在一个抽象类中，Server和Client都会以该抽象类为基类实现所有接口函数，所不同的是Server端是真正的功能实现，而Client端是对这些函数远程调用请求的包装。

客户端：ActivityManagerProxy =====>Binder驱动=====> ActivityManagerService：服务器

Binder只能传递数据，并不知道是要调用ActivityManagerServices的哪个方法，所以在数据中会添加方法的唯一标识码，比如前面的startActivity()方法中的标识：START_ACTIVITY_TRANSACTION。

在远端服务调用了transact()方法后，即mRemote.transact()，会转接到远端服务中间者ActivityManagerNative的onTransact方法中，找到对应的标识码：
>  注意这里的接收者ActivityManagerNative的onTransact方法是在system_process进程中，想要继续debug调试的的话，需要在monitor中选中这个进程，然后再在下面的方法中添加断点。最后放开上面添加的断点。

~~~ Java
@Override
public boolean onTransact(int code, Parcel data, Parcel reply, int flags)
        throws RemoteException {
    switch (code) {
    case START_ACTIVITY_TRANSACTION:
    {   //最好在case方法里面添加断点，避免受其他系统发送的消息影响
        data.enforceInterface(IActivityManager.descriptor);
        IBinder b = data.readStrongBinder();
        IApplicationThread app = ApplicationThreadNative.asInterface(b);
        String callingPackage = data.readString();
        Intent intent = Intent.CREATOR.createFromParcel(data);
        String resolvedType = data.readString();
        IBinder resultTo = data.readStrongBinder();
        String resultWho = data.readString();
        int requestCode = data.readInt();
        int startFlags = data.readInt();
        ProfilerInfo profilerInfo = data.readInt() != 0
                ? ProfilerInfo.CREATOR.createFromParcel(data) : null;
        Bundle options = data.readInt() != 0
                ? Bundle.CREATOR.createFromParcel(data) : null;
        int result = startActivity(app, callingPackage, intent, resolvedType,
                resultTo, resultWho, requestCode, startFlags, profilerInfo, options);
        reply.writeNoException();
        reply.writeInt(result);
        return true;
    }
    //...省略
}
~~~
这里的startActivity方法，是在IActivityManager接口中定义的，跟ActivityManagerProxy没有关系了，是由远端服务来实现的，这里我们可以猜想应该是叫做ActivityManagerService中，这个类的源代码可以在[android.googlesource](https://android.googlesource.com/platform/frameworks/base/+/master/services/core/java/com/android/server/am/ActivityManagerService.java)里找到：

这个类里有三个重载的startActivity()方法，不要晕，仔细看，对应的应该是第三个方法：
~~~ Java
final int startActivity(Intent intent, ActivityStackSupervisor.ActivityContainer container)

@Override
public int startActivity(IBinder whoThread, String callingPackage,
        Intent intent, String resolvedType, Bundle bOptions)

@Override
public final int startActivity(IApplicationThread caller, String callingPackage,
        Intent intent, String resolvedType, IBinder resultTo, String resultWho, int requestCode,
        int startFlags, ProfilerInfo profilerInfo, Bundle bOptions)
~~~

这个方法直接调用了startActivityAsUser()方法
~~~ Java
@Override
public final int startActivityAsUser(IApplicationThread caller, String callingPackage,
        Intent intent, String resolvedType, IBinder resultTo, String resultWho, int requestCode,
        int startFlags, ProfilerInfo profilerInfo, Bundle bOptions, int userId) {
    enforceNotIsolatedCaller("startActivity");
    userId = mUserController.handleIncomingUser(Binder.getCallingPid(), Binder.getCallingUid(),
            userId, false, ALLOW_FULL_ONLY, "startActivity", null);
    // TODO: Switch to user app stacks here.
    return mActivityStarter.startActivityMayWait(caller, -1, callingPackage, intent,
            resolvedType, null, null, resultTo, resultWho, requestCode, startFlags,
            profilerInfo, null, null, bOptions, false, userId, null, null);
}
~~~

这里的mActivityStarter是在ActivityManagerService创建的时候初始化的。
~~~ Java
public ActivityManagerService(Context systemContext) {
    //...省略代码
    mStackSupervisor = new ActivityStackSupervisor(this);
    mActivityStarter = new ActivityStarter(this, mStackSupervisor);
    //...省略代码
}
~~~
下面就是ActiviStarter、ActivityStack和ActivityStackSupervisor三个类之间方法调来调去了，这里为了节省篇幅就不再贴代码了，有毅力的同学可以每个方法都打上一个断点，走一遍：

ActivityStarter.startActivityMayWait()-> ActivityStarter.startActivityLocked()-> ActivityStarter.startActivityUnchecked()-> ActivityStackSupervisor.resumeFocusedStackTopActivityLocked()

再往后都是ActivityStackSupervisor类的方法，调用：

从resumeFocusedStackTopActivityLocked()-> resumeFocusedStackTopActivityLocked()-> resumeTopActivityUncheckedLocked()
-> resumeTopActivityInnerLocked()-> startSpecificActivityLocked()-> realStartActivityLocked()


#### IApplicationThread接口简介

重点在最后的方法里realStartActivityLocked()调用了，**app.thread.scheduleLaunchActivity()**方法
~~~ Java
final boolean realStartActivityLocked(ActivityRecord r, ProcessRecord app,
            boolean andResume, boolean checkConfig) throws RemoteException {

    //...代码省略

    final ActivityStack stack = task.stack;
    try {
        //...代码省略

        app.forceProcessStateUpTo(mService.mTopProcessState);
        app.thread.scheduleLaunchActivity(new Intent(r.intent), r.appToken,
                System.identityHashCode(r), r.info, new Configuration(mService.mConfiguration),
                new Configuration(task.mOverrideConfig), r.compat, r.launchedFromPackage,
                task.voiceInteractor, app.repProcState, r.icicle, r.persistentState, results,
                newIntents, !andResume, mService.isNextTransitionForward(), profilerInfo);

        //...代码省略
    } catch (RemoteException e) {

    }

    return true;
}
~~~
scheduleLaunchActivity()方法是IApplicationThread接口里面的方法，但是是由那个类实现的呢，这里就需要猜了，哈哈

下面几个类都实现了相同的接口IApplicationThread：
~~~ Java
private class ApplicationThread extends ApplicationThreadNative {}

public abstract class ApplicationThreadNative extends Binder implements IApplicationThread{}

class ApplicationThreadProxy implements IApplicationThread {}

public interface IApplicationThread extends IInterface {}
~~~
IApplicationThread的源码在这里：[android.googlesource](https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/app/IApplicationThread.java)

> IApplicationThread是实现了IInterface接口的，说明也是用的Binder远程通讯，这里的当前进程是system_process，
本地代理类应该是ApplicationThreadProxy类，而这个类是内部类，在ApplicationThreadNative类中

来看ApplicationThreadProxy类的scheduleLaunchActivity()方法：
~~~ Java
public final void scheduleLaunchActivity(Intent intent, IBinder token, int ident,
        ActivityInfo info, Configuration curConfig, Configuration overrideConfig,
        CompatibilityInfo compatInfo, String referrer, IVoiceInteractor voiceInteractor,
        int procState, Bundle state, PersistableBundle persistentState,
        List<ResultInfo> pendingResults, List<ReferrerIntent> pendingNewIntents,
        boolean notResumed, boolean isForward, ProfilerInfo profilerInfo) throws RemoteException {
    Parcel data = Parcel.obtain();
    data.writeInterfaceToken(IApplicationThread.descriptor);

    //....写入数据

    mRemote.transact(SCHEDULE_LAUNCH_ACTIVITY_TRANSACTION, data, null,
            IBinder.FLAG_ONEWAY);
    data.recycle();
}
~~~
记住这个标识，SCHEDULE_LAUNCH_ACTIVITY_TRANSACTION，在ApplicationThreadNative类的onTransact()方法里找：

> 这里注意，调用完mRemote.transact()方法，回调的onTransact()方法已经不再system_process进程中了，而是到了App进程：foo.bar.multi

~~~ Java
//onTransact()方法里：
case SCHEDULE_LAUNCH_ACTIVITY_TRANSACTION:
    {
        data.enforceInterface(IApplicationThread.descriptor);
        // 取出数据
        scheduleLaunchActivity(intent, b, ident, info, curConfig, overrideConfig, compatInfo,
                referrer, voiceInteractor, procState, state, persistentState, ri, pi,
                notResumed, isForward, profilerInfo);
        return true;
    }
~~~
取出各种数据，在调用scheduleLaunchActivity()方法。

> 这里的cheduleLaunchActivity()在哪里实现？感觉应该在ApplicationThreadService类里。但是并没有这个类。其实应该是ApplicationThread类，不叫XXXService了，这个类实现了ApplicationThreadNative接口，同时它是ActivityThread的内部类

到这里总结下IApplicationThread接口的Binder机制：

客户端：ApplicationThread <=====Binder驱动<===== ApplicationThreadProxy：服务器
对比之前的IActivityManager：
客户端：ActivityManagerProxy =====>Binder驱动=====> ActivityManagerService：服务器
有没有发现**Binder只能单向传递**。

再来看ApplicationThread类的scheduleLaunchActivity()方法
~~~ Java
// we use token to identify this activity without having to send the
// activity itself back to the activity manager. (matters more with ipc)
@Override
public final void scheduleLaunchActivity(Intent intent, IBinder token, int ident,
        ActivityInfo info, Configuration curConfig, Configuration overrideConfig,
        CompatibilityInfo compatInfo, String referrer, IVoiceInteractor voiceInteractor,
        int procState, Bundle state, PersistableBundle persistentState,
        List<ResultInfo> pendingResults, List<ReferrerIntent> pendingNewIntents,
        boolean notResumed, boolean isForward, ProfilerInfo profilerInfo) {

    updateProcessState(procState, false);
    ActivityClientRecord r = new ActivityClientRecord();

    r.token = token;
    r.ident = ident;
    r.intent = intent;
    r.referrer = referrer;
    r.voiceInteractor = voiceInteractor;
    r.activityInfo = info;
    r.compatInfo = compatInfo;
    r.state = state;
    r.persistentState = persistentState;

    r.pendingResults = pendingResults;
    r.pendingIntents = pendingNewIntents;

    r.startsNotResumed = notResumed;
    r.isForward = isForward;

    r.profilerInfo = profilerInfo;

    r.overrideConfig = overrideConfig;
    updatePendingConfiguration(curConfig);

    sendMessage(H.LAUNCH_ACTIVITY, r);
}
~~~


在继续看接收消息的地方，在H类里（H类继承了Handler），
~~~ Java
public void handleMessage(Message msg) {
    if (DEBUG_MESSAGES) Slog.v(TAG, ">>> handling: " + codeToString(msg.what));
    switch (msg.what) {
        case LAUNCH_ACTIVITY: {
            Trace.traceBegin(Trace.TRACE_TAG_ACTIVITY_MANAGER, "activityStart");
            final ActivityClientRecord r = (ActivityClientRecord) msg.obj;

            r.packageInfo = getPackageInfoNoCheck(
                  r.activityInfo.applicationInfo, r.compatInfo);
            handleLaunchActivity(r, null, "LAUNCH_ACTIVITY");
            Trace.traceEnd(Trace.TRACE_TAG_ACTIVITY_MANAGER);
        } break;

    //....
~~~
handleLaunchActivity()->performLaunchActivity()。

performLaunchActivity()通过ClassLoader导入相应的Activity类，然后把它启动起来，注意看代码里的注释。
~~~ Java
    Activity activity = null;
    try {
        //通过ClassLoader将foo.bar.multi.XXXActivity类加载进来：
        java.lang.ClassLoader cl = r.packageInfo.getClassLoader();
        activity = mInstrumentation.newActivity(
                cl, component.getClassName(), r.intent);
        StrictMode.incrementExpectedActivityCount(activity.getClass());
        r.intent.setExtrasClassLoader(cl);
        r.intent.prepareToEnterProcess();
        if (r.state != null) {
            r.state.setClassLoader(cl);
        }
    } catch (Exception e) {
        if (!mInstrumentation.onException(activity, e)) {
            throw new RuntimeException(
                "Unable to instantiate activity " + component
                + ": " + e.toString(), e);


    try {
        //创建Application对象，这是根据AndroidManifest.xml配置文件中的Application标签的信息来创建的
        Application app = r.packageInfo.makeApplication(false, mInstrumentation);

        //代码主要创建Activity的上下文信息，并通过attach方法将这些上下文信息设置到XXXActivity中去：
        if (activity != null) {
          activity.attach(appContext, this, getInstrumentation(), r.token,
          r.ident, app, r.intent, r.activityInfo, title, r.parent,
          r.embeddedID, r.lastNonConfigurationInstances, config,
          r.referrer, r.voiceInteractor, window);

        //调用activity的onCreate函数，上面提到过这个方法：
        mInstrumentation.callActivityOnCreate(activity, r.state);

~~~


无论是通过点击应用程序图标来启动Activity，还是通过Activity内部调用startActivity接口来启动新的Activity，都要借助于应用程序框架层的ActivityManagerService服务进程。在Android应用程序框架层中，ActivityManagerService是一个非常重要的接口，它不但负责启动Activity和Service，还负责管理Activity和Service。

Android应用程序框架层中的ActivityManagerService启动Activity的过程大致如下图所示：

![图片来自：http://blog.csdn.net/luoshengyang/article/details/6685853](images/blogimages/2017/activity-start-process.png)
在这个图中，ActivityManagerService和ActivityStack位于同一个进程中，而ApplicationThread和ActivityThread位于另一个进程中。其中，ActivityManagerService是负责管理Activity的生命周期的，ActivityManagerService还借助ActivityStack是来把所有的Activity按照后进先出的顺序放在一个堆栈中；对于每一个应用程序来说，都有一个ActivityThread来表示应用程序的主进程，而每一个ActivityThread都包含有一个ApplicationThread实例，它是一个Binder对象，负责和其它进程进行通信。

下面简要总结一下启动的过程：

- Step 1. 无论是通过Launcher来启动Activity，还是通过Activity内部调用startActivity接口来启动新的Activity，都通过Binder进程间通信进入到ActivityManagerService进程中，并且调用ActivityManagerService.startActivity接口；
- Step 2. ActivityManagerService调用ActivityStack.startActivityMayWait来做准备要启动的Activity的相关信息；
- Step 3. ActivityStack通知ApplicationThread要进行Activity启动调度了，这里的ApplicationThread代表的是调用ActivityManagerService.startActivity接口的进程，对于通过点击应用程序图标的情景来说，这个进程就是Launcher了，而对于通过在Activity内部调用startActivity的情景来说，这个进程就是这个Activity所在的进程了；
- Step 4. ApplicationThread不执行真正的启动操作，它通过调用ActivityManagerService.activityPaused接口进入到ActivityManagerService进程中，看看是否需要创建新的进程来启动Activity；
- Step 5. 对于通过点击应用程序图标来启动Activity的情景来说，ActivityManagerService在这一步中，会调用startProcessLocked来创建一个新的进程，而对于通过在Activity内部调用startActivity来启动新的Activity来说，这一步是不需要执行的，因为新的Activity就在原来的Activity所在的进程中进行启动；
- Step 6. ActivityManagerServic调用ApplicationThread.scheduleLaunchActivity接口，通知相应的进程执行启动Activity的操作；
- Step 7. ApplicationThread把这个启动Activity的操作转发给ActivityThread，ActivityThread通过ClassLoader导入相应的Activity类，然后把它启动起来。


### 参考资料
[Android源码分析-Activity的启动过程](http://blog.csdn.net/singwhatiwanna/article/details/18154335)

罗老师的，[Android应用程序的Activity启动过程简要介绍和学习计划](http://blog.csdn.net/luoshengyang/article/details/6685853)和 [Android应用程序启动过程源代码分析](http://blog.csdn.net/luoshengyang/article/details/6689748)

[【凯子哥带你学Framework】Activity启动过程全解析](http://blog.csdn.net/zhaokaiqiang1992/article/details/49428287)

[Android系统篇之—-解读AMS远端服务调用机制以及Activity的启动流程](
http://www.wjdiankong.cn/android%E7%B3%BB%E7%BB%9F%E7%AF%87%E4%B9%8B-%E8%A7%A3%E8%AF%BBams%E8%BF%9C%E7%AB%AF%E6%9C%8D%E5%8A%A1%E8%B0%83%E7%94%A8%E6%9C%BA%E5%88%B6%E4%BB%A5%E5%8F%8Aactivity%E7%9A%84%E5%90%AF%E5%8A%A8/)
