---
layout: post
title: NA Crash捕获原理
category: accumulation
tags:
  - NA Crash
keywords: NA Crash
banner: https://cdn.conorlee.top/Houses%20in%20Auvers.jpg
thumbnail: https://cdn.conorlee.top/Houses%20in%20Auvers.jpg
toc: true
---

本文主要介绍Native Crash捕获原理
<!--more-->
### 目前流行的Native Crash捕获方案

| *方案* |	*优点* |	*缺点* |
|:--------:|:-------:|:--------:|
|Google Breakpad |官方出品，多平台支持，文档全面 |代码体量大，crash信息保存在mini_dump文件中，拿到dump文件后，需要再次解析才能获取crash堆栈|
| 利用Logcat日志 |利用Android系统实现	| 需要再Crash时启动新进程过滤logcat日志，不可靠|
|unwind库	|实现简单，改动容易。很多三方库也是基于该方案| 需要了解信号机制，自行实现获取Crash堆栈方式| 

### 本次实践采用方案三
- 使用 `信号量+<unwind.h>`库。
- 崩溃信息写入文件，通过 log 上传到日志平台（Elastic）
- 下次启动后读取文件内容上报，然后删除文件
- 对java层的崩溃捕获不会有任何影响，native层bugly不再能够上报异常


~~目前遗留的问题~~
- ~~64位so在部分设备（比如我自己的 IQOO）上无法获取到崩溃堆栈，只能捕获到信号，但是32位的so没有这个问题~~
- ~~数据可以上传到java层，但是日志上报是在java层的子线程，native无法等待子线程执行完成~~
- ~~堆栈信息在java层上传的时候未格式化~~


### 实现方案细节
要实现 Native Crash 的收集，主要有四个重点：

- 知道 Crash 的发生；
- 捕获到 Crash 的位置；
- 获取 Crash 发生位置的函数调用栈；
- 数据能回传到服务器。

#### 一、知道 Crash 的发生
与 Java 平台不同，C/C++ 没有一个通用的异常处理接口，在 C 层，CPU 通过异常中断的方式，触发异常处理流程。不同的处理器，有不同的异常中断类型和中断处理方式，linux 把这些中断处理，统一为信号量，每一种异常都有一个对应的信号，可以注册回调函数进行处理需要关注的信号量。


常见的信号量有：
~~~ c++
#define SIGHUP 1 // 终端连接结束时发出(不管正常或非正常)
#define SIGINT 2 // 程序终止(例如Ctrl-C)
#define SIGQUIT 3 // 程序退出(Ctrl-\)
#define SIGILL 4 // 执行了非法指令，或者试图执行数据段，堆栈溢出
#define SIGTRAP 5 // 断点时产生，由debugger使用
#define SIGABRT 6 // 调用abort函数生成的信号，表示程序异常
#define SIGIOT 6 // 同上，更全，IO异常也会发出
#define SIGBUS 7 // 非法地址，包括内存地址对齐出错，比如访问一个4字节的整数, 但其地址不是4的倍数
#define SIGFPE 8 // 计算错误，比如除0、溢出
#define SIGKILL 9 // 强制结束程序，具有最高优先级，本信号不能被阻塞、处理和忽略
#define SIGUSR1 10 // 未使用，保留
#define SIGSEGV 11 // 非法内存操作，与SIGBUS不同，他是对合法地址的非法访问，比如访问没有读权限的内存，向没有写权限的地址写数据
#define SIGUSR2 12 // 未使用，保留
#define SIGPIPE 13 // 管道破裂，通常在进程间通信产生
#define SIGALRM 14 // 定时信号,
#define SIGTERM 15 // 结束程序，类似温和的SIGKILL，可被阻塞和处理。通常程序如果终止不了，才会尝试SIGKILL
#define SIGSTKFLT 16 // 协处理器堆栈错误
#define SIGCHLD 17 // 子进程结束时, 父进程会收到这个信号。
#define SIGCONT 18 // 让一个停止的进程继续执行
#define SIGSTOP 19 // 停止进程,本信号不能被阻塞,处理或忽略
#define SIGTSTP 20 // 停止进程,但该信号可以被处理和忽略
#define SIGTTIN 21 // 当后台作业要从用户终端读数据时, 该作业中的所有进程会收到SIGTTIN信号
#define SIGTTOU 22 // 类似于SIGTTIN, 但在写终端时收到
#define SIGURG 23 // 有紧急数据或out-of-band数据到达socket时产生
#define SIGXCPU 24 // 超过CPU时间资源限制时发出
#define SIGXFSZ 25 // 当进程企图扩大文件以至于超过文件大小资源限制
#define SIGVTALRM 26 // 虚拟时钟信号. 类似于SIGALRM, 但是计算的是该进程占用的CPU时间.
#define SIGPROF 27 // 类似于SIGALRM/SIGVTALRM, 但包括该进程用的CPU时间以及系统调用的时间
#define SIGWINCH 28 // 窗口大小改变时发出
#define SIGIO 29 // 文件描述符准备就绪, 可以开始进行输入/输出操作
#define SIGPOLL SIGIO // 同上，别称
#define SIGPWR 30 // 电源异常
#define SIGSYS 31 // 非法的系统调用
~~~

