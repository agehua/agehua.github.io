---
layout: post
title:  Activity启动流程分析
category: accumulation
tags: TimerTask
keywords: handler, TimerTask
banner: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Almond%20Branch%20in%20a%20Glass%20with%20a%20Book.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Almond%20Branch%20in%20a%20Glass%20with%20a%20Book.jpg
toc: true
---


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
我们下面的文章将围绕着这几个类进行介绍。可能你第一次看的时候，印象不深，不过没关系，当你跟随者我读完这篇文章的时候，我会在最后再次列出这些对象的功能，相信那时候你会对这些类更加的熟悉和深刻。

- **ActivityManagerServices**，简称AMS，服务端对象，负责系统中所有Activity的生命周期，查看源码，[点击这里](frameworks/base/services/java/com/android/server/am/ActivityManagerService.java)

- **ActivityThread**，App的真正入口。当开启App之后，会调用main()开始运行，开启消息循环队列，这就是传说中的UI线程或者叫主线程。与ActivityManagerServices配合，一起完成Activity的管理工作

- **ApplicationThread**，用来实现ActivityManagerService与ActivityThread之间的交互。在ActivityManagerService需要管理相关Application中的Activity的生命周期时，通过ApplicationThread的代理对象与ActivityThread通讯。

- **ApplicationThreadProxy**，是ApplicationThread在服务器端的代理，负责和客户端的ApplicationThread通讯。AMS就是通过该代理与ActivityThread进行通信的。

- **Instrumentation**，每一个应用程序只有一个Instrumentation对象，每个Activity内都有一个对该对象的引用。Instrumentation可以理解为应用进程的管家，ActivityThread要创建或暂停某个Activity时，都需要通过Instrumentation来进行具体的操作。

- **ActivityStack**，Activity在AMS的栈管理，用来记录已经启动的Activity的先后关系，状态信息等。通过ActivityStack决定是否需要启动新的进程。

- **ActivityRecord**，ActivityStack的管理对象，每个Activity在AMS对应一个ActivityRecord，来记录Activity的状态以及其他的管理信息。其实就是服务器端的Activity对象的映像。

- **TaskRecord**，AMS抽象出来的一个“任务”的概念，是记录ActivityRecord的栈，一个“Task”包含若干个ActivityRecord。AMS用TaskRecord确保Activity启动和退出的顺序。如果你清楚Activity的4种launchMode，那么对这个概念应该不陌生。

#### App程序的入口
我们一般在启动Activity的时候都是使用系统提供的方法Context.startActivity()操作的，关于这个方法的实现代码是在ContextImpl.java中：
~~~ Java
@Override
public void startActivity(Intent intent) {
    warnIfCallingFromSystemProcess();
    startActivity(intent, null);
}

/** @hide */
@Override
public void startActivityAsUser(Intent intent, UserHandle user) {
    startActivityAsUser(intent, null, user);
}

@Override
public void startActivity(Intent intent, Bundle options) {
    warnIfCallingFromSystemProcess();

    // Calling start activity from outside an activity without FLAG_ACTIVITY_NEW_TASK is
    // generally not allowed, except if the caller specifies the task id the activity should
    // be launched in.
    if ((intent.getFlags()&Intent.FLAG_ACTIVITY_NEW_TASK) == 0
            && options != null && ActivityOptions.fromBundle(options).getLaunchTaskId() == -1) {
        throw new AndroidRuntimeException(
                "Calling startActivity() from outside of an Activity "
                + " context requires the FLAG_ACTIVITY_NEW_TASK flag."
                + " Is this really what you want?");
    }
    mMainThread.getInstrumentation().execStartActivity(
            getOuterContext(), mMainThread.getApplicationThread(), null,
            (Activity) null, intent, -1, options);
}
~~~
这里用到了mMainThread变量去执行的操作，再看看这个变量，它是ActivityThread的一个实例。ActivityThread是一个应用非常关键的一个类，首先它是一个应用的主线程，其次就是他才是一个程序的入口的地方：

~~~ Java
public static void main(String[] args) {
    //...代码省略
}
~~~
完整源码可以看这个：[android.googlesource](https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/app/ActivityThread.java)。后面介绍这个入口main方法什么时候执行。

