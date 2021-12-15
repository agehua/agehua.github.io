---
layout: post
title: ANR 总结与分析
category: accumulation
tags:
    - ANR
keywords: ANR, InputDispatcher
banner: https://cdn.conorlee.top/French%20Peasant%20Woman%20Suckling%20Her%20Baby%20after%20Dalou.jpg
thumbnail: https://cdn.conorlee.top/French%20Peasant%20Woman%20Suckling%20Her%20Baby%20after%20Dalou.jpg
toc: true
---

### ANR 分类
Android ANR问题分为三类：Input，Receiver，Service

在我们常见的APP开发中，主要遇到的都是Input超时问题，本文就详细分析下Input超时是如何产生以及如何排查
<!--more-->
在分析Input超时之前，我们先来简单的介绍一下Android的Input系统。Android Input体系中，大致有两种类型的事件，实体按键key事件，屏幕点击触摸事件，当然如果根据事件类型的不同我们还能细分为基础实体按键的key(power，volume up/down，recents，back，home)，实体键盘按键，屏幕点击(多点，单点)，屏幕滑动等等的事件。在Android整个Input体系中有三个格外重要的成员：Eventhub，InputReader，InputDispatcher。它们分别担负着各自不同的职责，Eventhub负责监听/dev/input产生Input事件，InputReader负责从Eventhub读取事件，并将读取的事件发给InputDispatcher，InputDispatcher则根据实际的需要具体分发给当前手机获得焦点实际的Window。当然它们三者之间有工作远比我介绍的要复杂的很多。

