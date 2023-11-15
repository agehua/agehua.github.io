---
layout: post
title: Matrix TraceCanary源码分析
category: accumulation
tags:
  - ConstraintLayout
keywords: ConstraintLayout
banner: https://cdn.conorlee.top/Haystacks%20in%20Provence.jpg
thumbnail: https://cdn.conorlee.top/Haystacks%20in%20Provence.jpg
toc: true
---

本文主要介绍Matrix的Trace部分，主要涉及帧率、ANR、慢函数、启动耗时的检测逻辑。

<!--more-->
### Trace Canary主要特性

- 编译期动态修改字节码, 高性能记录执行耗时与调用堆栈
- 准确的定位到发生卡顿的函数，提供执行堆栈、执行耗时、执行次数等信息，帮助快速解决卡顿问题
- 自动涵盖卡顿、启动耗时、页面切换、慢函数检测等多个流畅性指标

Tracer模块主要结构：
![Trace Canary主要特性](/images/blogimages/2023/matrix-trace-canary.png)


其中：
- FrameTracer负责帧率检测
- AnrTracer负责ANR问题检测
- EvilMethodTracer负责检测慢函数
- StartupTracer负责应用启动耗时检测

#### 帧率检测
FrameTracer部分主要做帧率、掉帧、帧耗时等检测，具体实现逻辑在FrameTracer和UIThreadMonitor。

Demo中的入口在TestFpsActivity，onCreate()中通过FrameTracer.onStartTrace()开启检测，页面退出时通过FrameTracer.onCloseTrace()结束检测，并移除监控回调。

从使用侧开始，看看帧率的计算逻辑。

TestFpsActivity：
~~~ java
sample.tencent.matrix.trace.TestFpsActivity

protected void onCreate(@Nullable Bundle savedInstanceState) {
	// 启动帧率检测，使用FrameTrace计算FPS
	Matrix.with().getPluginByClass(TracePlugin.class).getFrameTracer().onStartTrace();
	// 添加帧率回调
	Matrix.with().getPluginByClass(TracePlugin.class).getFrameTracer().addListener(mDoFrameListener);
}

protected void onDestroy() {
	super.onDestroy();
	// 移除帧率回调
	Matrix.with().getPluginByClass(TracePlugin.class).getFrameTracer().removeListener(mDoFrameListener);
	// 关闭帧率检测
	Matrix.with().getPluginByClass(TracePlugin.class).getFrameTracer().onCloseTrace();
}
~~~
看下TestFpsActivity的FPS监控回调部分：
~~~ java
sample.tencent.matrix.trace.TestFpsActivity

// 异步线程
private static HandlerThread sHandlerThread = new HandlerThread("test");

// 帧率检测回调
private IDoFrameListener mDoFrameListener = new IDoFrameListener(new Executor() {
        Handler handler = new Handler(sHandlerThread.getLooper());

        @Override
        public void execute(Runnable command) {
            //将回调方法放到异步线程执行
            handler.post(command);
        }
    }) {
        @Override
        public void doFrameAsync(String visibleScene, long taskCost, long frameCostMs, int droppedFrames, boolean isContainsFrame) {
            super.doFrameAsync(visibleScene, taskCost, frameCostMs, droppedFrames, isContainsFrame);
            // 计算总的掉帧数
            count += droppedFrames;
        }
    };
~~~

回调接口IDoFrameListener支持将同步和异步回调方法：doFrameSync()和doFrameAsync，需要使用侧传入一个Executor。

解释下IDoFrameListener.doFrameAsync()的参数含义：

- visibleScene：场景名称，默认是Activity的类名
- taskCost：主线程每一帧耗时
- frameCostMs：帧率检测时用不到
- droppedFrames：掉帧数
- isContainsFrame：是否包含一帧

