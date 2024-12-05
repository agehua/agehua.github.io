---
layout: post
title: Android Studio CPU Profiler使用
category: accumulation
tags:
    - Profiler
keywords: Profiler
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Field%20with%20Wheat%20Stacks.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Field%20with%20Wheat%20Stacks.jpg
toc: true
---
## Android Studio CPU Profiler使用

### CPU Profiler
谷歌的官方文档[使用 CPU Profiler 检查 CPU 活动](https://developer.android.com/studio/profile/cpu-profiler#method_traces)中，对CPU Profiler的功能描述是：
<!--more-->
- 系统跟踪数据：捕获精细的详细信息，以便您检查应用与系统资源的交互情况。
- 方法和函数跟踪数据：对于应用进程中的每个线程，您可以了解一段时间内执行了哪些方法 (Java) 或函数 (C/C++)，以及每个方法或函数在其执行期间消耗的 CPU 资源。您还可以使用方法和函数跟踪数据来识别调用方和被调用方。调用方是指调用其他方法或函数的方法或函数，而被调用方是指被其他方法或函数调用的方法或函数。您可以使用此信息来确定哪些方法或函数负责调用常常会消耗大量资源的特定任务，并优化应用的代码以避免不必要的工作。

本文是我想对App的启动过程进行优化，先总结对Profiler工具的使用。

### 如何获取 trace 数据

CPU Profiler可以对trace文件进行分析，获取trace文件的方式，大概有3种：
- 1.启用调试设备，在Profiler面板左上角点击加号➕，第二项是已连接的可调试设备，点击设备，选择要调试应用的进程。
- 2.还是上面加号➕，点击选择第一项 load from file...，表示用本地的文件，关于文件的生成有两种方式：
    - 1.在搭载 Android 9（API 级别 28）或更高版本的设备上，您可以使用一个名为“系统跟踪”的系统应用来记录设备上的系统跟踪数据，
    详细介绍在[这里](https://developer.android.com/topic/performance/tracing/on-device?hl=zh_cn)
    - 2.在Android 8.0（API 级别 26）或更高版本的设备上，使用 Debug API，掉用startMethodTracing(String tracePath) 开始记录数据；调用 stopMethodTracing()停止记录数据。
    结束后会在tracePath（默认会在sd卡根目录创建`<trace-name>`.trace文件）。

`注意Debug API 不要和其他两种方式同时使用`


### 如何生成app启动过程中的 trace数据
- 1.依次选择 Run > Edit Configurations。
- 2.在 Profiling 标签中，勾选 Start recording a method trace on startup 旁边的复选框。
- 3.从菜单中选择 CPU 记录配置。
- 4.点击 Apply。
- 5.依次选择 Run > Profile，将您的应用部署到搭载 Android 8.0（API 级别 26）或更高版本的设备上。

### 如何使用CPU Profiler 进行分析
成功导入或直接调试出数据后，Profiler面板会如下图所示(这里只截了一部分)：
![Cpu Profiler](/images/blogimages/2020/cpu_profiler.png)

图中最上面是整个trace文件记录的时间段，默认是选中了全部。
下面是我们的应用里使用到CPU的各个线程，其中记录显示的颜色有三种：
- 1.橙色，是指对系统 API 的调用
- 2.绿色，对应用自有方法的调用
- 3.蓝色，为对第三方 API（包括 Java 语言 API）的调用

点击mian，对应应用的主线程，Profiler面板的右侧会展示这个线程的数据，分三个维度：
- Top Down 标签显示一个调用列表，在该列表中展开方法或函数节点会显示它的被调用方

- Flame Chart 标签页提供一个倒置的调用图表，用来汇总完全相同的调用堆栈。也就是说，将具有相同调用方顺序的完全相同的方法或函数收集起来，并在火焰图中将它们表示为一个较长的横条。这样更方便您查看哪些方法或函数消耗的时间最多。不过，这也意味着，横轴不代表时间轴，而是表示执行每个方法或函数所需的相对时间。

- Bottom Up 标签页显示一个调用列表，在该列表中展开函数或方法的节点会显示它的调用方。Bottom Up 标签页用于按照占用的 CPU 时间由多到少（或由少到多）的顺序对方法或函数排序

### App启动流程分析
在Top Down标签页下，有一个搜索框，可以输入想要重点关注的方法调用。
对于App启动流程，我建议有几处可以关注：
- **attach()** (android.app.Application)，这里会回调application的attachBaseContext()方法。
- **callApplicationOnCreate()** (android.app.Instrumentation)，这里会回调application的onCreate()方法，可以检查以上两处有无耗时操作，比如各种sdk的初始化。
- 搜索框输入第一个启动的activity，类似 `WelcomeActivity` 这种，查找关于这个activity的onCreate()和onResume()中的耗时方法。

暂时只想到这几点，如有遗漏，欢迎补充~



