---
layout: post
title: NA Crash日志分析
category: accumulation
tags:
  - NA Crash
keywords: NA Crash
banner: https://cdn.conorlee.top/Interior%20of%20the%20Restaurant%20Carrel%20in%20Arles.jpg
thumbnail: https://cdn.conorlee.top/Interior%20of%20the%20Restaurant%20Carrel%20in%20Arles.jpg
toc: true
---

本文主要介绍Native Crash和ANR问题定位（如何分析log日志）
<!--more-->
### NA Crash日志对比

Logcat中抓到的日志：
~~~ java
A  Fatal signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 0x0 in tid 14206 (dodola.breakpad), pid 14206 (dodola.breakpad)
Cmdline: com.dodola.breakpad
pid: 14206, tid: 14206, name: dodola.breakpad  >>> com.dodola.breakpad <<<
	#00 pc 000000000000f6d0  /data/app/~~thPbPJ0ZokAxuuRZPsdShQ==/com.dodola.breakpad-_xwNfmDQiQWDDcg5TlCGqA==/lib/arm64/libcrash-lib.so (Crash()+20) (BuildId: ad36e849968841628dab3afed7e6010c4c500b02)
	#01 pc 000000000000f774  /data/app/~~thPbPJ0ZokAxuuRZPsdShQ==/com.dodola.breakpad-_xwNfmDQiQWDDcg5TlCGqA==/lib/arm64/libcrash-lib.so (Java_com_dodola_breakpad_MainActivity_crash+20) (BuildId: ad36e849968841628dab3afed7e6010c4c500b02)
	#04 pc 000000000031cad4  /data/data/com.dodola.breakpad/code_cache/.overlay/base.apk/classes.dex (com.dodola.breakpad.MainActivity$1$1.run+8)
	#08 pc 000000000031cb1c  /data/data/com.dodola.breakpad/code_cache/.overlay/base.apk/classes.dex (com.dodola.breakpad.MainActivity$1.onClick+20)
~~~

APM SDK抓到的日志（理想状态）：
~~~ java
Native Crash, signal: 11, tname: dodola.breakpad, pid: 13283, tid: 13283, code: 1, error: 0
Stack trace is      
    #0, pc 61075, fname: /data/app/~~kP4phbJym0CIi4w4vSCamQ==/com.dodola.breakpad-qFT3bN7o6AtUgPbpqY5P0Q==/lib/arm64/libcrash-lib.so, (sname: Java_com_dodola_breakpad_MainActivity_stringFromJNI, saddr: 55)   
    #1, pc 61019, fname: /data/app/~~kP4phbJym0CIi4w4vSCamQ==/com.dodola.breakpad-qFT3bN7o6AtUgPbpqY5P0Q==/lib/arm64/libcrash-lib.so, (sname: Java_com_dodola_breakpad_MainActivity_crash, saddr: 15)   
    #2, pc 61003, fname: /data/app/~~kP4phbJym0CIi4w4vSCamQ==/com.dodola.breakpad-qFT3bN7o6AtUgPbpqY5P0Q==/lib/arm64/libcrash-lib.so, (sname: _Z5Crashv, saddr: 15)     
    #3, pc 60987, fname: /data/app/~~kP4phbJym0CIi4w4vSCamQ==/com.dodola.breakpad-qFT3bN7o6AtUgPbpqY5P0Q==/lib/arm64/libcrash-lib.so
                                                                                                    
Java thread(main), stack is : 
    com.dodola.breakpad.MainActivity.crash() -2
    com.dodola.breakpad.MainActivity$1$1.run() 63
    java.lang.Thread.run() 1,015
    com.dodola.breakpad.MainActivity$1.onClick() 65
    android.view.View.performClick() 7,614
    android.view.View.performClickInternal() 7,591
    android.view.View.-$$Nest$mperformClickInternal() 0
    android.view.View$PerformClick.run() 29,809
    android.os.Handler.handleCallback() 942
    android.os.Handler.dispatchMessage() 99
    android.os.Looper.loopOnce() 223
    android.os.Looper.loop() 324
    android.app.ActivityThread.main() 8,546
    java.lang.reflect.Method.invoke() -2
    com.android.internal.os.RuntimeInit$MethodAndArgsCaller.run() 582
    com.android.internal.os.ZygoteInit.main() 1,061                                                                                                    
~~~