> 这里提一下visibleScene的获取逻辑：Matrix初始化时，AppActiveMatrixDelegate.controller()内部通过Application. registerActivityLifecycleCallbacks()监听Activity生命周期。当Activity start时，将activity.getClass().getName()做为visibleScene。FrameTracer再通过AppMethodBeat.getVisibleScene()获取visibleScene。

继续看下回调方法doFrameAsync()的上游逻辑。
~~~ java
FrameTracer.notifyListener()：
IDoFrameListener.doFrameAsync()是在FrameTracer.notifyListener()中调用的：

com.tencent.matrix.trace.tracer.FrameTracer

    private void notifyListener(final String visibleScene, final long taskCostMs, final long frameCostMs, final boolean isContainsFrame) {
        long start = System.currentTimeMillis();
        try {
            synchronized (listeners) {
                for (final IDoFrameListener listener : listeners) {
                    if (config.isDevEnv()) {
                        listener.time = SystemClock.uptimeMillis();
                    }
                    // 计算掉帧数
                    final int dropFrame = (int) (taskCostMs / frameIntervalMs);

                    listener.doFrameSync(visibleScene, taskCostMs, frameCostMs, dropFrame, isContainsFrame);
                    if (null != listener.getExecutor()) {
                    	//通过TestFpsActivity传入的Executor，将回调放到异步
                        listener.getExecutor().execute(new Runnable() {
                            @Override
                            public void run() {
                            	// 回调给TestFpsActivity
                                listener.doFrameAsync(visibleScene, taskCostMs, frameCostMs, dropFrame, isContainsFrame);
                            }
                        });
                    }
                }
            }
        } finally {
            long cost = System.currentTimeMillis() - start;
            // debug模式下，回调doFrameAsync中执行超过17ms，log警告
            if (config.isDebug() && cost > frameIntervalMs) {
                MatrixLog.w(TAG, "[notifyListener] warm! maybe do heavy work in doFrameSync! size:%s cost:%sms", listeners.size(), cost);
            }
        }
    }
~~~
从上面代码可以看出掉帧数dropFrame就是taskCostMs/frameIntervalMs。

- taskCostMs：一帧的执行耗时
- frameIntervalMs：默认是17ms，计算逻辑是Choreographer.mFrameIntervalNanos+1。
其中，Choreographer.mFrameIntervalNanos是屏幕刷新时间间隔，默认16ms。计算代码如下：
~~~ java
com.tencent.matrix.trace.tracer.FrameTracer

this.frameIntervalMs = TimeUnit.MILLISECONDS.convert(UIThreadMonitor.getMonitor().getFrameIntervalNanos(), TimeUnit.NANOSECONDS) + 1;
com.tencent.matrix.trace.core.UIThreadMonitor

choreographer = Choreographer.getInstance();

// 通过反射获取Choreographer的mFrameIntervalNanos字段，默认是16ms
frameIntervalNanos = reflectObject(choreographer, "mFrameIntervalNanos");
~~~
上面计算掉帧的方式：taskCostMs/17，其实并不是非常严谨，比如taskCostMs=33时，应该丢了两帧，但是计算结果是1。

上面提到的`FrameTracer.notifyListener()`由FrameTracer.doFrame()触发，看下FrameTracer.doFrame()：


~~~ java
FrameTracer.doFrame():

com.tencent.matrix.trace.tracer.FrameTracer

@Override
    public void doFrame(String focusedActivityName, long start, long end, long frameCostMs, long inputCostNs, long animationCostNs, long traversalCostNs) {
        if (isForeground()) {
            // IDoFrameListener.doFrameAsync()中的参数taskCostMs即为end - start。
            notifyListener(focusedActivityName, end - start, frameCostMs, frameCostMs >= 0);
        }
    }
~~~
这个方法没有做太多事情，主要是计算掉帧数用的taskCostMs是end - start的结果。

FrameTracer.doFrame()由UIThreadMonitor.dispatchEnd()触发，看下后者的实现。


~~~ java
UIThreadMonitor.dispatchEnd()