信号量定义在 signal.h 中：
~~~ c++
int sigaction(int __signal, const struct sigaction* __new_action, struct sigaction* __old_action);
~~~
- 第一个参数 int 类型，表示需要关注的信号量
- 第二个参数 sigaction 结构体指针，用于声明当某个特定信号发生的时候，应该如何处理。
- 第三个参数也是 sigaction 结构体指针，他表示的是默认处理方式，当我们自定义了信号量处理的时候，用他存储之前默认的处理方式。

这也是指针与引用的区别，指针操作操作的都是变量本身，所以给新指针赋值了以后，需要另一个指针来记录封装了默认处理方式的变量在内存中的位置。

#### siginfo_t 结构体
~~~ c++
siginfo_t {
    int      si_signo;     /* 信号数值 */
    int      si_errno;     /* 错误值 */
    int      si_code;      /* 信号代码 */
    int      si_trapno;   /*导致硬件生成信号的陷阱号，在大多数体系结构中未使用*/
    pid_t    si_pid;       /* 发送信号的进程ID */
    uid_t    si_uid;       /*发送信号的真实用户ID */
    int      si_status;    /* 退出值或信号状态*/
    clock_t  si_utime;     /*消耗的用户时间*/
    clock_t  si_stime;     /*消耗的系统时间*/
    sigval_t si_value;     /*信号值*/
    int      si_int;       /* POSIX.1b 信号*/
    void    *si_ptr;
    int      si_overrun;   /*计时器溢出计数*/
    int      si_timerid;   /* 计时器ID */
    void    *si_addr;      /*导致故障的内存位置 */
    long     si_band;
    int      si_fd;        /* 文件描述符*/
    short    si_addr_lsb;  /*地址的最低有效位 (从Linux 2.6.32开始存在) */
    void    *si_lower;     /*地址冲突时的下限*/
    void    *si_upper;     /*地址冲突时的上限 (从Linux 3.19开始存在) */
    int      si_pkey;      /*导致的PTE上的保护密钥*/
    void    *si_call_addr; /*系统调用指令的地址*/
    int      si_syscall;   /*尝试的系统调用次数*/
    unsigned int si_arch;  /* 尝试的系统调用的体系结构*/
  }
~~~

所以，要订阅异常发生的信号，最简单的做法就是直接用一个循环遍历所有要订阅的信号，对每个信号调用sigaction()，如下示例：

~~~ c++
void init() {
	struct sigaction handler;
	struct sigaction old_signal_handlers[SIGNALS_LEN];
 	for (int i = 0; i < SIGNALS_LEN; ++i) {
 		sigaction(signal_array[i], &handler, & old_signal_handlers[i]);
 	}
} 
~~~
#### 二、捕获到Crash的位置
sigaction 结构体有一个 sa_sigaction变量，他是个函数指针，原型为：void (*)(int siginfo_t *, void *)

~~~ c++
struct sigaction {
  union {
    sighandler_t sa_handler;
    void (*sa_sigaction)(int, struct siginfo*, void*);
  };
  sigset_t sa_mask;
  int sa_flags;
  void (*sa_restorer)(void);
}; 
~~~

因此，我们可以声明一个函数，直接将函数的地址赋值给sa_sigaction

~~~ c++
void signal_handle(int code, siginfo_t *si, void *context) {

}

void init() {
 struct sigaction old_signal_handlers[SIGNALS_LEN];
 struct sigaction handler;
 handler.sa_sigaction = signal_handle;
 handler.sa_flags = SA_SIGINFO;
 for (int i = 0; i < SIGNALS_LEN; ++i) {
 sigaction(signal_array[i], &handler, & old_signal_handlers[i]);
 }
}
~~~

这样当发生 Crash 的时候就会回调我们传入的signal_handle()函数了。在signal_handle()函数中，我们得要想办法拿到当前执行的代码信息。

##### 设置紧急栈空间
如果当前函数发生了无限递归造成堆栈溢出，在统计的时候需要考虑到这种情况而新开堆栈否则本来就满了的堆栈又在当前堆栈处理溢出信号，处理肯定是会失败的。所以我们需要设置一个用于紧急处理的新栈，可以使用 sigaltstack() 在任意线程注册一个可选的栈，保留一下在紧急情况下使用的空间。（系统会在危险情况下把栈指针指向这个地方，使得可以在一个新的栈上运行信号处理函数）

