---
layout: post
title:  Android Binder机制分析（三）
category: accumulation
tags:
  - AIDL
  - Binder
keywords: AIDL, Binder
banner: https://cdn.conorlee.top/Great%20Peacock%20Moth.jpg
thumbnail: https://cdn.conorlee.top/Great%20Peacock%20Moth.jpg
toc: true
---

### 背景
[上篇文章](/2017/07/10/android-binder-principle2/
)分析到了Binder机制，同时介绍了Android Framework层系统服务的binder使用。本篇文章分析下Binder的底层实现，以及为什么Binder只需要一次内存拷贝。

<!--more-->
### Binder到底是什么？
Android系统内核是Linux，每个进程有自己的虚拟地址空间，在32位系统下最大是4GB，其中3GB为用户空间，1GB为内核空间；每个进程用户空间相对独立，而内核空间是一样的，可以共享，如下图
![内核空间](/images/blogimages/2022/binder_kernel_space.png)

Linux驱动运行在内核空间，狭义上讲是系统用于控制硬件的中间程序，但是归根结底它只是一个程序一段代码，所以具体实现并不一定要和硬件有关。Binder就是将自己注册为一个misc类型的驱动，不涉及硬件操作，同时自身运行于内核中，所以可以当作不同进程间的桥梁实现IPC功能。

Linux最大的特点就是一切皆文件，驱动也不例外，所有驱动都会被挂载在文件系统dev目录下，Binder对应的目录是/dev/binder，注册驱动时将open release mmap等系统调用注册到Binder自己的函数，这样的话在用户空间就可以通过系统调用以访问文件的方式使用Binder。

> 进一步关于内核空间和用户空间的区别，可以看这篇文章：<https://zhuanlan.zhihu.com/p/343597285>

### Binder的简略通讯过程
一个进程如何通过binder和另一个进程通讯？最简单的流程如下

接收端进程开启一个专门的线程，通过系统调用在binder驱动（内核）中先注册此进程（创建保存一个bidner_proc），驱动为接收端进程创建一个任务队列（biner_proc.todo）
接收端线程开始无限循环，通过系统调用不停访问binder驱动，如果该进程对应的任务队列有任务则返回处理，否则阻塞该线程直到有新任务入队
发送端也通过系统调用访问，找到目标进程，将任务丢到目标进程的队列中，然后唤醒目标进程中休眠的线程处理该任务，即完成通讯


### Binder的一次拷贝
众所周知Binder的优势在于一次拷贝效率高，众多博客已经说烂了，那么什么是一次拷贝，如何实现，发生在哪里，这里尽量简单地解释一下。

上面已经说过，不同进程通过在内核中的Binder驱动来进行通讯，但是用户空间和内核空间是隔离开的，无法互相访问，他们之间传递数据需要借助`copy_from_user`和`copy_to_user`两个系统调用，把用户/内核空间内存中的数据拷贝到内核/用户空间的内存中，这样的话，如果两个进程需要进行一次单向通信则需要进行两次拷贝，如下图。

![两次内存拷贝](/images/blogimages/2022/binder_memory_copy.png)

Binder单次通信只需要进行一次拷贝，因为它使用了**`内存映射`**，**将一块物理内存（若干个物理页）分别映射到接收端用户空间和内核空间**，达到用户空间和内核空间共享数据的目的。

发送端要向接收端发送数据时，内核直接通过`copy_from_user`将数据拷贝到内核空间映射区，此时由于共享物理内存，接收进程的内存映射区也就能拿到该数据了，如下图:

![两次内存拷贝](/images/blogimages/2022/binder_one_time_copy.jpeg)

> 用户空间通过mmap系统调用，调用到Binder驱动中binder_mmap函数进行内存映射

### Binder套件架构
内核层的Binder驱动已经提供了IPC功能，不过还需要在framework native层提供一些对于驱动层的调用封装，使framework开发者更易于使用，由此封装出了native Binder；同时，由于framework native层是c/c++语言实现，对于应用开发者，需要更加方便的Java层的封装，衍生出Java Binder；最后在此之上，为了减少重复代码的编写和规范接口，在Java Binder的基础上又封装出了AIDL。经过层层封装，在使用者使用AIDL时对于Binder基本上是无感知的。

这里贴一张架构图。
![两次内存拷贝](/images/blogimages/2022/binder_structure.jpeg)

Native层：BpBinder代表服务端Binder的一个代理，内部有一个成员mHandle就是服务端Binder在驱动层的句柄，客户端通过调用BpBinder::transact传入该句柄，经过驱动层和服务端BBinder产生会话，最后服务端会调用到BBinder::onTransact。在这里两者之间通过约定好的code来标识会话内容。

Java层：Java层是对native层的封装，本质没有什么区别，。

AIDL：AIDL生成的代码对于Binder进行了进一步封装，`<接口>.Stub对应服务端Binder，<接口>.Stub.Proxy标识客户端`，内部持有一个mRemote实例（BinderProxy），aidl根据定义的接口方法生成若干个TRANSACTION_<函数名> code常量，两端Binder通过这些code标识解析参数，调用相应接口方法。换言之AIDL就是对BinderProxy.transact和Binder.onTransact进行了封装，使用者不必再自己定义每次通讯的code以及参数解析。


### Ref
[Binder概述，快速了解Binder体系](https://zhuanlan.zhihu.com/p/392245183)