com.tencent.matrix.trace.core.UIThreadMonitor

    private void dispatchEnd() {
        if (isBelongFrame) {
            doFrameEnd(token);
        }

        // 对应每一帧的开始时间，在dispatchBegin()中赋值
        long start = token;
        // 对应每一帧的结束时间
        long end = SystemClock.uptimeMillis();

        synchronized (observers) {
            for (LooperObserver observer : observers) {
                if (observer.isDispatchBegin()) {
                	// isBelongFrame ? end - start : 0 - frameCost, 如果是帧耗时计算，则frameCost为0，否则和taskCost一样
                	// queueCost数组，存储对应Choreographer中三类task(input/animation/traversal)的耗时
                    observer.doFrame(AppMethodBeat.getVisibleScene(), token, SystemClock.uptimeMillis(), isBelongFrame ? end - start : 0, queueCost[CALLBACK_INPUT], queueCost[CALLBACK_ANIMATION], queueCost[CALLBACK_TRAVERSAL]);
                }
            }
        }

        // 计算结束时间
        dispatchTimeMs[3] = SystemClock.currentThreadTimeMillis();
        dispatchTimeMs[1] = SystemClock.uptimeMillis();

        AppMethodBeat.o(AppMethodBeat.METHOD_ID_DISPATCH);

        synchronized (observers) {
            for (LooperObserver observer : observers) {
                if (observer.isDispatchBegin()) {
                    observer.dispatchEnd(dispatchTimeMs[0], dispatchTimeMs[2], dispatchTimeMs[1], dispatchTimeMs[3], token, isBelongFrame);
                }
            }
        }
    }
~~~
这部分内容主要做了这几件事情：

获取每一帧耗时的开始、结束时间戳，即start、end，将差值通过observer.doFrame()传给消费方
执行observer.doFrame()，该方法主要消费方是AnrTracer、EvilMethodTracer、FrameTracer
赋值dispatchTimeMs[1]，dispatchTimeMs[3]，其中dispatchTimeMs数组含义后面会提到
执行observer.dispatchEnd()，该方法主要消费方是AnrTracer、EvilMethodTracer
看完了UIThreadMonitor.dispatchEnd()，再看下UIThreadMonitor.dispatchBegin()：


~~~ java
com.tencent.matrix.trace.core.UIThreadMonitor

private void dispatchBegin() {
        // 对dispatchTimeMs[0]、dispatchTimeMs[2]赋值
        token = dispatchTimeMs[0] = SystemClock.uptimeMillis();
        dispatchTimeMs[2] = SystemClock.currentThreadTimeMillis();
        AppMethodBeat.i(AppMethodBeat.METHOD_ID_DISPATCH);

        //每一帧开始执行时的回调
        synchronized (observers) {
            for (LooperObserver observer : observers) {
                if (!observer.isDispatchBegin()) {
                    observer.dispatchBegin(dispatchTimeMs[0], dispatchTimeMs[2], token);
                }
            }
        }
    }
~~~
> 这里简单说下SystemClock.uptimeMillis()和SystemClock.currentThreadTimeMillis()的区别，详细内容可以看官方文档：
- SystemClock.uptimeMillis()：记录从开机到现在的时长，系统深睡眠（CPU睡眠、黑屏、系统等待外部输入对其唤醒）的时间不算在内，且不受系统设置时间调整的影响。是大多数时间间隔计算方法的基础，比如Thread.sleep(long)、Object.wait(long)、System.nanoTime()
- SystemClock.currentThreadTimeMillis()：这个比较容易理解，当前线程的活动时长（线程处于running状态）

LooperObserver.dispatchEnd()
解释下LooperObserver.dispatchEnd()的参数含义，后续对理解AnrTracer、EvilMethodTracer的实现有帮助：
~~~ java
com.tencent.matrix.trace.listeners.LooperObserver

public void dispatchEnd(long beginMs, long cpuBeginMs, long endMs, long cpuEndMs, long token, boolean isBelongFrame)
~~~