~~~ c++
void signal_handle(int sig) {
 write(2, "stack overflow\n", 15);
 _exit(1);
}
unsigned infinite_recursion(unsigned x) {
 return infinite_recursion(x)+1;
}
int main() {
 static char stack[SIGSTKSZ];
 stack_t ss = {
 .ss_size = SIGSTKSZ,
 .ss_sp = stack,
 };
 struct sigaction sa = {
 .sa_handler = signal_handle,
 .sa_flags = SA_ONSTACK
 };
 sigaltstack(&ss, 0);
 sigfillset(&sa.sa_mask);
 sigaction(SIGSEGV, &sa, 0);
 infinite_recursion(0);
}
~~~

#### 三、捕获出问题的代码

signal_handle() 函数中的第三个参数 context 是uc_mcontext的结构体指针，它封装了 cpu 相关的上下文，包括当前线程的寄存器信息和奔溃时的 pc 值，能够知道崩溃时的pc，就能知道崩溃时执行的是那条指令。

不过uc_mcontext结构体的定义是平台相关的，比如我们熟知的arm、x86这种都不是同一个结构体定义，上面的代码只列出了arm架构的寄存器信息，要兼容其他架构的 cpu 在处理的时候，就得要寄出宏编译大法，不同的架构使用不同的定义，如下：


~~~ c++
uintptr_t pc_from_ucontext(const ucontext_t *uc) {
#if (defined(__arm__))
 return uc->uc_mcontext.arm_pc;
#elif defined(__aarch64__)
 return uc->uc_mcontext.pc;
#elif (defined(__x86_64__))
 return uc->uc_mcontext.gregs[REG_RIP];
#elif (defined(__i386))
 return uc->uc_mcontext.gregs[REG_EIP];
#elif (defined (__ppc__)) || (defined (__powerpc__))
 return uc->uc_mcontext.regs->nip;
#elif (defined(__hppa__))
 return uc->uc_mcontext.sc_iaoq[0] & ~0x3UL;
#elif (defined(__sparc__) && defined (__arch64__))
 return uc->uc_mcontext.mc_gregs[MC_PC];
#elif (defined(__sparc__) && !defined (__arch64__))
 return uc->uc_mcontext.gregs[REG_PC];
#else
#error "Architecture is unknown, please report me!"
#endif
}
~~~
##### pc值转内存地址


pc值是程序加载到内存中的绝对地址，绝对地址不能直接使用，因为每次程序运行创建的内存肯定都不是固定区域的内存，所以绝对地址肯定每次运行都不一致。我们需要拿到崩溃代码相对于当前库的相对偏移地址，这样才能使用 addr2line 分析出是哪一行代码。通过dladdr()可以获得共享库加载到内存的起始地址，和pc值相减就可以获得相对偏移地址，并且可以获得共享库的名字。


Dl_info info; 
if (dladdr(addr, &info) && info.dli_fname) { 
 void * const nearest = info.dli_saddr; 
 uintptr_t addr_relative = addr - info.dli_fbase; 
}


##### 获取 Crash 发生时的函数调用栈


获取函数调用栈是最麻烦的，至今没有一个好用的，全都要做一些大改动。常见的做法有四种：


第一种：直接使用系统的<unwind.h>库，可以获取到出错文件与函数名。只不过需要自己解析函数符号，同时经常会捕获到系统错误，需要手动过滤。
第二种：在4.1.1以上，5.0以下，使用系统自带的 libcorkscrew.so，5.0开始，系统中没有了libcorkscrew.so，可以自己编译系统源码中的libunwind。libunwind是一个开源库，事实上高版本的安卓源码中就使用了他的优化版替换 libcorkscrew。
第三种：使用开源库coffeecatch，但是这种方案也不能百分之百兼容所有机型。
第四种：使用 Google 的breakpad，这是所有 C/C++堆栈获取的权威方案，基本上业界都是基于这个库来做的。只不过这个库是全平台的 android、iOS、Windows、Linux、MacOS 全都有，所以非常大，在使用的时候得把无关的平台剥离掉减小体积。

#### 四、数据传回到服务器
数据回传有两种方式，一种是直接将信息写入文件，下次启动的时候直接由 Java 上报；另一种就是回调 Java 代码，让 Java 去处理。用 Java 处理的好处是 Java 层可以继续在当前上下文上加上 Java 层的各种状态信息，写入到同一个文件中，使得开发在解决 bug 的时候能更方便。