信息有：
- NA Crash，Signal 11 -> SIGSEGV -> 非法内存操作，空指针操作
- 进程名，包名只展示后15个字符
- pid 进程id；tid 线程id
- code 1，代表 SEGV_MAPERR 
- Native调用堆栈（backtrace）。具体数据都来自`Dl_info`结构体
  - 行号计数（#0）
  - pc（加载模块的句柄）
  - dli_fname（一个指针，指向包含address的加载模块的文件名）
  - dli_sname（一个指针，指向与指定address最接近的符号的名称）
  - saddr（地址的偏移量，即与代码段函数地址的偏移量）
- Java调用堆栈（根据线程名匹配）

~~~ java
typedef struct {
  /* Pathname of shared object that contains address. */
  const char* dli_fname;
  /* Address at which shared object is loaded. */
  void* dli_fbase;
  /* Name of nearest symbol with address lower than addr. */
  const char* dli_sname;
  /* Exact address of symbol named in dli_sname. */
  void* dli_saddr;
} Dl_info;
~~~

### 根据日志堆栈定位到代码

#### 使用addr2line和objdump命令
这两个命令行工具已在NDK中集成，在我的mac上地址如下：
~~~ c++
/Users/xxx/Library/Android/sdk/ndk/21.4.7075529/toolchains/aarch64-linux-android-4.9/prebuilt/darwin-x86_64/bin
~~~

#### addr2line 工具可以帮助定位到代码行号

执行如下的命令，多个指针地址可以在一个命令中带入，以空格隔开即可：
~~~ c++
./aarch64-linux-android-addr2line -e /Users/xxx/StudioProjects/NativeCrash1/NativeCrashCatcher/app/build/intermediates/cmake/normDebug/obj/arm64-v8a/libcrash-lib.so 000000000000f6d0 000000000000f774
~~~

结果如下：
~~~ c++
/Users/xxx/StudioProjects/NativeCrash1/NativeCrashCatcher/app/.cxx/Debug/2b1z3a2r/arm64-v8a/../../../../src/main/cpp/crash.cpp:13
/Users/xxx/StudioProjects/NativeCrash1/NativeCrashCatcher/app/.cxx/Debug/2b1z3a2r/arm64-v8a/../../../../src/main/cpp/crash.cpp:29
~~~

从addr2line的结果就能看到，我们拿到了错误代码的调用行数（其实也可以分析出调用关系，本例体现不出来），在crash.cpp的13行和29行

> addr2line 命令使用介绍：https://cloud.tencent.com/developer/article/2019609

#### objdump命令获取函数信息
通过addr2line命令，其实我们已经找到了我们代码中出错的位置，已经可以帮助程序员定位问题所在了。但是，这个方法只能获取代码行数，并没有显示函数信息，下面就用objdump命令来获取函数信息.

~~~ c++
 /Users/xxx/Library/Android/sdk/ndk/21.3.6528147/toolchains/aarch64-linux-android-4.9/prebuilt/darwin-x86_64/aarch64-linux-android/bin/objdump -S /Users/xxx/StudioProjects/NativeCrash1/NativeCrashCatcher/app/build/intermediates/cmake/normDebug/obj/arm64-v8a/libcrash-lib.so > testcrash.asm
~~~

在生成的asm文件中查找刚刚我们定位的关键指针000000000000f6d0，如下图
![](/images/blogimages/2023/objdump-example.jpg)

从上图可以看出，这个指针所在的方法名。结合前面的行号，可以进一步确定发生异常的位置（要注意的是，在不同的NDK版本和不同的操作系统中，asm文件的格式不是完全相同，但都大同小异，请大家仔细比对）。

> objdump命令详细介绍：https://cloud.tencent.com/developer/article/1494510

### 实时解析crash日志
使用 ndk-stack工具，在我的电脑上，位置是：
~~~ c++
/Users/xxx/Library/Android/sdk/ndk/21.4.7075529/ndk-stack
~~~

使用方法如下：
~~~ java
adb logcat | $NDK/ndk-stack -sym $PROJECT_PATH/obj/local/armeabi-v7a
~~~
参考链接：https://developer.android.com/ndk/guides/ndk-stack?hl=zh-cn

