---
layout: post
title: Matrix io 介绍和使用
category: accumulation
tags:
  - Matrix
keywords: Matrix, IO, APM
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Iron%20Mill%20in%20The%20Hague.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Iron%20Mill%20in%20The%20Hague.jpg
---

## IO-Canary

参考 Matrix IO-Canary 功能，实现对主线程操作io、重复读相同文件、读写文件buffer过小的代码动态检测


<!--more-->
### 主要类

#### IOIssue
在 Java 层实体类是：
~~~ java
public final class IOIssue {
    public final int type;
    public final String path;
    public final long fileSize;
    public final int opCnt;
    public final long bufferSize;
    public final long opCostTime;
    public final int opType;
    public final long opSize;
    public final String threadName;
    public final String stack;

    public final int repeatReadCnt;

    public IOIssue(int type, String path, long fileSize, int opCnt, long bufferSize, long opCostTime,
                   int opType, long opSize, String threadName, String stack, int repeatReadCnt) {
        this.type = type;
        this.path = path;
        this.fileSize = fileSize;
        this.opCnt = opCnt;
        this.bufferSize = bufferSize;
        this.opCostTime = opCostTime;
        this.opType = opType;
        this.opSize = opSize;
        this.threadName = threadName;
        this.stack = stack;
        this.repeatReadCnt = repeatReadCnt;
    }
}
~~~

#### detector.h

功能实现是在Native层，可以先看 detector.h 这个类

~~~ C++
#ifndef MATRIX_IO_CANARY_DETECTOR_H
#define MATRIX_IO_CANARY_DETECTOR_H

#include <vector>
#include <set>
#include "core/io_info_collector.h"
#include "core/io_canary_env.h"

namespace iocanary {

    typedef enum {
        kDetectorMainThreadIO = 0,
        kDetectorSmallBuffer,
        kDetectorRepeatRead
    } DetectorType;

    typedef enum {
        kIssueMainThreadIO = 1,
        kIssueSmallBuffer,
        kIssueRepeatRead
    } IssueType ;

    class Issue {
    public:
        Issue(IssueType type, IOInfo file_io_info);

        const IssueType type_;
        const IOInfo file_io_info_;
        const std::string key_;

        int repeat_read_cnt_;
        std::string stack;
    private:
        static std::string GenKey(const IOInfo& file_io_info);
    };

    class FileIODetector {
    public:
        virtual ~FileIODetector();

        virtual void Detect(const IOCanaryEnv& env, const IOInfo& file_io_info, std::vector<Issue>& issues) = 0;

    protected:
        void PublishIssue(const Issue& target, std::vector<Issue>& issues);

    private:
        void MarkIssuePublished(const std::string& key);
        bool IsIssuePublished(const std::string& key);

        std::set<std::string> published_issue_set_;
    };
}
#endif //MATRIX_IO_CANARY_DETECTOR_H
~~~

重点看 Detect 方法的实现，有三个类，分别是：main_thread_detector.cc 、repeat_read_detector.cc、small_buffer_detector.cc

每个类对应一个功能，触发场景分别如下：

### 检测场景

#### main_thread_detector
触发条件：
- 1.必须是主线程
- 2.单次文件读写超过时间限制（kPossibleNegativeThreshold），type = 1，位置见下面
- 3.整体连续读写超时（kDefaultMainThreadTriggerThreshold）,type = 2，位置见下面
- 4.两种情况可以同时命中，type = 3
- 5.type不等于1，则上报

同时，用repeat_read_cnt 来记录type 值。

#### repeat_read_detector 
触发条件比较复杂：
- 1.用map记录文件被读取次数，文件path作为key。如果最大读取时间小于 kPossibleNegativeThreshold，则不会向map里记录。否则会计入map
- 2.如果文件被写过，从map中拿到repeat信息，清空该repeat信息，否则记录然后+1
- 3.如果repeat count 大于等于 kDefaultRepeatReadThreshold 次数，则上报

#### small_buffer_detector
这个比较简单：
- 1.buffer 操作次数 大于 kSmallBufferOpTimesThreshold
- 2.buffer 的空间大小 小于 kDefaultBufferSmallThreshold （4096KB）
- 3.最大读写时间 大于等于 kPossibleNegativeThreshold
- 4.三种条件需要同时满足，就会上报

#### 

预定义的限制数据，都是定义在 io_canary_env.h 文件中：
~~~ C++

#ifndef MATRIX_IO_CANARY_IO_CANARY_ENV_H
#define MATRIX_IO_CANARY_IO_CANARY_ENV_H

namespace iocanary {

    enum IOCanaryConfigKey {
        kMainThreadThreshold = 0,
        kSmallBufferThreshold,
        kRepeatReadThreshold,

        //!!kConfigKeysLen always the last one!!
        kConfigKeysLen
    };

    class IOCanaryEnv {
    public:
        IOCanaryEnv();
        void SetConfig(IOCanaryConfigKey key, long val);

        long GetJavaMainThreadID() const;
        long GetMainThreadThreshold() const;
        long GetSmallBufferThreshold() const;
        long GetRepeatReadThreshold() const;

        //in μs.
        //it may be negative if the io-cost more than POSSIBLE_NEGATIVE_THRESHOLD
        //else it can be negligible
        //80% of the well-known 16ms
        constexpr static const int kPossibleNegativeThreshold = 13*1000;
        constexpr static const int kSmallBufferOpTimesThreshold = 20;
    private:
        long GetConfig(IOCanaryConfigKey key) const;