- beginMs：主线程一帧任务的开始时间点，对应dispatchTimeMs[0]，在UIThreadMonitor.dispatchBegin()中赋值
- cpuBeginMs：当前线程活动期间，方法开始执行时间点，对应dispatchTimeMs[2]
- endMs：主线程一帧方法结束执行时间点，对应dispatchTimeMs[1]，在UIThreadMonitor.dispatchEnd()中赋值。所以，一帧耗时就是endMs-beginMs。
- cpuEndMs：当前线程活动期间，方法解释执行时间点，对应dispatchTimeMs[3]
- token：等同于beginMs
- isBelongFrame：可简单理解为是否属于主线程任务。在Choreographer处理input任务时执行UIThreadMonitor.run()，内部将- - isBelongFrame置为true，这块逻辑可以看下UIThreadMonitor.onStart()

上面提到，一帧任务执行结束时会执行UIThreadMonitor.dispatchEnd()，该方法在LooperMonitor注册的监听器中执行的。看下这块逻辑：

~~~ java
com.tencent.matrix.trace.core.UIThreadMonitor

LooperMonitor.register(new LooperMonitor.LooperDispatchListener() {
            @Override
            public boolean isValid() {
                return isAlive;
            }

            @Override
            public void dispatchStart() {
                super.dispatchStart();
                UIThreadMonitor.this.dispatchBegin();
            }

            @Override
            public void dispatchEnd() {
                super.dispatchEnd();
                UIThreadMonitor.this.dispatchEnd();
            }

        });
~~~
UIThreadMonitor向LooperMonitor注册了监听器，用于监听每一帧的开始和结束。

其中，LooperDispatchListener的dispatchStart()和dispatchEnd()都是LooperMonitor.dispatch()执行的。由isBegin决定执行dispatchStart()还是dispatchEnd()。看下LooperMonitor.dispatch()：
~~~ java
LooperMonitor.dispatch():
com.tencent.matrix.trace.core.LooperMonitor

private void dispatch(boolean isBegin, String log) {
        for (LooperDispatchListener listener : listeners) {
            if (listener.isValid()) {
                if (isBegin) {
                    if (!listener.isHasDispatchStart) {
                        // 内部回调dispatchStart()
                        listener.onDispatchStart(log);
                    }
                } else {
                    if (listener.isHasDispatchStart) {
                        // 内部回调dispatchEnd()
                        listener.onDispatchEnd(log);
                    }
                }
            } else if (!isBegin && listener.isHasDispatchStart) {
                listener.dispatchEnd();
            }
        }

    }
~~~
isBegin的逻辑看下面代码：
~~~ java
LooperMonitor.LooperPrinter:
com.tencent.matrix.trace.core.LooperMonitor

class LooperPrinter implements Printer {
       
        @Override
        public void println(String x) {
            ...

            if (!isHasChecked) {
            	// 根据参数x内容，判断是开始还是结束，即isBegin
                isValid = x.charAt(0) == '>' || x.charAt(0) == '<';
                isHasChecked = true;
                if (!isValid) {
                    MatrixLog.e(TAG, "[println] Printer is inValid! x:%s", x);
                }
            }

            if (isValid) {
                // x.charAt(0) == '>'时，isBegin为true
                dispatch(x.charAt(0) == '>', x);
            }

        }
    }
~~~
即，入参字符串x以>开头时，isBegin为true。

上面逻辑是自定义LooperPrinter的实现方法。在LooperMonitor初始化时，会向main looper注册一个LooperPrinter：
~~~ java
com.tencent.matrix.trace.core.LooperMonitor

public class LooperMonitor {
	private LooperPrinter printer;
	//主线程looper
	looper = Looper.getMainLooper()

	// 将前面提到的自定义LooperPrinter塞给主线程looper
	looper.setMessageLogging(printer = new LooperPrinter(originPrinter));
}
~~~
上面代码主要是把自定义LooperPrinter塞给主线程looper。对Looper比较熟悉的同学其实已经有眉目了，我们回顾下looper的逻辑：
~~~ java
Looper.loop()
android.os.Looper.java