#### 进程名展示不全原因
> Tips: 为什么进程名对比包名，有时候展示不全？
简单来说，这是因为Linux内核的一个特性：一个进程有两个不同的名称。
- 其中一个是可执行文件路径的最后一个组件。例如，如果您的应用程序位于/data/app/com.example.hello/native_executable，则为native_eexecute。这是出现在/proc/PID/status的“名称”字段中的名称。内核将其截断为15个字符，因此在本例中它包含native_executob。
- 另一个名称由调用应用程序的程序传递，作为其命令行参数。这是出现在/proc/PID/cmdline开头的名称，ps显示该名称。
- 可执行文件的路径也是符号链接/proc/PID/exe的目标。

  按照惯例，当一个程序启动另一个程序时，它应该使用可执行文件的名称作为命令行参数0，但它可以选择其他方式。/proc/PID/status的Name字段始终设置为内核可执行文件的（截断的）名称。

  来自：https://stackoverflow.com/questions/14176058/why-is-the-name-of-a-process-in-proc-pid-status-not-matching-package-name-or-ps

### 线上崩溃堆栈还原
- 1.示例源码：
~~~ c++
int add(){
    int a = 1;
    a ++;
    int b = a+3;
    return b;
}
int div(){
    int a = 1;
    a ++;
    int b = a/0;                //这里除0会引发崩溃
    return b;
}
int _tmain(int argc, _TCHAR* argv[]){
    add();
    sub();
    return 0;
}
~~~
- 2.对应符号表，这里简化了符号表，没带行号信息
~~~ c++
0x00F913B0 ~ 0x00F913F0    add()
0x00F91410 ~ 0x00F91450    div()
0x00F91A90 ~ 0x00F91ACD    _tmain()
~~~
- 3.现有一崩溃堆栈
~~~ c++
0x00F9143A
0x00F91AB0
~~~
- 4.进行符号化
~~~ c++
0x00F9143A    div()    //查找符号表，地址0x00F9143A的符号名，在0x00F91410 ~ 0x00F91450范围内
0x00F91AB0    _tmain() //查找符号表，地址0x00F91AB0的符号名，在0x00F91A90 ~ 0x00F91ACD范围内
~~~
这里的符号化最终也是用到`addr2line`工具