mMainThread.getInstrumentation()返回的是一个Instrumentation对象
~~~ Java
public Instrumentation getInstrumentation()
{
    return mInstrumentation;
}
~~~

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
可以看到execStartActivity()方法，调用了ActivityManagerNative的startActivity()方法，看到这个ActivityManagerNative类有没有感觉和上一篇文章，[Android Binder机制分析（二）](https://agehua.github.io/2017/07/10/android-binder-principle2/)里的ServiceManagerNative有点类似。

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

前面我们说到，在调用ContextImpl.tartActivity()的时候，实际上调用的是：
**mInstrumentation.execStartActivity()**
这个方法里面调用的是：
~~~ Java
ActivityManagerNative.getDefault().startActivity()
~~~
> 用鼠标定位一下，上面的startActivity()方法，会发现这个方法是IActivityManager接口里的方法。注意IActivityManager继承了IInterface接口，而这个接口就是AIDL接口类必须实现的接口。

再看，ActivityManagerNative.getDefault返回的就是ActivityManagerService的远程接口的本地代理，即ActivityManagerProxy。
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

这个方法，在这里面做的事情就是IPC通信，利用Binder对象，调用transact()，把所有需要的参数封装成Parcel对象，向AMS发送数据进行通信。
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
> Binder本质上只是一种底层通信方式，和具体服务没有关系。为了提供具体服务，Server必须提供一套接口函数以便Client通过远程访问使用各种服务。这时通常采用Proxy设计模式：将接口函数定义在一个抽象类中，Server和Client都会以该抽象类为基类实现所有接口函数，所不同的是Server端是真正的功能实现，而Client端是对这些函数远程调用请求的包装。

客户端：ActivityManagerProxy =====>Binder驱动=====> ActivityManagerService：服务器

Binder只能传递数据，并不知道是要调用ActivityManagerServices的哪个方法，所以在数据中会添加方法的唯一标识码，比如前面的startActivity()方法中的标识：START_ACTIVITY_TRANSACTION。

在远端服务调用了transact()方法后，即mRemote.transact()，会转接到远端服务中间者ActivityManagerNative的onTransact方法中，找到对应的标识码：
~~~ Java
@Override
public boolean onTransact(int code, Parcel data, Parcel reply, int flags)
        throws RemoteException {
    switch (code) {
    case START_ACTIVITY_TRANSACTION:
    {
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
public final int startActivity(IApplicationThread caller, String callingPackage,
        Intent intent, String resolvedType, IBinder resultTo, String resultWho, int requestCode,
        int startFlags, ProfilerInfo profilerInfo, Bundle bOptions) {
    return startActivityAsUser(caller, callingPackage, intent, resolvedType, resultTo,
            resultWho, requestCode, startFlags, profilerInfo, bOptions,
            UserHandle.getCallingUserId());
}

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

~~~ Java
public class ActivityStarter {  

  final int startActivityMayWait(IApplicationThread caller, int callingUid,
              String callingPackage, Intent intent, String resolvedType,
              IVoiceInteractionSession voiceSession, IVoiceInteractor voiceInteractor,
              IBinder resultTo, String resultWho, int requestCode, int startFlags,
              ProfilerInfo profilerInfo, IActivityManager.WaitResult outResult, Configuration config,
              Bundle bOptions, boolean ignoreTargetSecurity, int userId,
              IActivityContainer iContainer, TaskRecord inTask) {
          // Refuse possible leaked file descriptors
          if (intent != null && intent.hasFileDescriptors()) {
              throw new IllegalArgumentException("File descriptors passed in Intent");
          }
          mSupervisor.mActivityMetricsLogger.notifyActivityLaunching();
          //.....

          ResolveInfo rInfo = mSupervisor.resolveIntent(intent, resolvedType, userId);
          if (rInfo == null) {

              //...代码省略，不走这里
          }
          // Collect information about the target of the Intent.
          ActivityInfo aInfo = mSupervisor.resolveActivity(intent, rInfo, startFlags, profilerInfo);
          ActivityOptions options = ActivityOptions.fromBundle(bOptions);
          ActivityStackSupervisor.ActivityContainer container =
                  (ActivityStackSupervisor.ActivityContainer)iContainer;
          synchronized (mService) {
              //.....
              if (DEBUG_CONFIGURATION) Slog.v(TAG_CONFIGURATION,
                      "Starting activity when config will change = " + stack.mConfigWillChange);
              final long origId = Binder.clearCallingIdentity();
              if (aInfo != null &&
                      (aInfo.applicationInfo.privateFlags
                              & ApplicationInfo.PRIVATE_FLAG_CANT_SAVE_STATE) != 0) {
                  // This may be a heavy-weight process!  Check to see if we already
                  // have another, different heavy-weight process running.
                  if (aInfo.processName.equals(aInfo.applicationInfo.packageName)) {
                      //代码省略.....
                  }
              }
              final ActivityRecord[] outRecord = new ActivityRecord[1];
              int res = startActivityLocked(caller, intent, ephemeralIntent, resolvedType,
                      aInfo, rInfo, voiceSession, voiceInteractor,
                      resultTo, resultWho, requestCode, callingPid,
                      callingUid, callingPackage, realCallingPid, realCallingUid, startFlags,
                      options, ignoreTargetSecurity, componentSpecified, outRecord, container,
                      inTask);

              // .....

              if (outResult != null) {
                //这里传过来的outResult为null
              }
              return res;
          }
      }  
}
~~~
startActivityLocked()方法还是ActivityStarter类的方法，它又调用了ActivityStack.startActivityUncheckedLocked()方法。

~~~ Java
final int startActivityUncheckedLocked(ActivityRecord r,  
    ActivityRecord sourceRecord, Uri[] grantedUriPermissions,  
    int grantedMode, boolean onlyIfNeeded, boolean doResume) {  

    //....
    boolean addingToTask = false;  
    if (((launchFlags&Intent.FLAG_ACTIVITY_NEW_TASK) != 0 &&  
        (launchFlags&Intent.FLAG_ACTIVITY_MULTIPLE_TASK) == 0)  
        || r.launchMode == ActivityInfo.LAUNCH_SINGLE_TASK  
        || r.launchMode == ActivityInfo.LAUNCH_SINGLE_INSTANCE) {  
            // If bring to front is requested, and no result is requested, and  
            // we can find a task that was started with this same  
            // component, then instead of launching bring that one to the front.  
            if (r.resultTo == null) {  
                // See if there is a task to bring to the front.  If this is  
                // a SINGLE_INSTANCE activity, there can be one and only one  
                // instance of it in the history, and it is always in its own  
                // unique task, so we do a special search.  
                ActivityRecord taskTop = r.launchMode != ActivityInfo.LAUNCH_SINGLE_INSTANCE  
                    ? findTaskLocked(intent, r.info)  
                    : findActivityLocked(intent, r.info);  
                if (taskTop != null) {  
                    ......  
                }  
            }  
    }  

    ......  

    if (r.packageName != null) {  
        // If the activity being launched is the same as the one currently  
        // at the top, then we need to check if it should only be launched  
        // once.  
        ActivityRecord top = topRunningNonDelayedActivityLocked(notTop);  
        if (top != null && r.resultTo == null) {  
            if (top.realActivity.equals(r.realActivity)) {  
                ......  
            }  
        }  
    } else {  
        ......  
    }  

    boolean newTask = false;  

    // Should this be considered a new task?  
    if (r.resultTo == null && !addingToTask  
        && (launchFlags&Intent.FLAG_ACTIVITY_NEW_TASK) != 0) {  
            // todo: should do better management of integers.  
            mService.mCurTask++;  
            if (mService.mCurTask <= 0) {  
                mService.mCurTask = 1;  
            }  
            r.task = new TaskRecord(mService.mCurTask, r.info, intent,  
                (r.info.flags&ActivityInfo.FLAG_CLEAR_TASK_ON_LAUNCH) != 0);  
            ......  
            newTask = true;  
            if (mMainStack) {  
                mService.addRecentTaskLocked(r.task);  
            }  

    } else if (sourceRecord != null) {  
        ......  
    } else {  
      ......  
    }  

    //......  
    startActivityLocked(r, newTask, doResume);  
    return START_SUCCESS;  
}
~~~


但是！这里Binder通信是单方向的，即从ActivityManagerProxy指向ActivityManagerService的，如果AMS想要通知ActivityThread做一些事情，应该咋办呢？
还是通过Binder通信，不过是换了另外一对，换成了ApplicationThread和ApplicationThreadProxy。

客户端：ApplicationThread <=====Binder驱动<===== ApplicationThreadProxy：服务器
他们也都实现了相同的接口IApplicationThread：
~~~ Java
private class ApplicationThread extends ApplicationThreadNative {}

public abstract class ApplicationThreadNative extends Binder implements IApplicationThread{}

class ApplicationThreadProxy implements IApplicationThread {}

public interface IApplicationThread extends IInterface {}
~~~
IApplicationThread的源码在这里：[android.googlesource](https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/app/IApplicationThread.java)





这部分内容转载自：[【凯子哥带你学Framework】Activity启动过程全解析](http://blog.csdn.net/zhaokaiqiang1992/article/details/49428287)

### 结合Android Framework源码分析Activity启动流程
在Android系统中，有两种操作会引发Activity的启动，一种用户点击应用程序图标时，Launcher会为我们启动应用程序的主Activity；另一种是，应用程序的默认Activity启动起来后，它又可以在内部通过调用startActvity接口启动新的Activity，依此类推，每一个Activity都可以在内部启动新的Activity。通过这种连锁反应，按需启动Activity，从而完成应用程序的功能。

这里，我们通过一个具体的例子来说明如何启动Android应用程序的Activity。Activity的启动方式有两种，一种是显式的，一种是隐式的，隐式启动可以使得Activity之间的藕合性更加松散，因此，这里只关注隐式启动Activity的方法：
~~~ Java
Intent intent = new Intent("shy.luo.activity.subactivity");  
startActivity(intent);  

//在清单文件中
<activity android:name=".SubActivity"  
         android:label="@string/sub_activity">  
   <intent-filter>  
       <action android:name="shy.luo.activity.subactivity"/>  
       <category android:name="android.intent.category.DEFAULT"/>  
   </intent-filter>  
</activity>  
~~~

无论是通过点击应用程序图标来启动Activity，还是通过Activity内部调用startActivity接口来启动新的Activity，都要借助于应用程序框架层的ActivityManagerService服务进程。在Android应用程序框架层中，ActivityManagerService是一个非常重要的接口，它不但负责启动Activity和Service，还负责管理Activity和Service。

Android应用程序框架层中的ActivityManagerService启动Activity的过程大致如下图所示：

![图片来自：http://blog.csdn.net/luoshengyang/article/details/6685853](images/blogimages/2017/activity-start-process.png)
在这个图中，ActivityManagerService和ActivityStack位于同一个进程中，而ApplicationThread和ActivityThread位于另一个进程中。其中，ActivityManagerService是负责管理Activity的生命周期的，ActivityManagerService还借助ActivityStack是来把所有的Activity按照后进先出的顺序放在一个堆栈中；对于每一个应用程序来说，都有一个ActivityThread来表示应用程序的主进程，而每一个ActivityThread都包含有一个ApplicationThread实例，它是一个Binder对象，负责和其它进程进行通信。

下面简要介绍一下启动的过程：

- Step 1. 无论是通过Launcher来启动Activity，还是通过Activity内部调用startActivity接口来启动新的Activity，都通过Binder进程间通信进入到ActivityManagerService进程中，并且调用ActivityManagerService.startActivity接口；
- Step 2. ActivityManagerService调用ActivityStack.startActivityMayWait来做准备要启动的Activity的相关信息；
- Step 3. ActivityStack通知ApplicationThread要进行Activity启动调度了，这里的ApplicationThread代表的是调用ActivityManagerService.startActivity接口的进程，对于通过点击应用程序图标的情景来说，这个进程就是Launcher了，而对于通过在Activity内部调用startActivity的情景来说，这个进程就是这个Activity所在的进程了；
- Step 4. ApplicationThread不执行真正的启动操作，它通过调用ActivityManagerService.activityPaused接口进入到ActivityManagerService进程中，看看是否需要创建新的进程来启动Activity；
- Step 5. 对于通过点击应用程序图标来启动Activity的情景来说，ActivityManagerService在这一步中，会调用startProcessLocked来创建一个新的进程，而对于通过在Activity内部调用startActivity来启动新的Activity来说，这一步是不需要执行的，因为新的Activity就在原来的Activity所在的进程中进行启动；
- Step 6. ActivityManagerServic调用ApplicationThread.scheduleLaunchActivity接口，通知相应的进程执行启动Activity的操作；
- Step 7. ApplicationThread把这个启动Activity的操作转发给ActivityThread，ActivityThread通过ClassLoader导入相应的Activity类，然后把它启动起来。


### 参考资料

Android源码分析-Activity的启动过程：http://blog.csdn.net/singwhatiwanna/article/details/18154335

罗老师的，Android应用程序的Activity启动过程简要介绍和学习计划：http://blog.csdn.net/luoshengyang/article/details/6685853
      Android应用程序启动过程源代码分析：http://blog.csdn.net/luoshengyang/article/details/6689748

【凯子哥带你学Framework】Activity启动过程全解析 http://blog.csdn.net/zhaokaiqiang1992/article/details/49428287

Android系统篇之—-解读AMS远端服务调用机制以及Activity的启动流程：
http://www.wjdiankong.cn/android%E7%B3%BB%E7%BB%9F%E7%AF%87%E4%B9%8B-%E8%A7%A3%E8%AF%BBams%E8%BF%9C%E7%AB%AF%E6%9C%8D%E5%8A%A1%E8%B0%83%E7%94%A8%E6%9C%BA%E5%88%B6%E4%BB%A5%E5%8F%8Aactivity%E7%9A%84%E5%90%AF%E5%8A%A8/
