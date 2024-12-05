---
layout: post
title: ANR 触发原理
category: accumulation
tags:
  - ANR
keywords: ANR
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Harvest%20Landscape%202.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Harvest%20Landscape%202.jpg
toc: true
---

### Input超时机制
input的超时检测机制跟service、broadcast、provider截然不同，为了更好的理解input过程先来介绍两个重要线程的相关工作：

<!--more-->
- `InputReader线程`负责通过`EventHub`(监听目录/dev/input)读取输入事件，一旦监听到输入事件则放入到`InputDispatcher`的`mInBoundQueue`队列，并通知其处理该事件；
- `InputDispatcher`线程负责将接收到的输入事件分发给目标应用窗口，分发过程使用到3个事件队列：
    - `mInBoundQueue`用于记录InputReader发送过来的输入事件；
    - `outBoundQueue`用于记录即将分发给目标应用窗口的输入事件；
    - `waitQueue`用于记录已分发给目标应用，且应用尚未处理完成的输入事件；

input的超时机制并非时间到了一定就会爆炸，而是处理后续上报事件的过程才会去检测是否该爆炸，所以更像是扫雷的过程，具体如下图所示: 
![input anr flow](/images/blogimages/2022/input_anr_flow.png)

- 1.InputReader线程通过EventHub监听底层上报的输入事件，一旦收到输入事件则将其放至mInBoundQueue队列，并唤醒InputDispatcher线程
- 2.InputDispatcher开始分发输入事件，设置埋雷的起点时间。先检测是否有正在处理的事件(mPendingEvent)，如果没有则取出mInBoundQueue队头的事件，并将其赋值给mPendingEvent，且重置ANR的timeout；否则不会从mInBoundQueue中取出事件，也不会重置timeout。然后检查窗口是否就绪(checkWindowReadyForMoreInputLocked)，满足以下任一情况，则会进入扫雷状态(检测前一个正在处理的事件是否超时)，终止本轮事件分发，否则继续执行步骤3。当应用窗口准备就绪，则将mPendingEvent转移到outBoundQueue队列
  - 对于按键类型的输入事件，则outboundQueue或者waitQueue不为空，
  - 对于非按键的输入事件，则waitQueue不为空，且等待队头时间超时500ms
- 3.当outBoundQueue不为空，且应用管道对端连接状态正常，则将数据从outboundQueue中取出事件，放入waitQueue队列
- 4.InputDispatcher通过socket告知目标应用所在进程可以准备开始干活
- 5.App在初始化时默认已创建跟中控系统双向通信的socketpair，此时App的包工头(main线程)收到输入事件后，会层层转发到目标窗口来处理
- 6.包工头完成工作后，会通过socket向中控系统汇报工作完成，则中控系统会将该事件从waitQueue队列中移除。

input超时机制为什么是扫雷，而非定时爆炸呢？是由于对于input来说即便某次事件执行时间超过timeout时长，**只要用户后续在没有再生成输入事件，则不会触发ANR**。 这里的扫雷是指当前输入系统中正在处理着某个耗时事件的前提下，后续的每一次input事件都会检测前一个正在处理的事件是否超时（进入扫雷状态），检测当前的时间距离上次输入事件分发时间点是否超过timeout时长。如果前一个输入事件，则会重置ANR的timeout，从而不会爆炸。

### ANR处理流程
不管是啥 anr ，最终都会调用到 ProcessRecord 的 appNotResponding 方法，下面来看看这个方法里面具体都做了啥：

当发生ANR时, 会按顺序依次执行:
- 输出ANR Reason信息到EventLog. 也就是说ANR触发的时间点最接近的就是EventLog中输出的am_anr信息;
- 收集并输出重要进程列表中的各个线程的traces信息，该方法较耗时; 【见小节2】
- 输出当前各个进程的CPU使用情况以及CPU负载情况;
- 将traces文件和 CPU使用情况信息保存到dropbox，即data/system/dropbox目录
- 根据进程类型,来决定直接后台杀掉,还是弹框告知用户.

ANR输出重要进程的traces信息，这些进程包含:

- firstPids队列：第一个是ANR进程，第二个是system_server，剩余是所有persistent进程；
- Native队列：是指/system/bin/目录的mediaserver,sdcard 以及surfaceflinger进程；
- lastPids队列: 是指mLruProcesses中的不属于firstPids的所有进程。

### Ref

[深入理解 Android ANR 触发原理以及信息收集过程](<https://www.cnblogs.com/huansky/p/14954020.html>)