更详细信息参考：[客户端堆栈还原原理及实现](https://segmentfault.com/a/1190000042381613)

### 常见的NA Crash类型
#### 1. SIGILL
SIG是信号名的通用前缀，ILL是 illegal instruction（ `非法指令` ） 的缩写。

对应的数值为 4。

SIGILL 是当一个进程尝试执行一个非法指令时发送给它的信号。

常见原因有:

- CPU架构不匹配
- .so 文件被破坏，或者代码段被破坏导致；
- 主动崩溃，如 _builtintrap() 也会使用非法指令来实现。

si_addr 为出错的指令。

该信号量中常见的错误码说明：

| *Code* |	*说明* |
|:--------:|:-------:|
|ILL_ILLOPC	|非法的操作码(opcode)|
|ILL_ILLOPN	|非法的操作数(operand)|
|ILL_ILLADR	|非法的寻址模式|
|ILL_ILLTRP	|非法的trap|
|ILL_PRVOPC	|特权操作码(Privileged opcode)|
|ILL_PRVREG	|特权寄存器(Privileged register)|
|ILL_COPROC	|协处理器错误|
|ILL_BADSTK	|内部堆栈错误|

#### 2. SIGBUS
SIG是信号名的通用前缀，BUS是bus error (`总线错误`) ，意味着系统检测到硬件问题后发送给进程的信号。

对应的数值为`7`。

通常该信号的产生不是因为硬件有物理上的损坏，而是代码实现有 bug 导致 ，如地址不对齐，或者不存在的物理地址等。

si_addr 为所访问的非法地址。

该信号量中常见的错误码说明：

| *Code* |	*说明* |
|:--------:|:-------:|
|BUS_ADRALN	|访问的地址不对齐。32位处理器一般要求指针是4字节对齐的|
|BUS_ADRERR	|访问不存在的物理地址。一般是由于 mmap 的文件发生 truncated 导致。常见于文件访问过程中，被删除或者替换；或 mmap 到内存后，继续向文件写入且导致文件 truncated，再读取时就会出现该错误；另外， mmap 且访问超过文件实际大小的空间时，也可能会出现该错误|
|BUS_OBJERR	|特定对象的硬件错误|
|BUS_MCEERR_AR	|BUS_MCEERR_AR|
|BUS_MCEERR_AQ	|BUS_MCEERR_AQ|
#### 3. SIGSEGV
SIG 是信号名的通用前缀, SEGV 是 segmentation violation 的缩写。

对应的数值为 `11`。

该信号意味着一个进程执行了一个`无效的内存引用`，或发生`段错误`。

`si_addr` 为所访问的无效地址。

该信号量中常见的错误码说明：

| *Code* |	*说明* |
|:--------:|:-------:|
|SEGV_MAPERR	|地址不在 /proc/self/map 映射中|
|SEGV_ACCERR	|没有访问权限|
#### 4. SIGFPE
SIG是信号名的通用前缀。FPE是floating-point exception（`浮点异常`）的首字母缩写拼接而成。

对应的数值为 `8`。

该信号一般是算术运算相关问题导致的。

si_addr 为失败的指令。

该信号量中常见的错误码说明：

| *Code* |	*说明* |
|:--------:|:-------:|
|FPE_INTDIV	|整数除以 0|
|FPE_INTOVF	|整数溢出|
|FPE_FLTDIV	|浮点数除以 0|
|FPE_FLTOVF	|浮点数向下溢出|
|FPE_FLTRES	|浮点数结果不精确|
|FPE_FLTINV	|无效的浮点运算|
|FPE_FLTSUB	|下标超出范围|
#### 5. SIGABRT
SIG是信号名的通用前缀。ABRT是abort program的缩写。

对应的数值为 `6`。

该信号意味着异常退出；通常是调用 **abort(), raise(), kill(), pthread_kill()** 或者被系统进程杀死时出现。

当错误码为 `SI_USER` 时表示是被其它程序杀死，一般情况是由于ANR被 system_server 杀死；

其他错误码一般是业务自己调用 abort() 等函数退出，此时错误码一般认为无效。

#### 6. SIGSTKFLT
SIG是信号名的通用前缀，STKFLT是stack fault 的缩写。

对应的数值为 `16`。

按照官方文档说明，该信号量意味着协处理器栈故障。

根据网上的部分问题结论说明，在内存耗尽时，一般 malloc 返回 NULL 且设置 errno 为 ENOMEM，但有些系统可能会使用 SIGSTKFLT 信号代替。

#### 7. SIGPIPE
该信号对应的数值为 `13`。

该信号意味着管道错误，通常在进程间通信产生，比如采用FIFO(管道)通信的两个进程，读管道没打开或者意外终止仍继续往管道写，写进程就会收到 SIGPIPE 信号。

#### 8. SIGUSR2
为 `12`。用户自定义信号2。

#### 9. SIGTERM
SIG是信号名的通用前缀，TERM 是 Termination 的缩写。该信号对应的数值为 `15`。

与 SIGKILL (signal 9) 类似，不过 SIGKILL 不可捕获，而 SIGTERM 可被捕获。一般常见于 APP 处于后台时，被系统 vold 进程使用 SIGTERM 杀死，该情形基本没有意义，可忽略。

另外，chromium 多进程架构中，经常也会使用 SIGTERM 杀死出现了异常的子进程。

#### 10. SIGSYS
该信号对应的数值为 `31`，通常是因为无效的 linux 内核系统调用而产生。

在 android O (android 8.0) 中，部分不安全的系统调用被移除，若代码中仍然使用它们，则会出现 SIGSYS。

#### 11. SIGTRAP
该信号对应的数值为 `5`。gdb 调试设置断点等操作使用的信号。

| *Code* |	*说明* |
|:--------:|:-------:|
|TRAP_BRKPT	|TRAP_BRKPT|
|TRAP_TRACE	|TRAP_TRACE|
|TRAP_BRANCH	|TRAP_BRANCH|
|TRAP_HWBKPT	|TRAP_HWBKPT|

#### 
https://jtl.jiduprod.com/peqa/apm/abnormal-analysis/abnormal-detail/116218?env=prod_publish


https://jtl.jiduprod.com/peqa/apm/abnormal-analysis/abnormal-detail/113142?env=staging_common

https://jtl.jiduprod.com/peqa/apm/abnormal-analysis/abnormal-detail/108449?env=staging_common

找不到java层对应的线程：
https://jtl.jiduprod.com/peqa/apm/abnormal-analysis/abnormal-detail/109892?env=staging_common

filament 堆栈
https://jtl.jiduprod.com/peqa/apm/abnormal-analysis/abnormal-detail/117583?env=staging_common