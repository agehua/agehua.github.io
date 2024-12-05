---
layout: post
title: LeakCanary源码分析及借鉴
category: accumulation
tags:
    - LeakCanary
    - Android Source Code
keywords: LeakCanary, SourceCode
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Field%20of%20Spring%20Wheat%20at%20Sunrise.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Field%20of%20Spring%20Wheat%20at%20Sunrise.jpg
toc: true
---

### 煤矿中的金丝雀
[LeakCanary](https://github.com/square/leakcanary)是一个帮我们在Android或Java项目开发时检测内存泄漏的库。

本文以LeakCanary-1.5.1版本为例，分析LeakCanary原理，以及借鉴其中的方法。
<!--more-->
关于它的图标有一个故事：
![](/images/blogimages/2019/leak_canary_icon.png)

![](/images/blogimages/2019/canary_in_mine.png)

> 17世纪，英国矿井工人发现，金丝雀对瓦斯这种气体十分敏感。空气中哪怕有极其微量的瓦斯，金丝雀也会停止歌唱；而当瓦斯含量超过一定限度时，虽然鲁钝的人类毫无察觉，金丝雀却早已毒发身亡。当时在采矿设备相对简陋的条件下，工人们每次下井都会带上一只金丝雀作为“瓦斯检测指标”，以便在危险状况下紧急撤离。

### 内存泄漏检测的基本原理
**给弱引用关联一个引用队列，当弱引用持有内容被gc回收后，`该弱引用`会被添加到关联的引用队列中。**

试试下面的代码，注释掉`user=null;`和不注释掉，看看有什么不同
~~~ Java
public class RefTest {
    public static void main(String[] args) {
        //user为强引用
        User user = new User("张三", 18);
        //创建弱引用并关联引用队列
        ReferenceQueue<User> queue = new ReferenceQueue<>();
        WeakReference<User> weakReference = new WeakReference<User>(user,queue);
        //置空强引用，触发GC
        user=null;
        Runtime.getRuntime().gc();
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        //强制系统回收已经没有强引用的对象
        System.runFinalization();

        WeakReference pollRef=null;
        //弹出对列的引弱用
        while ((pollRef = (WeakReference) queue.poll()) != null) {
          System.out.println("pollRef的内存地址:"+pollRef.toString());
          System.out.println("pollRef等于weakReference？:"+weakReference.equals(pollRef));
        }
    }
}
~~~


#### Java垃圾回收(GC)

- 在Java中垃圾判断方法是 可达性分析算法，这个算法的基本思路是通过一系列的"GC Root"的对象作为起始点，从这些节点开始向下搜索，搜索所走过的路径成为引用链，当一个对象到GC Root没有任何引用链相连时，则证明此对象是不可用的。
- GC Root的对象包括以下几种：
    - 1、虚拟机栈中引用的对象。
    - 2、方法区中类静态属性引用的对象。
    - 3、方法区中常量引用的对象。
    - 4、本地方法栈中JNI引用的对象。
- 就算一个对象，通过可达性分析算法分析后，发现其是『不可达』的，也并不是非回收不可的。一般情况下，要宣告一个对象死亡，至少要经过两次标记过程：
    - 1、经过可达性分析后，一个对象并没有与GC Root关联的引用链，将会被第一次标记和筛选。筛选条件是此对象有没有必要执行finalize()方法。如果对象没有覆盖finalize()方法，或者已经执行过了。那就认为他可以回收了。如果有必要执行finalize()方法，那么将会把这个对象放置到F-Queue的队列中，等待执行。
    - 2、虚拟机会建立一个低优先级的Finalizer线程执行F-Queue里面的对象的finalize()方法。如果对象在finalize()方法中可以『拯救』自己，那么将不会被回收，否则，他将被移入一个即将被回收的ReferenceQueue。


### LeakCanary使用
LeakCanary在Application初始化，代码如下:

~~~ Java
 public class BaseApplication extends Application {
      @Override
      public void onCreate() {
          super.onCreate();
          //检查当前进程是否在HeapAnalyzerService所属进程
          if (LeakCanary.isInAnalyzerProcess(this)) {
            return;
          }
        //安装泄露检测
        LeakCanary.install(this);
      }
  }
~~~

编译成功后，可以在debug状态下的AndroidManifest.xml文件中，看到LeakCanary注册的组件。
~~~ Java
<!-- 这个是LeakCanary分析泄露的Service -->
  <service
      android:name="com.squareup.leakcanary.internal.HeapAnalyzerService"
      android:enabled="false"
      android:process=":leakcanary" />
  <!-- 这个是LeakCanary展示泄露的Service -->
  <service
      android:name="com.squareup.leakcanary.DisplayLeakService"
      android:enabled="false"/>
  <activity
      android:name="com.squareup.leakcanary.internal.DisplayLeakActivity"
      android:enabled="false"
      android:icon="@mipmap/leak_canary_icon"
      android:label="@string/leak_canary_display_activity_label"
      android:taskAffinity="com.squareup.leakcanary.com.pengguanming.studydemo"
      android:theme="@style/leak_canary_LeakCanary.Base" >
      <intent-filter>
          <action android:name="android.intent.action.MAIN" />
          <category android:name="android.intent.category.LAUNCHER" />
      </intent-filter>
  </activity>
~~~
其中DisplayLeakActivity被设置为Launcher，并设置了金丝雀的icon，这也就是为什么使用LeakCanary会在桌面上生成icon入口的原因。但是，这里要注意DisplayLeakActivity的enable属性被设置为false了，默认在桌面上是不会显示入口的。


#### 监听Activity销毁的方法

返回来看 LeakCanary.install(this)方法
~~~ Java
/**
   * Creates a {@link RefWatcher} that works out of the box, and starts watching activity
   * references (on ICS+).
   */
public static RefWatcher install(Application application) {
    return refWatcher(application).listenerServiceClass(DisplayLeakService.class)
        .excludedRefs(AndroidExcludedRefs.createAppDefaults().build())
        .buildAndInstall();
}

/** Builder to create a customized {@link RefWatcher} with appropriate Android defaults. */
public static AndroidRefWatcherBuilder refWatcher(Context context) {
    return new AndroidRefWatcherBuilder(context);
}
~~~
install方法通过AndroidRefWatcherBuilder，构造了一个RefWatcher对象，再看buildAndInstall()方法：
~~~ Java
public RefWatcher buildAndInstall() {
    RefWatcher refWatcher = build();
    if (refWatcher != DISABLED) {
        LeakCanary.enableDisplayLeakActivity(context);
        ActivityRefWatcher.install((Application) context, refWatcher);
    }
    return refWatcher;
}
~~~

ActivityRefWatcher.install()调用了watchActivities()方法。
~~~ Java
public static void install(Application application, RefWatcher refWatcher) {
    new ActivityRefWatcher(application, refWatcher).watchActivities();
}
~~~
watchActivities()通过传过来的application对象注册了一个registerActivityLifecycleCallbacks。
~~~ Java
public void watchActivities() {
    // Make sure you don't get installed twice.
    stopWatchingActivities();
    application.registerActivityLifecycleCallbacks(lifecycleCallbacks);
}
~~~

这个监听只监听了onActivityDestroyed()方法
~~~ Java
private final Application.ActivityLifecycleCallbacks lifecycleCallbacks =
  new ActivityLifecycleCallbacksAdapter() {
    @Override public void onActivityDestroyed(Activity activity) {
        refWatcher.watch(activity);
    }
};
~~~
上面就是LeakCanary注册监听Activity onDestroy()生命周期的过程

#### 检测对象弱引用关联引用队列
前面提到，弱引用关联一个`引用队列`，当弱引用保存的引用被释放掉后，弱引用就会进入到`引用队列`中，LeakCanary当然也是用这种方式，不过它更聪明。

进入到refWatcher.watch()方法中：

~~~ Java
public void watch(Object watchedReference, String referenceName) {
    if (this == DISABLED) {
      return;
    }
    // 这里的 watchedReference 就是一个 Activity
    checkNotNull(watchedReference, "watchedReference");
    checkNotNull(referenceName, "referenceName");
    final long watchStartNanoTime = System.nanoTime();
    String key = UUID.randomUUID().toString();
    // 为每个 watchedReference 生成一个随机key，将key添加到retainedKeys中
    retainedKeys.add(key);
    // 每次创建一个弱引用，并关联一个引用队列
    final KeyedWeakReference reference =
        new KeyedWeakReference(watchedReference, key, referenceName, queue);

    ensureGoneAsync(watchStartNanoTime, reference);
}
~~~

上面的retainedKeys是一个支持并发访问的 CopyOnWriteArraySet 对象。
这个 KeyedWeakReference 里的key值就是LeakCanary聪明的地方了
~~~ Java
final class KeyedWeakReference extends WeakReference<Object> {
    public final String key;
    public final String name;

    KeyedWeakReference(Object referent, String key, String name,
          ReferenceQueue<Object> referenceQueue) {
        super(checkNotNull(referent, "referent"), checkNotNull(referenceQueue, "referenceQueue"));
        // 比普通 WeakReference 多了两个参数，其中的 key 最为关键
        this.key = checkNotNull(key, "key");
        this.name = checkNotNull(name, "name");
    }
}
~~~

为什么要多保存一个key值呢？它在哪里用到了呢？继续往下看：
~~~ Java
private void ensureGoneAsync(final long watchStartNanoTime, final KeyedWeakReference reference) {
    // 这里逻辑先不考虑，直接看 ensureGone()
    watchExecutor.execute(new Retryable() {
      @Override public Retryable.Result run() {
        return ensureGone(reference, watchStartNanoTime);
      }
    });
}
~~~

~~~ Java
@SuppressWarnings("ReferenceEquality") // Explicitly checking for named null.
Retryable.Result ensureGone(final KeyedWeakReference reference, final long watchStartNanoTime) {
    // 计算watch方法到gc垃圾回收的时长
    long gcStartNanoTime = System.nanoTime();
    long watchDurationMs = NANOSECONDS.toMillis(gcStartNanoTime - watchStartNanoTime);
    // 尝试移除已经到达引用队列的弱引用
    removeWeaklyReachableReferences();

    if (debuggerControl.isDebuggerAttached()) {
        // The debugger can create false leaks.
        return RETRY;
    }
    if (gone(reference)) {
        return DONE;
    }
    // 手动gc
    gcTrigger.runGc();
    // gc后，再次移除引用队列中的弱引用
    removeWeaklyReachableReferences();
    if (!gone(reference)) {
        // 弱引用没有被移除，说明内存泄漏了
        long startDumpHeap = System.nanoTime();
        long gcDurationMs = NANOSECONDS.toMillis(startDumpHeap - gcStartNanoTime);
        // 确定泄漏后，生成prof文件
        File heapDumpFile = heapDumper.dumpHeap();
        if (heapDumpFile == RETRY_LATER) {
            // Could not dump the heap.
            return RETRY;
        }
        long heapDumpDurationMs = NANOSECONDS.toMillis(System.nanoTime() - startDumpHeap);
        heapdumpListener.analyze(
            new HeapDump(heapDumpFile, reference.key, reference.name, excludedRefs, watchDurationMs,
                gcDurationMs, heapDumpDurationMs));
    }
    return DONE;
}

private boolean gone(KeyedWeakReference reference) {
    return !retainedKeys.contains(reference.key);
}

private void removeWeaklyReachableReferences() {
    // WeakReferences are enqueued as soon as the object to which they point to becomes weakly
    // reachable. This is before finalization or garbage collection has actually happened.
    KeyedWeakReference ref;
    while ((ref = (KeyedWeakReference) queue.poll()) != null) {
        retainedKeys.remove(ref.key);
    }
}
~~~
在removeWeaklyReachableReferences()方法中，将queue队列里所有弱引用弹栈。如果出栈了，就移除掉retainedKeys中保存的key。

在gone()方法判断，如果retainedKeys依然包含reference的key，说明该reference没有进入queue队列，也就是没有释放掉对象的引用，所以很可能是发生了内存泄漏。

所以KeyedWeakReference中保存的key值，就是为了方便判断当前的弱引用是否发生了内存泄漏。

### LeakCanary的线程调度

刚才再看刚才忽略的 watchExecutor.execute()方法，watchExecutor是一个接口，它的实现类是AndroidWatchExecutor。
~~~ Java
public final class AndroidWatchExecutor implements WatchExecutor {

    static final String LEAK_CANARY_THREAD_NAME = "LeakCanary-Heap-Dump";
    private final Handler mainHandler;
    private final Handler backgroundHandler;
    private final long initialDelayMillis;
    private final long maxBackoffFactor;

    public AndroidWatchExecutor(long initialDelayMillis) {
        mainHandler = new Handler(Looper.getMainLooper());
        HandlerThread handlerThread = new HandlerThread(LEAK_CANARY_THREAD_NAME);
        handlerThread.start();
        backgroundHandler = new Handler(handlerThread.getLooper());
        this.initialDelayMillis = initialDelayMillis;
        maxBackoffFactor = Long.MAX_VALUE / initialDelayMillis;
    }

    @Override public void execute(Retryable retryable) {
        // 最终都会切换到主线程，调用waitForIdle()方法
        if (Looper.getMainLooper().getThread() == Thread.currentThread()) {
            waitForIdle(retryable, 0);
        } else {
            postWaitForIdle(retryable, 0);
        }
    }

    void postWaitForIdle(final Retryable retryable, final int failedAttempts) {
        // 切换到主线程
        mainHandler.post(new Runnable() {
            @Override public void run() {
                waitForIdle(retryable, failedAttempts);
            }
        });
    }

    void waitForIdle(final Retryable retryable, final int failedAttempts) {
        // This needs to be called from the main thread.
        // 在主线程执行下面代码
        Looper.myQueue().addIdleHandler(new MessageQueue.IdleHandler() {
            @Override public boolean queueIdle() {
            postToBackgroundWithDelay(retryable, failedAttempts);
            // 方法如果返回 false，这个 IdleHandler 只会执行一次
            return false;
            }
        });
    }

    void postToBackgroundWithDelay(final Retryable retryable, final int failedAttempts) {
        // 设置延迟发送时间
        long exponentialBackoffFactor = (long) Math.min(Math.pow(2, failedAttempts), maxBackoffFactor);
        long delayMillis = initialDelayMillis * exponentialBackoffFactor;
        // 子线程运行一个runnable
        backgroundHandler.postDelayed(new Runnable() {
            @Override public void run() {
            Retryable.Result result = retryable.run();
                if (result == RETRY) {
                    postWaitForIdle(retryable, failedAttempts + 1);
                }
            }
        }, delayMillis);
    }
}
~~~
AndroidWatchExecutor类中，有两个handler，一个在main thread，一个在HandlerThread的子线程中。外部调用execute()方法，最终都会在主线程调用waitForIdle()方法。

waitForIdle()运行在主线程，会在主线程添加一个IdleHandler。

#### IdleHandler的妙用

> IdleHandler，这是一种在只有**当消息队列没有消息时**或者是**队列中的消息还没有到执行时间时**才会执行的 IdleHandler，它存放在 `mPendingIdleHandlers` 队列中。

关于IdleHandler如何使用，这篇文章提供了一个非常好的方法：[你知道android的MessageQueue.IdleHandler吗？](https://wetest.qq.com/lab/view/352.html)

简单总结下这篇文章：
- 1.使用HandlerThread，利用HandlerThread绑定的子线程Handler，实现**单线程队列处理数据+异步线程接收数据更新**
- 2.反射HandlerThread，得到MessageQueue对象，调用MessageQueue.addIdleHandler()
- 3.在IdleHandler的queueIdle()回调方法里通知数据改变

这样做有两个好处：
- 1.HanlderThread按加入顺序操作数据，不需要对数据加锁
- 2.对HandlerThread的MessageQueue调用addIdleHandler()，不管操作了多少遍数据，只会在所有操作完成后（idle），通知数据的监听者，避免频繁更新UI

AndroidWatchExecutor的作用就是，在主线程空闲的时候才会调用，并且是延迟调用子线程的retryable.run()方法。这样可以保证不影响主线程的流畅度。

#### 手动触发GC
上面的retryable.run()运行在子线程，说明ensureGone()就是在子线程。在看一下LeakCanary是如何调用GC的：
~~~ Java
public interface GcTrigger {
    GcTrigger DEFAULT = new GcTrigger() {
        @Override public void runGc() {
        // Code taken from AOSP FinalizationTest:
        // https://android.googlesource.com/platform/libcore/+/master/support/src/test/java/libcore/
        // java/lang/ref/FinalizationTester.java
        // System.gc() does not garbage collect every time. Runtime.gc() is
        // more likely to perfom a gc.
        Runtime.getRuntime().gc();
        enqueueReferences();
        System.runFinalization();
    }

    private void enqueueReferences() {
        // Hack. We don't have a programmatic way to wait for the reference queue daemon to move
        // references to the appropriate queues.
        try {
            Thread.sleep(100);
        } catch (InterruptedException e) {
            throw new AssertionError();
        }
    }
    };

  void runGc();
}
~~~
gcTrigger.runGc() 先会调用 Runtime.getRuntime().gc() 以触发系统gc操作，然后当前后台子线程sleep 100毫秒，最后调用System.runFinalization()强制系统回收没有引用的队列，这样确保引用对象是否真的被回收了。

### 泄露信息分析

上面 heapDumper.dumpHeap() 方法的具体实现是在AndroidHeapDumper类中：
~~~ Java
@SuppressWarnings("ReferenceEquality") // Explicitly checking for named null.
@Override public File dumpHeap() {
    // 创建一个XXX_pending.hprof文件，这种文件最多7个
    // 文件存储路径为 /storage/emulated/0/Download/leakcanary-xxx(package-name)/
    File heapDumpFile = leakDirectoryProvider.newHeapDumpFile();

    if (heapDumpFile == RETRY_LATER) {
        // 文件创建失败
        return RETRY_LATER;
    }

    FutureResult<Toast> waitingForToast = new FutureResult<>();
    showToast(waitingForToast);

    if (!waitingForToast.wait(5, SECONDS)) {
        CanaryLog.d("Did not dump heap, too much time waiting for Toast.");
        return RETRY_LATER;
    }

    Toast toast = waitingForToast.get();
    try {
        // 向文件写入内容
        Debug.dumpHprofData(heapDumpFile.getAbsolutePath());
        cancelToast(toast);
        return heapDumpFile;
    } catch (Exception e) {
        CanaryLog.d(e, "Could not dump heap");
        // Abort heap dump
        return RETRY_LATER;
    }
  }

private void showToast(final FutureResult<Toast> waitingForToast) {
    mainHandler.post(new Runnable() {
      @Override public void run() {
        final Toast toast = new Toast(context);
        toast.setGravity(Gravity.CENTER_VERTICAL, 0, 0);
        toast.setDuration(Toast.LENGTH_LONG);
        LayoutInflater inflater = LayoutInflater.from(context);
        toast.setView(inflater.inflate(R.layout.leak_canary_heap_dump_toast, null));
        toast.show();
        // Waiting for Idle to make sure Toast gets rendered.
        Looper.myQueue().addIdleHandler(new MessageQueue.IdleHandler() {
          @Override public boolean queueIdle() {
                waitingForToast.set(toast);
                return false;
          }
        });
      }
    });
  }

private void cancelToast(final Toast toast) {
    mainHandler.post(new Runnable() {
      @Override public void run() {
        toast.cancel();
      }
    });
  }
~~~

这里的FutureResult，会阻塞当前后台子线程，并监听主线程是否空闲，若5秒内不空闲，则返回RETRY_LATER。
若主线程空闲则会调用waitingForToast.set(toast)，不在阻塞后台子线程，调用android.os.Debug.dumpHprofData()生成.prof文件。

> Debug.dumpHprofData 会导致线程阻塞。因为dumpheap的操作是在应用进程的主线程中进行操作，本质上是在该应用进程的虚拟机中进行，dumpheap时应用进程会block住，如过heap文件过大很容易导致应用进程操作界面卡住，如果此时再进行点击或滑动等操作极易再抛出anr等弹窗，用户体验极差。

这里介绍下 FutureResult 的实现：
~~~ Java
public final class FutureResult<T> {
    // AtomicReference实现对象引用的原子更新
    private final AtomicReference<T> resultHolder;
    // 同步辅助类，在完成一组正在其他线程中执行的操作之前，它允许一个或多个线程一直等待
    private final CountDownLatch latch;

    public FutureResult() {
        resultHolder = new AtomicReference<>();
        latch = new CountDownLatch(1);
    }

    public boolean wait(long timeout, TimeUnit unit) {
        try {
        return latch.await(timeout, unit);
        } catch (InterruptedException e) {
        throw new RuntimeException("Did not expect thread to be interrupted", e);
        }
    }

    public T get() {
        if (latch.getCount() > 0) {
            throw new IllegalStateException("Call wait() and check its result");
        }
        return resultHolder.get();
    }

    public void set(T result) {
        resultHolder.set(result);
        latch.countDown();
    }
}
~~~
关于CountDownLatch的介绍参考：[Java并发学习之CountDownLatch实现原理及使用姿势](https://my.oschina.net/u/566591/blog/1560140)

### prof文件分析
关于prof文件的分析，还要回到RefWatch.ensureGone()方法中：
~~~ Java
heapdumpListener.analyze(
        new HeapDump(heapDumpFile, reference.key, reference.name, excludedRefs, watchDurationMs,
            gcDurationMs, heapDumpDurationMs));
~~~

最终会调用到
~~~ Java
HeapAnalyzerService.runAnalysis(context, heapDump, listenerServiceClass);

// 在HeapAnalyzerService 中
public static void runAnalysis(Context context, HeapDump heapDump,
      Class<? extends AbstractAnalysisResultService> listenerServiceClass) {
    Intent intent = new Intent(context, HeapAnalyzerService.class);
    intent.putExtra(LISTENER_CLASS_EXTRA, listenerServiceClass.getName());
    intent.putExtra(HEAPDUMP_EXTRA, heapDump);
    context.startService(intent);
}
~~~
HeapAnalyzerService继承自IntentService。在前面的AndroidManifest中可以看到，HeapAnalyzerService是运行在单独的进程中。

在HeapAnalyzerService的onHandleIntent方法中接收和处理传递过来的dump文件。
~~~ Java
@Override protected void onHandleIntent(Intent intent) {
    if (intent == null) {
      CanaryLog.d("HeapAnalyzerService received a null intent, ignoring.");
      return;
    }
    String listenerClassName = intent.getStringExtra(LISTENER_CLASS_EXTRA);
    HeapDump heapDump = (HeapDump) intent.getSerializableExtra(HEAPDUMP_EXTRA);

    HeapAnalyzer heapAnalyzer = new HeapAnalyzer(heapDump.excludedRefs);

    AnalysisResult result = heapAnalyzer.checkForLeak(heapDump.heapDumpFile, heapDump.referenceKey);
    // 调用另一个AbstractAnalysisResultService的静态方法，将结果返回
    AbstractAnalysisResultService.sendResultToListener(this, listenerClassName, heapDump, result);
}
~~~
再看下在heapAnalyzer.checkForLeak()方法：
~~~ Java
public AnalysisResult checkForLeak(File heapDumpFile, String referenceKey) {
    long analysisStartNanoTime = System.nanoTime();

    if (!heapDumpFile.exists()) {
        Exception exception = new IllegalArgumentException("File does not exist: " + heapDumpFile);
        return failure(exception, since(analysisStartNanoTime));
    }

    try {
        // 使用HAHA库
        HprofBuffer buffer = new MemoryMappedFileBuffer(heapDumpFile);
        HprofParser parser = new HprofParser(buffer);
        Snapshot snapshot = parser.parse();
        // 这里使用了THashMap，去掉重复路径
        deduplicateGcRoots(snapshot);
        // 根据引用key找到泄漏对象
        Instance leakingRef = findLeakingReference(referenceKey, snapshot);

        // False alarm, weak reference was cleared in between key check and heap dump.
        if (leakingRef == null) {
            return noLeak(since(analysisStartNanoTime));
        }
        // 返回一个AnalysisResult对象
        return findLeakTrace(analysisStartNanoTime, snapshot, leakingRef);
    } catch (Throwable e) {
        return failure(e, since(analysisStartNanoTime));
    }
}
~~~
在heapAnalyzer.checkForLeak()方法中引入[HAHA库](github.com/square/haha)(一个heap prof堆文件分析库)，将hprof文件解析成内存快照Snapshot对象进行分析。
还使用jetBrains的[THashMap](https://github.com/JetBrains/intellij-deps-trove4j/blob/master/core/src/main/java/gnu/trove/THashMap.java)(THashMap的内存占用量比HashMap小)做中转，去掉snapshot中GCRoot的重复路径，以减少内存压力。

#### 返回分析结果
在HeapAnalyzerService的onHandleIntent方法中，调用了
~~~ Java
AbstractAnalysisResultService.sendResultToListener(this, listenerClassName, heapDump, result);

// 在AbstractAnalysisResultService 中
public static void sendResultToListener(Context context, String listenerServiceClassName,
      HeapDump heapDump, AnalysisResult result) {
    Class<?> listenerServiceClass;
    try {
      listenerServiceClass = Class.forName(listenerServiceClassName);
    } catch (ClassNotFoundException e) {
      throw new RuntimeException(e);
    }
    Intent intent = new Intent(context, listenerServiceClass);
    intent.putExtra(HEAP_DUMP_EXTRA, heapDump);
    intent.putExtra(RESULT_EXTRA, result);
    context.startService(intent);
}
~~~
启动DisplayLeakService，并将结果返回。DisplayLeakService运行在主线程，继承自AbstractAnalysisResultService，也是一个IntentService。
在AbstractAnalysisResultService的onHandleIntent()接收结果，并回调onHeapAnalyzed()方法，创建并显示一个通知。

具体的结果分析和处理，可以看这篇文章：[LeakCanary源码分析第三讲－HeapAnalyzerService详解](http://vjson.com/wordpress/leakcanary%E6%BA%90%E7%A0%81%E5%88%86%E6%9E%90%E7%AC%AC%E4%B8%89%E8%AE%B2%EF%BC%8Dheapanalyzerservice%E8%AF%A6%E8%A7%A3.html)

**内存泄漏信息保存和分析的大概流程是：**
- 通过Debug.dumpHprofData()，生成一个xxx_pending.hprof文件
- 使用HAHA库（github.com/square/haha），将hprof文件解析成内存快照Snapshot对象进行分析
- 使用jetBrains的THashMap做中转，去掉snapshot中GCRoot的重复路径，以减少内存压力。
- 找出泄露对象并找出泄露对象的最短路径
- 得到结果后，重命名xxx_pending.hprof 文件为yyyy-MM-dd_HH-mm-ss_SSS.hprof
    - 将hprof文件内容和分析结果都保存到yyyy-MM-dd_HH-mm-ss_SSS.hprof.result文件中


以上就是整个LeakCanary捕获内存泄漏，并展示出来的整体流程。


### 学习借鉴

- **使用FutureResult同步主线程和子线程**
- **使用CopyOnWriteArraySet解决并发读写问题**
- **构建者模式，代码简洁、清新，链式调用创建对象，参考RefWatcherBuilder对象**
- **MessageQueue.addIdleHandler(IdleHandler handler)，监听线程空闲**
- **手动GC，参考GCTrigger.runGc()。**
- **IntentService跨进程**

### Ref

https://www.jianshu.com/p/49239eac7a76

https://juejin.im/post/5d09fb7e5188252af2012e73
