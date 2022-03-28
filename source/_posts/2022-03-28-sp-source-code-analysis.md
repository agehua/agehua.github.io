---
layout: post
title:  SP源码阅读和其导致ANR原因分析
category: accumulation
tags:
  - Android Source Code
keywords: SharedPreferences, sp
banner: https://cdn.conorlee.top/Green%20Wheat%20Field%20with%20Cypress.jpg
thumbnail: https://cdn.conorlee.top/Green%20Wheat%20Field%20with%20Cypress.jpg
toc: true
---

### 背景
本文是在看了字节的这篇文章: [剖析 SharedPreference apply 引起的 ANR 问题](<https://www.jianshu.com/p/9ae0f6842689>)，发现很多原理不太清楚，特意根据源码总结一下，避免网上的人云亦云

<!--more-->
### 从如何使用开始

我们使用SP，都是通过Context，直接看ContextImpl的具体实现：
~~~ java
class ContextImpl extends Context {
    //静态存储类，缓存所有应用的SP容器,该容器key对应应用名称，value则为每个应用存储所有sp的容器
    private static ArrayMap<String, ArrayMap<File, SharedPreferencesImpl>> sSharedPrefsCache;
    
     @Override
    public SharedPreferences getSharedPreferences(String name, int mode) {
        ......
        
        File file;
        synchronized (ContextImpl.class) {  // 获取锁，然后根据name获取相对应的文件
            if (mSharedPrefsPaths == null) {
                mSharedPrefsPaths = new ArrayMap<>();
            }
            file = mSharedPrefsPaths.get(name);
            if (file == null) {
                file = getSharedPreferencesPath(name);
                mSharedPrefsPaths.put(name, file);
            }
        }
        return getSharedPreferences(file, mode);
    }

    @Override
    public SharedPreferences getSharedPreferences(File file, int mode) {
        SharedPreferencesImpl sp;
        synchronized (ContextImpl.class) { // 获取类锁
            // 根据packagename从 sSharedPrefsCache 中拿到当前包名的cache
            final ArrayMap<File, SharedPreferencesImpl> cache = getSharedPreferencesCacheLocked();
            sp = cache.get(file); // 得到一个 SharedPreferencesImpl 实例
            if (sp == null) { // cache中没有就创建一个
                checkMode(mode);
                if (getApplicationInfo().targetSdkVersion >= android.os.Build.VERSION_CODES.O) {
                    if (isCredentialProtectedStorage()
                            && !getSystemService(UserManager.class)
                                    .isUserUnlockingOrUnlocked(UserHandle.myUserId())) {
                        throw new IllegalStateException("SharedPreferences in credential encrypted "
                                + "storage are not available until after user is unlocked");
                    }
                }
                sp = new SharedPreferencesImpl(file, mode);
                cache.put(file, sp);
                return sp;
            }
        }
        if ((mode & Context.MODE_MULTI_PROCESS) != 0 ||
            getApplicationInfo().targetSdkVersion < android.os.Build.VERSION_CODES.HONEYCOMB) {
            // If somebody else (some other process) changed the prefs
            // file behind our back, we reload it.  This has been the
            // historical (if undocumented) behavior.
            sp.startReloadIfChangedUnexpectedly();
        }
        return sp;
    }
    
}
~~~

> ArrayMap 是android framework 中非常常见的容器类，比如 Bundle 中传递的 Key和 Value 都是用ArrayMap来存储的

继续看 SharedPreferencesImpl 是怎么实现保存的

### 找到具体实现类 SharedPreferencesImpl
SharedPreferencesImpl 实现了SharedPreferences 接口，其内部类 EditorImpl 也实现 Editor 接口

我们随便找一个EditorImpl类的 putXXX的方法，比如 putBoolean ：
~~~ java
// EditorImpl
    @Override
    public Editor putBoolean(String key, boolean value) {
        synchronized (mEditorLock) { // mEditorLock是一个Object对象锁
            mModified.put(key, value); // mModified是一个 HashMap，保存暂时修改的值
                return this;
        }
    }
    // 为什么要多贴一个 remove方法，继续看下面的commit方法
    @Override
    public Editor remove(String key) {
        synchronized (mEditorLock) {
            mModified.put(key, this);
            return this;
        }
    }
~~~

> 从这里也能看到SP的整个实现，各种地方都加了synchronized实现线程安全，应该是线程安全的

Editor调用完putXXX()后，需要调用 commit() 或 apply()来保存提交，我们先看下commit() :

~~~ java
// EditorImpl
    @Override
    public boolean commit() {
        long startTime = 0;

        if (DEBUG) {
            startTime = System.currentTimeMillis();
        }
        // 将提交的数据保存到内存中
        MemoryCommitResult mcr = commitToMemory();

        SharedPreferencesImpl.this.enqueueDiskWrite(
            mcr, null /* sync write on this thread okay */);
        try {
            // 调用了CountDownLatch的 await 方法，等待写入文件完成，会阻塞当前线程
            mcr.writtenToDiskLatch.await(); 
        } catch (InterruptedException e) {
            return false;
        } finally {
            if (DEBUG) {
                Log.d(TAG, mFile.getName() + ":" + mcr.memoryStateGeneration
                        + " committed after " + (System.currentTimeMillis() - startTime)
                        + " ms");
            }
        }
        notifyListeners(mcr);
        return mcr.writeToDiskResult;
    }
~~~
> 上面的await()，如果SP内容过多，会阻塞主线程导致ANR，但这种情况并不常见

继续看上面提到的 commitToMemory()
~~~ java
// EditorImpl
    private MemoryCommitResult commitToMemory() { // Returns true if any changes were made
        long memoryStateGeneration;
        boolean keysCleared = false;
        List<String> keysModified = null;
        Set<OnSharedPreferenceChangeListener> listeners = null;
        Map<String, Object> mapToWriteToDisk;

        synchronized (SharedPreferencesImpl.this.mLock) {
            // We optimistically don't make a deep copy until
            // a memory commit comes in when we're already
            // writing to disk.
            if (mDiskWritesInFlight > 0) {
                // 有未同步到本地的数据
                mMap = new HashMap<String, Object>(mMap);
            }
            mapToWriteToDisk = mMap;
            // mDiskWritesInFlight 代表的是“此时需要将数据写入磁盘，但还未处理或未处理完成的次数”
            // 将 mDiskWritesInFlight 自增1（这里是唯一会增加 mDiskWritesInFlight 的地方）
            mDiskWritesInFlight++;

            // 省略 OnSharedPreferenceChangeListener 通知
            synchronized (mEditorLock) {
                boolean changesMade = false;

                if (mClear) { // 只有调用了 Editor.clear() ，mClear才为true
                    if (!mapToWriteToDisk.isEmpty()) {
                        changesMade = true;
                        mapToWriteToDisk.clear();
                    }
                    keysCleared = true;
                    mClear = false;
                }
                // mModified是上面调用 putXXX方法来保存对应key和value的容器
                for (Map.Entry<String, Object> e : mModified.entrySet()) {
                    String k = e.getKey();
                    Object v = e.getValue();
                    // 当v 为 this或 v为null时移除对应的key
                    if (v == this || v == null) { // 什么情况下v为this？再看下上面的remove(key)方法
                        if (!mapToWriteToDisk.containsKey(k)) {
                            continue;
                        }
                        mapToWriteToDisk.remove(k);
                    } else {
                        if (mapToWriteToDisk.containsKey(k)) {// 相同key的value是否相等，相等则跳过
                            Object existingValue = mapToWriteToDisk.get(k);
                            if (existingValue != null && existingValue.equals(v)) {
                                continue;
                            }
                        }
                        mapToWriteToDisk.put(k, v);
                    }

                    changesMade = true;
                }

                mModified.clear(); // 清空 mModified 历史记录

                if (changesMade) {
                    mCurrentMemoryStateGeneration++;
                }

                memoryStateGeneration = mCurrentMemoryStateGeneration;
            }
        }
        // memoryStateGeneration 包含了所有的数据，所以每次commit都会把所有数据都写入磁盘一遍
        return new MemoryCommitResult(memoryStateGeneration, keysCleared, keysModified,
                listeners, mapToWriteToDisk);
    }
~~~
总结一下，commitToMemory()方法会将 Editor 中的提交和现有内存中的数据（mMap）合并，保存到 mapToWriteToDisk （其实也是mMap）中。

需要注意的是，在commitToMemory()方法中，当mClear为true，会清空mMap，但不会清空mModified，所以依然会遍历mModified，将其中保存的写记录同步到mMap中，所以下面这种写法是错误的：

~~~ java
sharedPreferences.edit()
    .putString("key1", "value1")    // key1 不会被 clear 掉，commit 之后依旧会被写入磁盘中
    .clear()
    .commit();
~~~

> 此外，为什么Editor.remove(Key)时为什么不直接在map中移除对应key呢？这是为了在多线程场景下保证数据一致

看下 MemoryCommitResult 都有什么内容：
~~~ java
// SharedPreferencesImpl 的内部类 MemoryCommitResult
    private static class MemoryCommitResult {
        final long memoryStateGeneration;
        final boolean keysCleared;
        @Nullable final List<String> keysModified;
        @Nullable final Set<OnSharedPreferenceChangeListener> listeners;
        final Map<String, Object> mapToWriteToDisk;
        final CountDownLatch writtenToDiskLatch = new CountDownLatch(1);
        //...
        // 这个方法在写入文件后会被调用
        void setDiskWriteResult(boolean wasWritten, boolean result) {
            this.wasWritten = wasWritten;
            writeToDiskResult = result;
            writtenToDiskLatch.countDown(); // CountDownLatch 减一
        }
    }
~~~

commitToMemory() 之后会调用下面的方法将数据写入文件中
~~~ java
    private void enqueueDiskWrite(final MemoryCommitResult mcr,
                                  final Runnable postWriteRunnable) {
        final boolean isFromSyncCommit = (postWriteRunnable == null);
        // 创建一个 Runnable 对象，该对象负责写磁盘操作
        final Runnable writeToDiskRunnable = new Runnable() {
                @Override
                public void run() {
                    synchronized (mWritingToDiskLock) {
                       // 顾名思义了，这就是最终通过文件操作将数据写入磁盘的方法了
                        writeToFile(mcr, isFromSyncCommit);
                    }
                    synchronized (mLock) {
                        // 写入磁盘后，将 mDiskWritesInFlight 自减1，代表写磁盘的需求减少一个
                        mDiskWritesInFlight--;
                    }
                    if (postWriteRunnable != null) {  // 执行 postWriteRunnable（提示，在 apply 中，postWriteRunnable 才不为 null）
                        postWriteRunnable.run();
                    }
                }
            };

        // Typical #commit() path with fewer allocations, doing a write on
        // the current thread.
        if (isFromSyncCommit) { // 由commit调用时，这里是true
            boolean wasEmpty = false;
            synchronized (mLock) {
                // 如果此时只有一个 commit 请求（注意，是 commit 请求，而不是 apply ）未处理，那么 wasEmpty 为 true
                wasEmpty = mDiskWritesInFlight == 1;
            }
            if (wasEmpty) {
                // 当只有一个 commit 请求未处理，那么无需开启线程进行处理，直接在本线程执行 writeToDiskRunnable 即可
                writeToDiskRunnable.run();
                return;
            }
        }
         // 将 writeToDiskRunnable 方法线程池中执行（非主线程）
        // 程序执行到这里，有两种可能：
        // 1. 调用的是 commit() 方法，并且当前只有一个 commit 请求未处理
        // 2. 调用的是 apply() 方法，第二个参数为true
        QueuedWork.queue(writeToDiskRunnable, !isFromSyncCommit);
    }
~~~

### apply()和commit()到底有什么区别

再来看下apply()和commit()有什么不同。
~~~ java
    @Override
    public void apply() {
        final long startTime = System.currentTimeMillis();
        // commitToMemory方法前面已经分析过，这里和commit方法相同
        final MemoryCommitResult mcr = commitToMemory(); 
        final Runnable awaitCommit = new Runnable() {
                @Override
                public void run() {
                    try {
                        mcr.writtenToDiskLatch.await(); // commit方法中也提到了这里会等待写入文件完成
                    } catch (InterruptedException ignored) {
                    }
                    // 省略log信息
                }
            };
        // 由一个LinkedList 保存
        QueuedWork.addFinisher(awaitCommit);

        Runnable postWriteRunnable = new Runnable() {
                @Override
                public void run() {
                     // 在 enqueueDiskWrite 中调用了 run() 方法
                    awaitCommit.run();
                    QueuedWork.removeFinisher(awaitCommit); // 移除掉对应的awaitCommit
                }
            };
        // 比commit方法多了一个 runnable
        SharedPreferencesImpl.this.enqueueDiskWrite(mcr, postWriteRunnable);
    }
~~~
对比apply和commit可以发现。commitToMemory()是一样的，不同点如下：

- commit()只有一个待处理的（**`mDiskWritesInFlight == 1`**）则不会进入到 QueuedWork ，直接由**writeToDiskRunnable**处理
- commit()有多个待处理，和apply()一样都会进入到 **QueuedWork**，区别是第二个参数分别是 false和true

下面在看 QueuedWork.queue方法：
~~~ java
// QueuedWork.java
    public static void queue(Runnable work, boolean shouldDelay) {
        Handler handler = getHandler();

        synchronized (sLock) {
            sWork.add(work); // LinkedList 保存每个 work

            if (shouldDelay && sCanDelay) { // apply会延迟 100
                handler.sendEmptyMessageDelayed(QueuedWorkHandler.MSG_RUN, DELAY);
            } else {
                handler.sendEmptyMessage(QueuedWorkHandler.MSG_RUN);
            }
        }
    }
~~~
handler 对象是在子线程中创建的：
~~~ java
// QueuedWork.java
    private static Handler getHandler() {
        synchronized (sLock) {
            if (sHandler == null) {
                HandlerThread handlerThread = new HandlerThread("queued-work-looper",
                        Process.THREAD_PRIORITY_FOREGROUND);
                handlerThread.start();
                // 创建了一个子线程handler
                sHandler = new QueuedWorkHandler(handlerThread.getLooper());
            }
            return sHandler;
        }
    }
~~~

由handler处理 MSG_RUN 消息，最终会调用到下面的方法：

~~~ java
// QueuedWork.java
    private static void processPendingWork() { // 执行在子线程
        long startTime = 0;

        if (DEBUG) {
            startTime = System.currentTimeMillis();
        }

        synchronized (sProcessingWork) {
            LinkedList<Runnable> work;

            synchronized (sLock) {
                work = (LinkedList<Runnable>) sWork.clone();
                sWork.clear();
                getHandler().removeMessages(QueuedWorkHandler.MSG_RUN);
            }
            // 遍历处理 LinkedList 中的消息
            if (work.size() > 0) {
                for (Runnable w : work) { // 这里的work是 enqueueDiskWrite() 方法中的 writeToDiskRunnable
                    w.run();
                }

                if (DEBUG) {
                    Log.d(LOG_TAG, "processing " + work.size() + " items took " +
                            +(System.currentTimeMillis() - startTime) + " ms");
                }
            }
        }
    }
~~~

可以看到 apply() 和 commit() 方法都是提交runnable给 `QueuedWork`，由 `QueuedWork`保存在一个LinkedList中，然后由Handler切换到**子线程**去遍历 LinkedList 挨个执行。只不过 apply() 比 commit() 多了100毫秒延迟。

### 为什么SP会导致很多ANR

既然是在子线程中执行，为什么网上都说 apply和commit会导致ANR呢？

那就是下面这个关键方法，注意看官方的注释：
~~~ java
// QueuedWork.java
    /**
     * Trigger queued work to be processed immediately. The queued work is processed on a separate
     * thread asynchronous. While doing that run and process all finishers on this thread. The
     * finishers can be implemented in a way to check weather the queued work is finished.
     * 
     * Is called from the Activity base class's onPause(), after BroadcastReceiver's onReceive,
     * after Service command handling, etc. (so async work is never lost)
     */
    public static void waitToFinish() {
        long startTime = System.currentTimeMillis();
        boolean hadMessages = false;

        Handler handler = getHandler();

        synchronized (sLock) { // 拿到锁之后移除handler中消息，前面 queue()方法也是用这个锁
            if (handler.hasMessages(QueuedWorkHandler.MSG_RUN)) {
                // Delayed work will be processed at processPendingWork() below
                handler.removeMessages(QueuedWorkHandler.MSG_RUN);
            }
            // We should not delay any work as this might delay the finishers
            sCanDelay = false;
        }

        StrictMode.ThreadPolicy oldPolicy = StrictMode.allowThreadDiskWrites();// 允许 disk reads & writes
        try {
            processPendingWork(); // 这里会耗时
        } finally {
            StrictMode.setThreadPolicy(oldPolicy); // 回退 StrictMode
        }

        try {
            while (true) { // 遍历通知各个finisher
                Runnable finisher;
                synchronized (sLock) {
                    finisher = sFinishers.poll();
                }
                if (finisher == null) {
                    break;
                }
                finisher.run();
            }
        } finally {
            sCanDelay = true;
        }

        // 删除耗时统计
    }
~~~
简单翻译一下：`waitToFinish()` 会立即触发队列的处理。队列中的work是在单独的线程中异步处理的，同时所有的finishers也是在这个线程中处理。finishers是用来实现一种检查队列中的work是否完成的方式。

`waitToFinish()` 会在一些地方被调用，比如：Activity的onPause()中，BroadcastReceiver's onReceive之后，Service command handling之后等等（其实这些都是在****主线程**）。

在 ActivityThread的搜索`waitToFinish()`这个方法，可以看到有好几处，比如 handleStopService()中，如下：
~~~ java
    private void handleStopService(IBinder token) {
        Service s = mServices.remove(token);
        if (s != null) {
            try {

                s.onDestroy();
                s.detachAndCleanUp();
                //..

                QueuedWork.waitToFinish();
        //...
    }
~~~
ActivityThread 是主线程，所以当SP中提交的修改比较多时，因为 QueuedWork.waitToFinish() 有可能会导致主线程 ANR

比如下面这个ANR日志：
~~~ java
java.lang.Object.wait(Native Method)
java.lang.Thread.parkFor(Thread.java:1220)
sun.misc.Unsafe.park(Unsafe.java:299)
java.util.concurrent.locks.LockSupport.park(LockSupport.java:157)
java.util.concurrent.locks.AbstractQueuedSynchronizer.parkAndCheckInterrupt(AbstractQueuedSynchronizer.java:813)
java.util.concurrent.locks.AbstractQueuedSynchronizer.doAcquireSharedInterruptibly(AbstractQueuedSynchronizer.java:973)
java.util.concurrent.locks.AbstractQueuedSynchronizer.acquireSharedInterruptibly(AbstractQueuedSynchronizer.java:1281)
java.util.concurrent.CountDownLatch.await(CountDownLatch.java:202)
android.app.SharedPreferencesImpl$EditorImpl$1.run(SharedPreferencesImpl.java:363)
android.app.QueuedWork.waitToFinish(QueuedWork.java:88)
android.app.ActivityThread.handleServiceArgs(ActivityThread.java:3336)
android.app.ActivityThread.access$2300(ActivityThread.java:197)
android.app.ActivityThread$H.handleMessage(ActivityThread.java:1709)
android.os.Handler.dispatchMessage(Handler.java:111)
android.os.Looper.loop(Looper.java:224)
android.app.ActivityThread.main(ActivityThread.java:5958)
java.lang.reflect.Method.invoke(Native Method)
java.lang.reflect.Method.invoke(Method.java:372)
com.android.internal.os.ZygoteInit$MethodAndArgsCaller.run(ZygoteInit.java:1113)
~~~

### 总结
- SharedPreferences肯定是线程安全的，它的内部实现使用了大量synchronized关键字，但不是进程安全的
- SharedPreferences 不要存放特别大的数据
  - 第一次加载时，需要将整个SP加载到内存当中，如果过于大，会导致阻塞，甚至会导致 ANR
  - 每次apply或者commit，都会把全部的数据一次性写入磁盘, 所以 SP 文件不应该过大, 影响整体性能
  - SharedPreference的文件存储性能与`文件大小`相关，我们不要将毫无关联的配置项保存在同一个文件中，同时考虑将频繁修改的条目单独隔离出来
- apply()同步回写（commitToMemory()）内存SharedPreferences.mMap，然后把异步回写磁盘的任务放到一个`子线程`中等待处理。apply()不需要等待写入磁盘完成，而是马上返回
- commit()同步回写（commitToMemory()）内存SharedPreferences.mMap，然后如果`mDiskWritesInFlight`（此时需要将数据写入磁盘，但还未处理或未处理完成的次数）的值等于1，那么直接在调用 `commit()的线程`（一般是主线程）执行回写磁盘的操作，否则把异步回写磁盘的任务放到一个子线程中等待执行。commit()会阻塞调用线程，直到写入磁盘完成才返回
- SP 操作仅仅把 commit() 替换为 apply() 不是万能的，如果 commit() 过于频繁也会和apply一样，在 `ActivityThread` 中导致ANR



### Ref

[面试高频题：一眼看穿 SharedPreferences](<https://juejin.cn/post/6844903758355234824>)