public static void loop() {
        final Looper me = myLooper();
        if (me == null) {
            throw new RuntimeException("No Looper; Looper.prepare() wasn't called on this thread.");
        }
        final MessageQueue queue = me.mQueue;

        ...

        for (;;) {
            Message msg = queue.next(); // might block
            if (msg == null) {
                // No message indicates that the message queue is quitting.
                return;
            }

            // This must be in a local variable, in case a UI event sets the logger
            // logging对象外部自定义，传入一个自定义Printer实现每一帧的监听计算
            final Printer logging = me.mLogging;
            if (logging != null) {
                // 每一帧开始时打印的log
                logging.println(">>>>> Dispatching to " + msg.target + " " +
                        msg.callback + ": " + msg.what);
            }
            ...
            try {
            		// 消息执行
            		msg.target.dispatchMessage(msg);
                dispatchEnd = needEndTime ? SystemClock.uptimeMillis() : 0;
            } finally {
                if (traceTag != 0) {
                    Trace.traceEnd(traceTag);
                }
            }
            ...
            if (logging != null) {
            		// 每一帧结束时打印的log
            		logging.println("<<<<< Finished to " + msg.target + " " + msg.callback);
            }
            ...
        }
    }
~~~
很多APM计算帧率都采用了这个逻辑，我在Android图形渲染之Choreographer原理 最后也提到使用“Looper & Printer”计算帧率。

Looper在每个消息消费前后msg.target.dispatchMessage(msg)，会通过一个Printer对象分别打印`>>>>> Dispatching to`和`<<<<< Finished to`两个日志。通过**Looper.setMessageLogging()**可以设置我们自定义的Printer，通过监听主线程方法执行前后的两个日志字符串，就可以计算一帧的耗时。

前面LooperMonitor中就通过Looper.setMessageLogging()将自定义的Printer传给了Looper，所以可以拦截每个消息执行前后的日志。

到这里，每一帧的监听、掉帧逻辑及帧率计算方式就讲完了。

总结：
回顾下整体代码的执行过程：

- Looper.loop()		//主线程消息执行也是looper机制
- LooperMonitor.LooperPrinter.println()		//消息执行前后会打印两个日志
- LooperMonitor.dispatch()	//拦截到两个日志后进行分发
- UIThreadMonitor.dispatchBegin()/dispatchEnd()	//记录相关时间节点，分发每一帧执行前后的回调方法
- FrameTracer.doFrame()
- FrameTracer.notifyListener()
- TestFpsActivity.IDoFrameListener.doFrameSync()/doFrameAsync()	//业务侧获取帧率回调
总体来说，帧率检测使用了“Looper & Printer”方式。Printer的日志字符串对应主线程每一帧方法执行的开始和结束，通过给主线程Looper设置自定义Printer，即可拦截日志。根据日志内容即可得知每一帧执行的前后节点，进而记录每一帧的开始、结束时间戳，从而可以计算每一帧的耗时、帧率、掉帧等核心指标。

#### ANR检测
ANR检测的主要实现在AnrTracer。

demo中，将TestTraceMainActivity.testANR()内部执行耗时改大一点（大于5000ms），比如改到7800，就能看到AS中输出如下ANR警告log：



看下AnrTracer如何统计ANR，收集上述log信息：
~~~ java
AnrTracer：
com.tencent.matrix.trace.tracer.AnrTracer