        //in μs
        constexpr static const int kDefaultMainThreadTriggerThreshold = 500*1000; // 500毫秒
        //We take 4096B(4KB) as a small size of the buffer
        constexpr static const int kDefaultBufferSmallThreshold = 4096;
        constexpr static const int kDefaultRepeatReadThreshold = 5;

        long configs_[IOCanaryConfigKey::kConfigKeysLen];
    };
}

#endif //MATRIX_IO_CANARY_IO_CANARY_ENV_H
~~~

### Matrix 向后兼容 
Matrix 目前只兼容到了Android P，也就是 Android 9。 下面对 Android 9 以上版本的适配，做了一定的兼容，当然下面的代码仅仅说明了适配的方向，可能还有一些其他问题需要处理。

对于 Closeable 泄露监控来说，在 Android 10 及上无法兼容的原因是 CloseGuard#getReporter 无法直接通过反射获取， reporter 字段也是无法直接通过反射获取。如果无法获取到原始的 reporter，那么原始的 reporter 在我们 hook 之后就会失效。如果我们狠下决心，这也是可以接受的，但是对于这种情况我们应该尽量避免。

那么我们现在的问题就是如何在高版本上获取到原始的 reporter，那么有办法吗？有的，因为我们前面说到了无法直接通过反射获取，但是可以间接获取到。这里我们可以通过 反射的反射 来获取。实例如下：

~~~ java
private static void doHook() throws Exception {
    Class<?> clazz = Class.forName("dalvik.system.CloseGuard");
    Class<?> reporterClass = Class.forName("dalvik.system.CloseGuard$Reporter");

    Method setEnabledMethod = clazz.getDeclaredMethod("setEnabled", boolean.class);
    setEnabledMethod.invoke(null, true);

    // 直接反射获取reporter
//        Method getReporterMethod = clazz.getDeclaredMethod("getReporter");
//        final Object originalReporter = getReporterMethod.invoke(null);

    // 反射的反射获取
    Method getDeclaredMethodMethod = Class.class.getDeclaredMethod("getDeclaredMethod", String.class, Class[].class);
    Method getReporterMethod = (Method) getDeclaredMethodMethod.invoke(clazz, "getReporter", null);
    final Object originalReporter = getReporterMethod.invoke(null);

    Method setReporterMethod = clazz.getDeclaredMethod("setReporter", reporterClass);
    Object proxy = Proxy.newProxyInstance(
            reporterClass.getClassLoader(),
            new Class<?>[]{reporterClass},
            new InvocationHandler() {
                @Override
                public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
                    return method.invoke(originalReporter, args);
                }
            }
    );
    setReporterMethod.invoke(null, proxy);
}
~~~

对于 native hook 的三种场景来说，Android 9 版本以上无法适配的原因应该是 hook 失效导致的。可以使用字节的 [bytehook](https://github.com/bytedance/bhook) 替换老旧的 xhook 来达到同样的效果。

同时对于 9 以后的版本需要 hook "libc.so" 的write和 read 方法：
~~~ C++
// ...
bool is_libjavacore = (strstr(so_name, "libc.so") != nullptr || strstr(so_name, "libjavacore.so") != nullptr);
if (is_libjavacore) {
    bytehook_stub_t stub2 = bytehook_hook_single(so_name, NULL, "read", (void *) ProxyRead, hacker_bytehook_hooked_callback, NULL);
    if (stub2 == nullptr) {
        __android_log_print(ANDROID_LOG_WARN, kTag,
                            "doHook hook read failed, try __read_chk");
        bytehook_stub_t stub3 =bytehook_hook_single(so_name, NULL, "__read_chk", (void *) ProxyReadChk,hacker_bytehook_hooked_callback, NULL);
        if (stub3 == nullptr) {
            __android_log_print(ANDROID_LOG_WARN, kTag,
                                                "doHook hook failed: __read_chk");
            return JNI_FALSE;
        } else {
            __android_log_print(ANDROID_LOG_WARN, kTag, "doHook hook __read_chk success");
            anrHookStubs.push_back(stub3);
        }
    } else {
        __android_log_print(ANDROID_LOG_WARN, kTag, "doHook hook read success");
        anrHookStubs.push_back(stub2);
    }

    bytehook_stub_t stub4 = bytehook_hook_single(so_name,NULL, "write", (void*)ProxyWrite,  hacker_bytehook_hooked_callback, NULL);
    if (stub4 == nullptr) {
        __android_log_print(ANDROID_LOG_WARN, kTag, "doHook hook write failed, try __write_chk");
        bytehook_stub_t stub5 = bytehook_hook_single(so_name, NULL,"__write_chk", (void*)ProxyWriteChk, hacker_bytehook_hooked_callback, NULL);
        if (stub5 == nullptr) {
            __android_log_print(ANDROID_LOG_WARN, kTag, "doHook hook failed: __write_chk");
            return JNI_FALSE;
        } else {
            __android_log_print(ANDROID_LOG_WARN, kTag, "doHook hook __write_chk success");
            anrHookStubs.push_back(stub5);
        }
    } else {
        __android_log_print(ANDROID_LOG_WARN, kTag, "doHook hook write success");
        anrHookStubs.push_back(stub4);
    }
}
// ...
~~~