详细源码可以参考：[Input系统—ANR原理分析](http://gityuan.com/2017/01/01/input-anr/)

### ANR 产生原因
Input ANR reason主要有以下几类：

- 无窗口, 有应用：Waiting because no window has focus but there is a focused application that may eventually add a window when it finishes starting up.
- 窗口暂停: Waiting because the [targetType] window is paused.
- 窗口未连接: Waiting because the [targetType] window’s input channel is not registered with the input dispatcher. The window may be in the process of being removed.
- 窗口连接已死亡：Waiting because the [targetType] window’s input connection is [Connection.Status]. The window may be in the process of being removed.
- 窗口连接已满：Waiting because the [targetType] window’s input channel is full. Outbound queue length: [outboundQueue长度]. Wait queue length: [waitQueue长度].
- 按键事件，输出队列或事件等待队列不为空：Waiting to send key event because the [targetType] window has not finished processing all of the input events that were previously delivered to it. Outbound queue length: [outboundQueue长度]. Wait queue length: [waitQueue长度].
- 非按键事件，事件等待队列不为空且头事件分发超时500ms：Waiting to send non-key event because the [targetType] window has not finished processing certain input events that were delivered to it over 500ms ago. Wait queue length: [waitQueue长度]. Wait queue head age: [等待时长].

> 其中
- targetType: 取值为”focused”或者”touched”
- Connection.Status: 取值为”NORMAL”，”BROKEN”，”ZOMBIE”

### 实例分析，Reason: Waiting because the focused window is paused.
最近遇到这个问题，但是没有trace文件可以分析，只有logcat和dumpsys文件。
下面利用这两个文件尝试分析下如何定位这类ANR
先看下logcat内容

- step1, 在logcat中搜索 `Application is not responding`，并确认这条日志的时间点：06-01 04:38:39.160
~~~ java
// 第一步搜索 Application is not responding
06-01 04:38:39.160  1457  1669 I InputDispatcher: Application is not responding: AppWindowToken{b7d5d7c token=Token{5a5c6f ActivityRecord{12f034e u0 com.xxx/.MainActivity t4569}}} - Window{703780b u0 cxxx/xxx.xActivity}.  It has been 5008.2ms since event, 5003.2ms since wait started.  Reason: Waiting because the focused window is paused.
~~~
- step2, ANR属于InputDispatcher类型，向前选5s，大概在 06-01 04:38:34的时间点
- step3, 在上一步的时间点，看看有没有到对应的input事件，找到了：
~~~ java
06-01 04:38:34.151 23143 23143 D AndroidRuntime: Calling main entry com.android.commands.input.Input
06-01 04:38:34.152 23143 23143 I Input   : injectKeyEvent: KeyEvent { action=ACTION_DOWN, keyCode=KEYCODE_BACK, scanCode=0, metaState=0, flags=0x0, repeatCount=0, eventTime=2108288622, downTime=2108288622, deviceId=-1, source=0x101 }
~~~
- step4, 根据事件，猜测是点击返回时发生了ANR，

- step5, 搜索关键字 `ANR in`，查看CPU占用，发现CPU占用过高
~~~ java
06-01 04:38:49.302  1457 23156 E ActivityManager: ANR in xxx (xxx/.x,xActivity)
06-01 04:38:49.302  1457 23156 E ActivityManager: PID: 18833
06-01 04:38:49.302  1457 23156 E ActivityManager: Reason: Input dispatching timed out (Waiting because the focused window is paused.), happend at time = 2108297657@#@18833
06-01 04:38:49.302  1457 23156 E ActivityManager: Load: 0.0 / 0.0 / 0.0
06-01 04:38:49.302  1457 23156 E ActivityManager: CPU usage from 105ms to -6001ms ago:
06-01 04:38:49.302  1457 23156 E ActivityManager:   131% 18833/xxx: 30% user + 100% kernel / faults: 19725 minor 18 major
06-01 04:38:49.302  1457 23156 E ActivityManager:   33% 19724/com.android.commands.monkey: 29% user + 3.6% kernel / faults: 5507 minor 1 major
06-01 04:38:49.302  1457 23156 E ActivityManager:   20% 1457/system_server: 12% user + 8.3% kernel / faults: 1084 minor 7 major
06-01 04:38:49.302  1457 23156 E ActivityManager:   10% 452/surfaceflinger: 3.9% user + 6% kernel / faults: 45 minor
~~~
虽然现在手机基本都是多核CPU，比如4核的话，CPU上线是 400%。但App正常使用时，单个CPU占用应该不超过100%。
到这里猜测，在点击按钮返回时，页面刷新App CPU占用过高，导致ANR，此时对应Activity的onPause和onResume两个生命周期，


在分析一下为什么ANR 原因是 Input dispatching timed out (Waiting because the focused window is paused.)

#### 查看 dumpsys 文件
在 dumpsys 文件中搜索 `Input Dispatcher`，找到下面的内容，注释内容是我自己添加的，便于理解
~~~ java
Input Dispatcher State at time of last ANR:
  ANR:
    Time: 2021-06-01 04:38:39 // 这里时间跟 step1 的时间一致
    Window: AppWindowToken{b7d5d7c token=Token{5a5c6f ActivityRecord{12f034e u0 xxx/.xxxActivity t4569}}} - Window{703780b u0 xxx/xxx.xActivity}
    // 注意上面的 Window{703780b ...
    DispatchLatency: 5008.2ms
    WaitDuration: 5003.2ms
    Reason: Waiting because the focused window is paused.
  DispatchEnabled: 1
  DispatchFrozen: 0
  FocusedApplication: name='AppWindowToken{b7d5d7c token=Token{5a5c6f ActivityRecord{12f034e u0 xxx/.xxxActivity t4569}}}', dispatchingTimeout=5000.000ms
  FocusedWindow: name='Window{703780b u0 xxx/xxx.xActivity}'
  TouchStates: <no displays touched>
  Windows:
    //.... 省略
    // 根据 703780b 找到对应的window，同时发现有两个window状态是 paused，但Window{6dfcc50  hasFocus=false 暂不考虑
    8: name='Window{6dfcc50 u0 PopupWindow:408c8f7}', displayId=0, paused=true, hasFocus=false, hasWallpaper=false, visible=true, canReceiveKeys=false, flags=0x41800008, type=0x000003e8, layer=21020, frame=[223,149][474,208], scale=1.000000, touchableRegion=[223,149][474,208], inputFeatures=0x00000000, ownerPid=18833, ownerUid=13560, dispatchingTimeout=5000.000ms
    // Window{703780b paused=true
    9: name='Window{703780b u0 xxx/xxx.xActivity}', displayId=0, paused=true, hasFocus=true, hasWallpaper=false, visible=true, canReceiveKeys=true, flags=0x81810520, type=0x00000001, layer=21015, frame=[0,0][1280,720], scale=1.000000, touchableRegion=[0,0][1280,720], inputFeatures=0x00000000, ownerPid=18833, ownerUid=13560, dispatchingTimeout=5000.000ms
~~~
在 dumpsys 文件中 继续搜索 `WINDOW MANAGER LAST ANR `，找到 **Window{703780b**
~~~ java
-------------------------------------------------------------------------------
WINDOW MANAGER LAST ANR (dumpsys window lastanr)
//.... 省略
Window #3 Window{703780b u0 xxx/xxx.xActivity}:
    mDisplayId=0 stackId=156 mSession=Session{eaba5cd 18833:u0a13560} mClient=android.os.BinderProxy@9fb1bda
    mOwnerUid=13560 mShowToOwnerOnly=true package=com.xxx appop=NONE
    mAttrs=WM.LayoutParams{(0,0)(fillxfill) sim=#20 ty=1 fl=#81810500 wanim=0x103045b vsysui=0x1606 sysuil=true needsMenuKey=2}
    Requested w=1280 h=720 mLayoutSeq=311401
    mBaseLayer=21000 mSubLayer=0 mAnimLayer=21015+0=21015 mLastLayer=21015
    mToken=AppWindowToken{31531d3 token=Token{40bffc2 ActivityRecord{82ca40d u0 xxx/xxx.xActivity t4569}}}
    mRootToken=AppWindowToken{31531d3 token=Token{40bffc2 ActivityRecord{82ca40d u0 xxx/xxx.xActivity t4569}}}
    mAppToken=AppWindowToken{31531d3 token=Token{40bffc2 ActivityRecord{82ca40d u0 xxx/xxx.xActivity t4569}}}
    mViewVisibility=0x0 mHaveFrame=true mObscured=false
    mSeq=2 mSystemUiVisibility=0x1606
    mGivenContentInsets=[0,0][0,0] mGivenVisibleInsets=[0,0][0,0]
    mConfiguration={1.12 ?mcc?mnc zh_CN ldltr sw360dp w640dp h342dp 320dpi nrml long land finger -keyb/v/h -nav/h s.339mThemeChanged = 0mThemeChangedFlags = 0mFlipFont = 0}
    mHasSurface=true mShownFrame=[0.0,0.0][1280.0,720.0] isReadyForDisplay()=true
    mFrame=[0,0][1280,720] last=[0,0][1280,720]
    mSystemDecorRect=[0,0][1280,720] last=[0,0][0,0]
    Frames: containing=[0,0][1280,720] parent=[0,0][1280,720]
        display=[0,0][1280,720] overscan=[0,0][1280,720]
        content=[0,0][1280,720] visible=[0,0][1280,720]
        decor=[0,0][1280,720]
        outset=[0,0][1280,720]
    Cur insets: overscan=[0,0][0,0] content=[0,0][0,0] visible=[0,0][0,0] stable=[0,36][0,0] outsets=[0,0][0,0]
    Lst insets: overscan=[0,0][0,0] content=[0,0][0,0] visible=[0,0][0,0] stable=[0,36][0,0] physical=[0,0][0,0] outset=[0,0][0,0]
    WindowStateAnimator{63d5e15 xxx/xxx.xActivity}:
      mSurface=Surface(name=xxx/xxx.xActivity)
      mDrawState=HAS_DRAWN mLastHidden=false
      Surface: shown=true layer=21015 alpha=1.0 rect=(0.0,0.0) 1280.0 x 720.0
      mGlobalScale=1.0 mDsDx=1.0 mDtDx=0.0 mDsDy=0.0 mDtDy=1.0
~~~
从上面日志可以发现这个window，属于是AppWindowToken，说明这个window就是一个Activity。

如果对AppWindowToken和ActivityRecord感兴趣的，可以看下罗老师的这篇文章：[https://blog.csdn.net/luoshengyang/article/details/8498908](https://blog.csdn.net/luoshengyang/article/details/8498908)

这里列一下返回时，两个Activity的生命周期交互
> onBackPressed() 流程：
SecondActivity - onPause() --- MainActivity - onRestart() --- MainActivity - onStart() --- MainActivity - onResume() --- SecondActivity - onStop() --- SecondActivity - onDestroy()

综合上面的分析和猜测 :) , 需要着重看下xxActivity的onPause和下层Activity的onResume方法。

OK，到这里我们就知道了该如何分析 `Waiting because the focused window is paused` 对应的ANR了。


### Waiting because no window has focus but there is a focused application that may eventually add a window when it finishes starting up

在dumpsys文件中：
~~~ java
Input Dispatcher State at time of last ANR:
  ANR:
    Time: 2021-05-31 23:33:18
    // 跟上面分析过的 dumpsys文件对比，发现这里没有 - Window{id u0 xxx/xxx.xActivity}
    Window: AppWindowToken{9a5328e token=Token{caaf289 ActivityRecord{b8e1990 u0 com.xxx/oxxx.PlayerActivity t4506}}}
    DispatchLatency: 5003.8ms
    WaitDuration: 5000.7ms
    Reason: Waiting because no window has focus but there is a focused application that may eventually add a window when it finishes starting up.
  DispatchEnabled: 1
  DispatchFrozen: 0
  FocusedApplication: name='AppWindowToken{9a5328e token=Token{caaf289 ActivityRecord{b8e1990 u0 com.xxx/oxxx.PlayerActivity t4506}}}', dispatchingTimeout=5000.000ms
  FocusedWindow: name='<null>'
  TouchStates: <no displays touched>
  Windows:
  // 这里有一个对应包名的window
  Window #8 Window{7e932b5 u0 Starting com.xxx}:
    mDisplayId=0 stackId=156 mSession=Session{86d25c5 1457:1000} mClient=android.view.ColorViewRootImplHooks$ColorW@7d1ceec
    mOwnerUid=1000 mShowToOwnerOnly=false package=com.xxx appop=NONE
    mAttrs=WM.LayoutParams{(0,0)(fillxfill) sim=#20 ty=3 fl=#81830518 pfl=0x11 wanim=0x103045b vsysui=0x600 needsMenuKey=2}
    Requested w=1280 h=720 mLayoutSeq=292571
    mBaseLayer=21000 mSubLayer=0 mAnimLayer=21040+0=21040 mLastLayer=21040
    mToken=AppWindowToken{9a5328e token=Token{caaf289 ActivityRecord{b8e1990 u0 com.xxx/oxxx.PlayerActivity t4506}}}
    mRootToken=AppWindowToken{9a5328e token=Token{caaf289 ActivityRecord{b8e1990 u0 com.xxx/oxxx.PlayerActivity t4506}}}
    mAppToken=AppWindowToken{9a5328e token=Token{caaf289 ActivityRecord{b8e1990 u0 com.xxx/oxxx.PlayerActivity t4506}}}
    mViewVisibility=0x0 mHaveFrame=true mObscured=false
    mSeq=0 mSystemUiVisibility=0x600
    mGivenContentInsets=[0,0][0,0] mGivenVisibleInsets=[0,0][0,0]
    mConfiguration={1.12 ?mcc?mnc zh_CN ldltr sw360dp w640dp h342dp 320dpi nrml long land finger -keyb/v/h -nav/h s.119mThemeChanged = 0mThemeChangedFlags = 0mFlipFont = 0}
    mHasSurface=true mShownFrame=[0.0,0.0][1280.0,720.0] isReadyForDisplay()=true
    mFrame=[0,0][1280,720] last=[0,0][1280,720]
    mSystemDecorRect=[0,0][1280,720] last=[0,0][0,0]
    Frames: containing=[0,0][1280,720] parent=[0,0][1280,720]
        display=[0,0][1280,720] overscan=[0,0][1280,720]
        content=[0,0][1280,720] visible=[0,0][1280,720]
        decor=[0,0][1280,720]
        outset=[0,0][1280,720]
    Cur insets: overscan=[0,0][0,0] content=[0,0][0,0] visible=[0,0][0,0] stable=[0,36][0,0] outsets=[0,0][0,0]
    Lst insets: overscan=[0,0][0,0] content=[0,0][0,0] visible=[0,0][0,0] stable=[0,36][0,0] physical=[0,0][0,0] outset=[0,0][0,0]
    WindowStateAnimator{b277152 Starting com.xxx}:
      mSurface=Surface(name=Starting com.xxx)
      mDrawState=HAS_DRAWN mLastHidden=false
      Surface: shown=true layer=21040 alpha=1.0 rect=(0.0,0.0) 1280.0 x 720.0
      mGlobalScale=1.0 mDsDx=1.0 mDtDx=0.0 mDsDy=0.0 mDtDy=1.0
~~~

在logcat文件中：
~~~ java
// input事件发生的时间点
05-31 23:33:18.336  1457  1669 I WindowManager: Input event dispatching timed out sending to application AppWindowToken{9a5328e token=Token{caaf289 ActivityRecord{b8e1990 u0 com.xxx/oxxx.PlayerActivity t4506}}}.  Reason: Waiting because no window has focus but there is a focused application that may eventually add a window when it finishes starting up.
05-31 23:33:18.341 28229 28258 I chatty  : uid=13560(com.xxx) expire 2 lines
05-31 23:33:18.344  1457  1669 D ActivityManager:  ANR post Runnable for ProcessRecord{772ec6 28229:com.xxx/u0a3560} to deal with anr happend at 2089972814@#@28229
05-31 23:33:18.344  1457  1669 D ActivityManager:  ANR caller(2) = com.android.server.am.ActivityRecord$Token.keyDispatchingTimedOut:388 com.android.server.wm.InputMonitor.notifyANR:134 com.android.server.input.InputManagerService.notifyANR:1625 <bottom of call stack> <bottom of call stack> <bottom of call stack> <bottom of call stack> <bottom of call stack> 
05-31 23:33:18.347  1457  1527 W InputManager: Input event injection from pid 28213 timed out.
05-31 23:33:18.348  1457 28282 D ActivityManager: inputDispatchingTimedOut appNotResponding: proc ProcessRecord{772ec6 28229:com.xxx/u0a3560}
05-31 23:33:18.349  1457 28282 D ActivityManager: appNotResponding: activity ActivityRecord{b8e1990 u0 com.xxx/oxxx.PlayerActivity t4506}
05-31 23:33:18.350 28213 28213 I Input   : injectKeyEvent: KeyEvent { action=ACTION_UP, keyCode=KEYCODE_BACK, scanCode=0, metaState=0, flags=0x0, repeatCount=0, eventTime=2089967799, downTime=2089967799, deviceId=-1, source=0x101 }
05-31 23:33:18.353  1457 28284 D ActivityManager: ANR deal 3 appNotResponding for ProcessRecord{772ec6 28229:com.xxx/u0a3560}, pid = 28229, annotation = Input dispatching timed out (Waiting because no window has focus but there is a focused application that may eventually add a window when it finishes starting up.), happend at time = 2089972814@#@28229

// ANR 收集到的时间点
05-31 23:33:24.416  1457 28284 E ActivityManager: ANR in com.xxx (com.xxx/oxxx.PlayerActivity)
05-31 23:33:24.416  1457 28284 E ActivityManager: PID: 28229
05-31 23:33:24.416  1457 28284 E ActivityManager: Reason: Input dispatching timed out (Waiting because no window has focus but there is a focused application that may eventually add a window when it finishes starting up.), happend at time = 2089972814@#@28229
05-31 23:33:24.416  1457 28284 E ActivityManager: Load: 0.0 / 0.0 / 0.0
05-31 23:33:24.416  1457 28284 E ActivityManager: CPU usage from 955ms to -6002ms ago:
05-31 23:33:24.416  1457 28284 E ActivityManager:   61% 26158/com.android.commands.monkey: 22% user + 39% kernel / faults: 3743 minor 26 major
05-31 23:33:24.416  1457 28284 E ActivityManager:   20% 1457/system_server: 12% user + 7.8% kernel / faults: 16282 minor 21 major
05-31 23:33:24.416  1457 28284 E ActivityManager:   9.6% 6732/VosTXThread: 0% user + 9.6% kernel
05-31 23:33:24.416  1457 28284 E ActivityManager:  +0% 28229/com.xxx: 0% user + 0% kernel
~~~
发现我们app的CPU 占用是0%，说明可能是我们的应用还没有启动起来，就发生了ANR。此时CPU占用最高的是monkey线程。

幸运的是这次抓到trace文件

#### 关键：traces.txt 日志分析
关于traces文件分析，我参考的是这篇文章：[Android ANR日志分析指南](https://zhuanlan.zhihu.com/p/50107397)

当APP不响应、响应慢了、或者WatchDog的监视没有得到回应时，系统就会dump出一个traces.txt文件，存放在文件目录: `/data/anr/traces.txt`，通过traces文件,我们可以拿到线程名、堆栈信息、线程当前状态、binder call等信息。

> 通过adb命令拿到该文件：`adb pull /data/anr/traces.txt`

我详细解析一下traces.txt里面的一些字段，看看它到底能给我们提供什么信息.
main：main标识是主线程，如果是线程，那么命名成“Thread-X”的格式,x表示线程id,逐步递增。
prio：线程优先级,默认是5
tid：tid不是线程的id，是线程唯一标识ID
group：是线程组名称
sCount：该线程被挂起的次数
dsCount：是线程被调试器挂起的次数
obj：对象地址
self：该线程Native的地址
sysTid：是线程号(主线程的线程号和进程号相同)
nice：是线程的调度优先级
sched：分别标志了线程的调度策略和优先级
cgrp：调度归属组
handle：线程处理函数的地址。
state：是调度状态
schedstat：从 /proc/[pid]/task/[tid]/schedstat读出，三个值分别表示线程在cpu上执行的时间、线程的等待时间和线程执行的时间片长度，不支持这项信息的三个值都是0；
utm：是线程用户态下使用的时间值(单位是jiffies）
stm：是内核态下的调度时间值
core：是最后执行这个线程的cpu核的序号。

main的堆栈信息是我们最关心的，它能够定位到具体位置。从上面的traces,我们可以判断java.io.File.isAbsolute 导致发生了ANR。这时候可以对着源码查看，找到出问题，并且解决它。

这里就主要看 mian线程
~~~ java
// ... 省略
suspend all histogram:	Sum: 29.777ms 99% C.I. 4us-17039.359us Avg: 465.265us Max: 20121us
DALVIK THREADS (36):
"Signal Catcher" daemon prio=5 tid=2 Runnable 
  | group="system" sCount=0 dsCount=0 obj=0x12d0a0a0 self=0xed7b8600
  | sysTid=28234 nice=0 cgrp=default sched=0/0 handle=0xf497f930
  | state=R schedstat=( 5272239 1424062 9 ) utm=0 stm=0 core=6 HZ=100
  | stack=0xf4883000-0xf4885000 stackSize=1014KB
  | held mutexes= "mutator lock"(shared held)
  native: #00 pc 0035e8cd  /system/lib/libart.so (_ZN3art15DumpNativeStackERNSt3__113basic_ostreamIcNS0_11char_traitsIcEEEEiPKcPNS_9ArtMethodEPv+116)
  native: #01 pc 0033ed6b  /system/lib/libart.so (_ZNK3art6Thread4DumpERNSt3__113basic_ostreamIcNS1_11char_traitsIcEEEE+138)
  native: #02 pc 00348ce5  /system/lib/libart.so (_ZN3art14DumpCheckpoint3RunEPNS_6ThreadE+420)
  native: #03 pc 00349841  /system/lib/libart.so (_ZN3art10ThreadList13RunCheckpointEPNS_7ClosureE+192)
  native: #04 pc 00349d41  /system/lib/libart.so (_ZN3art10ThreadList4DumpERNSt3__113basic_ostreamIcNS1_11char_traitsIcEEEE+124)
  native: #05 pc 0034a429  /system/lib/libart.so (_ZN3art10ThreadList14DumpForSigQuitERNSt3__113basic_ostreamIcNS1_11char_traitsIcEEEE+312)
  native: #06 pc 0031f8e3  /system/lib/libart.so (_ZN3art7Runtime14DumpForSigQuitERNSt3__113basic_ostreamIcNS1_11char_traitsIcEEEE+66)
  native: #07 pc 0032a007  /system/lib/libart.so (_ZN3art13SignalCatcher13HandleSigQuitEv+910)
  native: #08 pc 0032aa2d  /system/lib/libart.so (_ZN3art13SignalCatcher3RunEPv+668)
  native: #09 pc 0003fc7b  /system/lib/libc.so (_ZL15__pthread_startPv+30)
  native: #10 pc 00019fc5  /system/lib/libc.so (__start_thread+6)
  (no managed stack frames)

"main" prio=5 tid=1 Suspended // 状态是暂停
  | group="main" sCount=1 dsCount=0 obj=0x740c1360 self=0xf5402500
  | sysTid=28229 nice=0 cgrp=default sched=0/0 handle=0xf76e5b34
  | state=S schedstat=( 489913748 62354685 405 ) utm=38 stm=10 core=7 HZ=100
  | stack=0xff78b000-0xff78d000 stackSize=8MB
  | held mutexes=
  at java.io.File.isAbsolute(File.java:510) 
  at java.io.File.getAbsolutePath(File.java:373)
  at com.tencent.tinker.lib.tinker.Tinker$Builder.<init>(Proguard:359)
  at com.tencent.tinker.lib.tinker.con.a(Proguard:63)
  at com.xxx.hotfix.con.a(Proguard:127)
  at com.xxx.hotfix.con.a(Proguard:46)
  at com.xxx.hotfix.c.con.b(Proguard:198)
  at com.xxx.ApplicationDelegate.initTinker(Proguard:208)
  at com.xxx.ApplicationDelegate.onCreate(Proguard:152)
  at com.tencent.tinker.entry.TinkerApplicationInlineFence.onCreateImpl_$noinline$(Proguard:99)
  at com.tencent.tinker.entry.TinkerApplicationInlineFence.onCreate(Proguard:110)
  at com.tencent.tinker.loader.app.TinkerApplication.onCreate(Proguard:153)
  at android.app.Instrumentation.callApplicationOnCreate(Instrumentation.java:1014)
  at android.app.ActivityThread.handleBindApplication(ActivityThread.java:4995)
  at android.app.ActivityThread.access$1800(ActivityThread.java:170)
  at android.app.ActivityThread$H.handleMessage(ActivityThread.java:1555)
  at android.os.Handler.dispatchMessage(Handler.java:102)
  at android.os.Looper.loop(Looper.java:179)
  at android.app.ActivityThread.main(ActivityThread.java:5769)
  at java.lang.reflect.Method.invoke!(Native method)
  at com.android.internal.os.ZygoteInit$MethodAndArgsCaller.run(ZygoteInit.java:784)
  at com.android.internal.os.ZygoteInit.main(ZygoteInit.java:674)

"ReferenceQueueDaemon" daemon prio=5 tid=4 Waiting
  | group="system" sCount=1 dsCount=0 obj=0x12d04be0 self=0xf5402a00
  | sysTid=28235 nice=0 cgrp=default sched=0/0 handle=0xf4880930
  | state=S schedstat=( 947291 9095001 12 ) utm=0 stm=0 core=7 HZ=100
  | stack=0xf477e000-0xf4780000 stackSize=1038KB
  | held mutexes=
  at java.lang.Object.wait!(Native method)
  - waiting on <0x09e82a5a> (a java.lang.Class<java.lang.ref.ReferenceQueue>)
  at java.lang.Daemons$ReferenceQueueDaemon.run(Daemons.java:152)
  - locked <0x09e82a5a> (a java.lang.Class<java.lang.ref.ReferenceQueue>)
  at java.lang.Thread.run(Thread.java:818)

"FinalizerDaemon" daemon prio=5 tid=5 Waiting
  | group="system" sCount=1 dsCount=0 obj=0x12d04c40 self=0xf5402f00
  | sysTid=28236 nice=0 cgrp=default sched=0/0 handle=0xf477b930
  | state=S schedstat=( 899169 4021874 6 ) utm=0 stm=0 core=5 HZ=100
  | stack=0xf4679000-0xf467b000 stackSize=1038KB
  | held mutexes=
  at java.lang.Object.wait!(Native method)
  - waiting on <0x005da48b> (a java.lang.ref.ReferenceQueue)
  at java.lang.Object.wait(Object.java:423)
  at java.lang.ref.ReferenceQueue.remove(ReferenceQueue.java:101)
  - locked <0x005da48b> (a java.lang.ref.ReferenceQueue)
  at java.lang.ref.ReferenceQueue.remove(ReferenceQueue.java:72)
  at java.lang.Daemons$FinalizerDaemon.run(Daemons.java:193)
  at java.lang.Thread.run(Thread.java:818)

"FinalizerWatchdogDaemon" daemon prio=5 tid=6 Waiting
  | group="system" sCount=1 dsCount=0 obj=0x12d04ca0 self=0xf5403400
  | sysTid=28237 nice=0 cgrp=default sched=0/0 handle=0xf4676930
  | state=S schedstat=( 899684 3116201 10 ) utm=0 stm=0 core=7 HZ=100
  | stack=0xf4574000-0xf4576000 stackSize=1038KB
  | held mutexes=
  at java.lang.Object.wait!(Native method)
  - waiting on <0x0e839968> (a java.lang.Daemons$FinalizerWatchdogDaemon)
  at java.lang.Daemons$FinalizerWatchdogDaemon.waitForObject(Daemons.java:275)
  - locked <0x0e839968> (a java.lang.Daemons$FinalizerWatchdogDaemon)
  at java.lang.Daemons$FinalizerWatchdogDaemon.run(Daemons.java:244)
  at java.lang.Thread.run(Thread.java:818)
~~~
看trace文件，应该是其他线程占用了资源，导致main线程暂停

最后，结合上面CPU占用分析，应该是monkey线程占用了资源，导致main线程挂起

###  拓展Service，Reason: executing service xxx/xxx.Service
在logcat中遇到一个Service ANR

Service Timeout是位于”ActivityManager”线程中的AMS.MainHandler收到SERVICE_TIMEOUT_MSG消息时触发。

对于Service有两类:

- 对于前台服务，则超时为SERVICE_TIMEOUT = 20s；
- 对于后台服务，则超时为SERVICE_BACKGROUND_TIMEOUT = 200s

由变量ProcessRecord.execServicesFg来决定是否前台启动
先看logcat日志，有如下内容，以时间先后排列：
~~~ java
// 系统进程都已经挂掉了
06-01 04:54:26.174  1457  1807 I ActivityManager: Process com.android.browser (pid 27980) has died
06-01 04:54:26.174  1457  1807 D ActivityManager: cleanUpApplicationRecord -- 27980
06-01 04:54:26.975  1457  1807 I ActivityManager: Start proc 28150:com.oppo.usercenter/u0a59 for content provider com.oppo.usercenter/com.platform.usercenter.newcommon.db.UserCenterContentProvider
06-01 04:54:27.011  1457  1471 I ActivityManager: Process com.oppo.market (pid 28086) has died
06-01 04:54:27.011  1457  1471 D ActivityManager: cleanUpApplicationRecord -- 28086
06-01 04:54:28.093  1457  2688 I ActivityManager: Process com.tencent.mm:push (pid 12440) has died
06-01 04:54:28.093  1457  2688 D ActivityManager: cleanUpApplicationRecord -- 12440
// 应用被force-stop, 发送广播ACTION_PACKAGE_RESTARTED，用于停止已注册的alarm,notification.
06-01 04:54:40.840  1457  1457 D JobSchedulerService: Receieved: android.intent.action.PACKAGE_RESTARTED
06-01 04:54:40.841  1457  1457 D JobSchedulerService: Stop app Removing jobs for uid: 13560 pkg = xxx
06-01 04:54:40.883  1457  1522 W WindowManager: Attempted to add application window with unknown token Token{6f25abd ActivityRecord{bf5f214 u0 xxx/oxxx.PlayerActivity t4576 f}}.  Aborting.
// 在一个Activity启动前，系统会先创建一个启动窗口作为过渡，这里starting window就代表启动窗口，启动窗口也添加失败
06-01 04:54:40.902  1457  1522 W WindowManager: Exception when adding starting window 
06-01 04:54:40.902  1457  1522 W WindowManager: java.lang.IllegalArgumentException: View=com.android.internal.policy.PhoneWindow$DecorView{7d429de V.E...... R.....ID 0,0-0,0} not attached to window manager
06-01 04:54:40.902  1457  1522 W WindowManager: 	at android.view.WindowManagerGlobal.findViewLocked(WindowManagerGlobal.java:424)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at android.view.WindowManagerGlobal.removeView(WindowManagerGlobal.java:350)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at android.view.WindowManagerImpl.removeViewImmediate(WindowManagerImpl.java:118)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at com.android.server.policy.PhoneWindowManager.addStartingWindow(PhoneWindowManager.java:2811)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at com.android.server.wm.WindowManagerService$H.handleMessage(WindowManagerService.java:8406)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at android.os.Handler.dispatchMessage(Handler.java:102)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at android.os.Looper.loop(Looper.java:179)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at android.os.HandlerThread.run(HandlerThread.java:61)
06-01 04:54:40.902  1457  1522 W WindowManager: 	at com.android.server.ServiceThread.run(ServiceThread.java:46)
// 杀掉应用进程
06-01 05:26:03.900  1457  2476 I ActivityManager: Force stopping xxx appid=13560 user=0: from pid 24185
06-01 05:26:03.901  1457  2476 I ActivityManager: Killing 7767:xxx/u0a3560 (adj 0): stop xxx
06-01 05:26:03.902  1457  2476 D ActivityManager: cleanUpApplicationRecord -- 7767
// 重启应用进程
06-01 05:26:25.005  1457  2643 I ActivityManager: Start proc 7916:xxx/u0a3560 for activity xxx/.WelcomeActivity
06-01 05:26:25.020 14580 14580 I MediaScannerReceiver: onReceive, intent=Intent { act=android.intent.action.MEDIA_SCANNER_SCAN_FILE dat=file:///sdcard/monkey_screenshot/122_2021-06-01_05-26-09.png flg=0x10 cmp=com.android.providers.media/.MediaScannerReceiver }

// window freeze timeout App freeze timeout  网上说这里是屏幕方向改变时，清理旧配置信息超时
06-01 05:26:27.071  1457  1522 W WindowManager: Window freeze timeout expired.
06-01 05:26:27.071  1457  1522 W WindowManager: Force clearing orientation change: Window{9ec2733 u0 Starting xxx}
06-01 05:26:27.071  1457  1522 W WindowManager: Force clearing orientation change: Window{38b9629 u0 xxx/xxx.WelcomeActivity}
06-01 05:26:27.071  1457  1522 W WindowManager: Force clearing orientation change: Window{863574f u0 xxx/xxx.WelcomeActivity}
06-01 05:26:27.071  1457  1522 W WindowManager: Force clearing orientation change: Window{4bf40a2 u0 com.android.systemui.ImageWallpaper}
06-01 05:26:27.075  1457  1522 V WindowManager: set sys.app_freeze_timeout: pkg=xxx
06-01 05:26:27.084  1457  1522 W WindowManager: App freeze timeout expired.
06-01 05:26:27.085  1457  1522 W WindowManager: Force clearing freeze: AppWindowToken{e409897 token=Token{68b58d8 ActivityRecord{17fbbb u0 xxx/.WelcomeActivity t4595}}}
06-01 05:26:27.145  1457  1522 I WindowManager: Screen frozen for +2s74ms due to Window{9ec2733 u0 Starting xxx}
06-01 05:26:27.156  1457  1522 I WindowManager: finishPostLayoutPolicyLw (statusbar != null ) true topIsFullscreen true top = Window{9ec2733 u0 Starting xxx}
06-01 05:26:27.156  1457  1522 I StatusBarManagerService:  topIsFullscreen fullscreen true
06-01 05:26:27.174  1457  1471 I WindowManager: Switching to real app window: Window{863574f u0 xxx/xxx.WelcomeActivity}

// launch timeout 可以看下这篇文章： https://blog.csdn.net/bruk_spp/article/details/97769376
06-01 05:26:47.226  1457  1497 W ActivityManager: Launch timeout has expired, giving up wake lock!
06-01 05:26:47.230 21977 21995 I Launcher.LauncherTraceUtil: writeLauncherStateToFile. The mStateTrace is null!
06-01 05:26:47.232  7916  7953 I chatty  : uid=13560(xxx) expire 2 lines

06-01 05:26:47.332  1457  8210 I Process : Sending signal. PID: 7916 SIG: 3
// ANR 发生的地方
06-01 05:26:47.234  1457  1497 W ActivityManager: Timeout executing service: ServiceRecord{d006edc u0 xxx/xxxService}
06-01 05:26:47.235  1457  1500 V OppoAppStartupManager: intent == null || callerApp == null
06-01 05:26:47.239  1457  1863 D BluetoothManagerService: checkIfCallerIsForegroundUser: valid=true callingUser=0 parentUser=-10000 foregroundUser=0
06-01 05:26:47.242  2646  2646 E ANR_LOG : >>> msg's executing time is too long
06-01 05:26:47.242  2646  2646 E ANR_LOG : Blocked msg = { when=-2s244ms what=3 target=com.qti.internal.telephony.gsm.QtiGsmServiceStateTracker obj=android.os.AsyncResult@f88f23e } , cost  = 2243 ms
06-01 05:26:47.242  2646  2646 E ANR_LOG : >>>Current msg List is:
06-01 05:26:47.242  2646  2646 E ANR_LOG : Current msg <1>  = { when=+20s0ms what=10 target=com.qti.internal.telephony.gsm.QtiGsmServiceStateTracker }
06-01 05:26:47.242  2646  2646 E ANR_LOG : >>>CURRENT MSG DUMP OVER<<<
06-01 05:26:47.243 14580 14580 I MediaScannerReceiver: onReceive, intent=Intent { act=android.intent.action.MEDIA_SCANNER_SCAN_FILE dat=file:///sdcard/monkey_screenshot/123_2021-06-01_05-26-49.png flg=0x10 cmp=com.android.providers.media/.MediaScannerReceiver }
06-01 05:26:47.245  1457  1457 E ANR_LOG : >>> msg's executing time is too long
06-01 05:26:47.245  1457  1457 E ANR_LOG : Blocked msg = { when=-5s541ms what=0 target=android.os.Handler callback=com.android.server.BatteryService$12 } , cost  = 5541 ms
06-01 05:26:47.245  1457  8210 D ActivityManager: ANR deal 3 appNotResponding for ProcessRecord{7d985f 7916:xxx/u0a3560}, pid = 7916, annotation = executing service xxx/xxx.Service

// 捕获到ANR的地方
06-01 05:26:53.364  1457  8210 E ActivityManager: ANR in xxx
06-01 05:26:53.364  1457  8210 E ActivityManager: PID: 7916
06-01 05:26:53.364  1457  8210 E ActivityManager: Reason: executing service xxx/xxx.Service
06-01 05:26:53.364  1457  8210 E ActivityManager: Load: 0.0 / 0.0 / 0.0
06-01 05:26:53.364  1457  8210 E ActivityManager: CPU usage from 0ms to 6087ms later:
06-01 05:26:53.364  1457  8210 E ActivityManager:   108% 7916/xxx: 21% user + 86% kernel / faults: 15083 minor 47 major
06-01 05:26:53.364  1457  8210 E ActivityManager:   9.2% 1457/system_server: 4.9% user + 4.2% kernel / faults: 958 minor
~~~

ANR发生的过程怀疑是，应用被杀死，但没有杀死Service，然后重启应用，启动过程中CPU占用过高，而出现的ANR

### 哪些场景会出现ANR
- 场景1：低内存anr：查看CPU，手机内存低导致anr时，一般以下两个进程CPU使用率高，手机会出现大量应用anr：
~~~ java
35% 90/mmcqd/0: 0% user + 35% kernel
18% 49/kswapd0: 0% user + 18% kernel
~~~
- 场景2： 某一个进程的cpu使用率高，导致当前进程无法获取资源，可以根据trace查看是哪个进程在工作；
- 场景3：主线程卡死： 1）主线程等待其他线程锁waiting to lock； 2） 主线程繁忙；3）主线程对应的native调用栈卡住；
- 场景4：Activity resume、restart或pause太慢，导致无焦点窗口问题，通常会有黑屏白屏等现象；
- 场景5：省电精灵智能省电模式冻结后台进程，resume activity时，没有及时解冻，产生无焦点窗口anr；
- 场景6：其它进程Binder线程耗尽（ProcessState.cpp中指定最大binder线程数，如maxThreads:32），导致当前进程卡在IPCThreadState::talkWithDriver；
- 场景7：Input dispatching timed out：接收输入事件的窗口出现异常，丢失焦点，伴随冻屏现象
- 场景8：monkey启动大量对象GC耗时久导致anr。从dumpsys中可以看到启动很多window，从log中看GC耗时很久；
- 场景9：抓取整机log导致anr。此类问题中，adbd、bugreport、chargelogcat等进程CPU使用率高；
- 场景10：Framework缺陷导致无焦点窗口问题，activity已经正常resume了（am_on_resume_called），dumpsys中mFocusedWindow为null。