@Override
    public void dispatchBegin(long beginMs, long cpuBeginMs, long token) {
        super.dispatchBegin(beginMs, cpuBeginMs, token);
        // 构造AnrTask
        anrTask = new AnrHandleTask(AppMethodBeat.getInstance().maskIndex("AnrTracer#dispatchBegin"), token);
        
        // 延迟5s发消息，Constants.DEFAULT_ANR = 5 * 1000
        // token是方法开始执行的时间
        // (SystemClock.uptimeMillis() - token)：校正方法的开始时间
        anrHandler.postDelayed(anrTask, Constants.DEFAULT_ANR - (SystemClock.uptimeMillis() - token));
    }

    @Override
    public void dispatchEnd(long beginMs, long cpuBeginMs, long endMs, long cpuEndMs, long token, boolean isBelongFrame) {
        super.dispatchEnd(beginMs, cpuBeginMs, endMs, cpuEndMs, token, isBelongFrame);
        // 移除anrTask
        if (null != anrTask) {
            anrTask.getBeginRecord().release();
            anrHandler.removeCallbacks(anrTask);
        }
    }
~~~
前面讲FrameTracer时提到过，每一帧执行前后分别回调Tracer.dispatchBegin()和Tracer.dispatchEnd()。

上面代码在dispatchBegin()内部发送了一个延迟消息执行AnrTask，延迟时间约为5s（Constants.DEFAULT_ANR = 5 * 1000），在dispatchEnd()移除对应的任务。所以，如果主线程一帧执行任务超过5s，AnrTask就会执行。

要注意的一点是，ANR log中，may be happen ANR(5007ms)!，这里的5007ms时间并不是一帧任务的耗时（因为我们把测试方法耗时改成了7800ms），而是AnrTracer中AnrTask的消息延迟时间。

AnrTask：
看下AnrTask的逻辑：
~~~ java
com.tencent.matrix.trace.tracer.AnrTracer.AnrHandleTask

@Override
public void run() {
    long curTime = SystemClock.uptimeMillis();
    boolean isForeground = isForeground();
    
    // process 进程信息
    int[] processStat = Utils.getProcessPriority(Process.myPid());
    long[] data = AppMethodBeat.getInstance().copyData(beginRecord);
    beginRecord.release();
    // 业务场景，FrameTracer中提到过
    String scene = AppMethodBeat.getVisibleScene();

    // memory 内存信息，其中VmSize从“/proc/{PID}/status”文件中获取
    long[] memoryInfo = dumpMemory();

    // Thread state
    Thread.State status = Looper.getMainLooper().getThread().getState();
    
    // 线程调用堆栈
    StackTraceElement[] stackTrace = Looper.getMainLooper().getThread().getStackTrace();
    // 堆栈裁剪
    String dumpStack = Utils.getStack(stackTrace, "|*\t\t", 12);

    // frame
    UIThreadMonitor monitor = UIThreadMonitor.getMonitor();
    // 获取input/animation/traversal三类任务耗时
    long inputCost = monitor.getQueueCost(UIThreadMonitor.CALLBACK_INPUT, token);
    long animationCost = monitor.getQueueCost(UIThreadMonitor.CALLBACK_ANIMATION, token);
    long traversalCost = monitor.getQueueCost(UIThreadMonitor.CALLBACK_TRAVERSAL, token);

    // trace
    ...

    StringBuilder reportBuilder = new StringBuilder();
    StringBuilder logcatBuilder = new StringBuilder();
    
    // 获取ANR时长
    long stackCost = Math.max(Constants.DEFAULT_ANR, TraceDataUtils.stackToString(stack, reportBuilder, logcatBuilder));

    // stackKey
    String stackKey = TraceDataUtils.getTreeKey(stack, stackCost);
    
    // 打印ANR日志
    MatrixLog.w(TAG, "%s \npostTime:%s curTime:%s",
            printAnr(scene, processStat, memoryInfo, status, logcatBuilder, isForeground, stack.size(), stackKey, dumpStack, inputCost, animationCost, traversalCost, stackCost), token, curTime); // for logcat

    if (stackCost >= Constants.DEFAULT_ANR_INVALID) {
        MatrixLog.w(TAG, "The checked anr task was not executed on time. "
                + "The possible reason is that the current process has a low priority. just pass this report");
        return;
    }
    
    // report 启动IssuesListActivity页面，展示ANR信息
    ...

}
~~~
主要内容都加了注释，可以参考下。

总结：
ANR的检测逻辑：在主线程一帧任务开始前发送（5s）的延迟消息，任务结束时移除该消息。如果主线程一帧任务执行时间超过了消息的延迟时间（认为这一帧导致了ANR），延迟消息将会执行，触发ANR信息收集任务

延迟消息（AnrTask）在异步线程执行，收集进程、调用堆栈、内存使用、trace信息，后续将信息log出来，并启动IssuesListActivity展示ANR结果。

慢函数检测
将TestTraceMainActivity.testANR()实现耗时改成7800ms，AS会输出如下慢函数log。从log中可以看出发生了Jankiness，方法耗时7804ms，和我们的修改基本吻合。



下面看下慢函数的实现逻辑，主要代码在EvilMethodTracer。
~~~ java
EvilMethodTracer：
com.tencent.matrix.trace.tracer.EvilMethodTracer

@Override
    public void dispatchEnd(long beginMs, long cpuBeginMs, long endMs, long cpuEndMs, long token, boolean isBelongFrame) {
        super.dispatchEnd(beginMs, cpuBeginMs, endMs, cpuEndMs, token, isBelongFrame);
        long start = config.isDevEnv() ? System.currentTimeMillis() : 0;
        try {
            long dispatchCost = endMs - beginMs;
            // 一帧耗时超过evilThresholdMs（默认700ms），执行慢函数计算逻辑
            if (dispatchCost >= evilThresholdMs) {
                long[] data = AppMethodBeat.getInstance().copyData(indexRecord);
                long[] queueCosts = new long[3];
                System.arraycopy(queueTypeCosts, 0, queueCosts, 0, 3);
                String scene = AppMethodBeat.getVisibleScene();
                // 异步线程执行AnalyseTask
                MatrixHandlerThread.getDefaultHandler().post(new AnalyseTask(isForeground(), scene, data, queueCosts, cpuEndMs - cpuBeginMs, endMs - beginMs, endMs));
            }
        } finally {
            ...
        }
    }
~~~
主要逻辑在dispatchEnd()里面。从上面代码可以看出，当主线程一帧内耗时超过evilThresholdMs（默认700ms，可以通过TraceConfig配置），执行慢函数的警告，打印上面提到的log。

其中，慢函数的耗时，就是上面代码中的endMs - beginMs，这个逻辑在“帧率检测 - UIThreadMonitor.dispatchEnd()”中有具体讲到。

具体慢函数打印的log逻辑如下：
~~~ java
AnalyseTask:
com.tencent.matrix.trace.tracer.EvilMethodTracer.AnalyseTask

private class AnalyseTask implements Runnable {
        ...

        void analyse() {

            // process 进程信息
            int[] processStat = Utils.getProcessPriority(Process.myPid());
            String usage = Utils.calculateCpuUsage(cpuCost, cost);
            LinkedList<MethodItem> stack = new LinkedList();
            ...

            StringBuilder reportBuilder = new StringBuilder();
            StringBuilder logcatBuilder = new StringBuilder();
            long stackCost = Math.max(cost, TraceDataUtils.stackToString(stack, reportBuilder, logcatBuilder));
            String stackKey = TraceDataUtils.getTreeKey(stack, stackCost);

					// 和AnrTracer一样，打印log，启动IssuesListActivity页面，展示EvilMethod信息
            MatrixLog.w(TAG, "%s", printEvil(scene, processStat, isForeground, logcatBuilder, stack.size(), stackKey, usage, queueCost[0], queueCost[1], queueCost[2], cost)); // for logcat

            ...
        }
    }
~~~
其中，AnalyseTask的逻辑和AnrTask类似，只是log中的部分内容不一样，就不具体展开了。

总结：
慢函数的检测逻辑比较简单，在主线程一帧执行结束时，判断帧耗时是否大于预先设置的慢函数阈值，超过该阈值，即认为慢函数发生。


