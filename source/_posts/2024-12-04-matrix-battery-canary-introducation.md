---
layout: post
title: Matrix Battery 介绍和使用
category: accumulation
tags:
  - Matrix
keywords: Matrix, Battery, APM
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/La%20Berceuse%20Augustine%20Roulin%203.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/La%20Berceuse%20Augustine%20Roulin%203.jpg
toc: true
---

App耗电问题是一个影响用户体验，甚至影响app使用率的一个重要的性能问题。相比Crash、Anr，耗电排查是一个需要综合各种因素来最终确定问题所在的复杂流程。同时又因为耗电受影响的范围大、不同手机衡量标准又不太统一导致该问题一直是一个令人头痛的疑难杂症。

本文通过介绍耗电基本知识和 Matrix Battery的基本使用，来总结下面对耗电问题应该如何排查和定位

<!--more-->
### Android 系统电量统计服务
Android 系统的电量统计工作，是由一个叫 BatteryStatsService 的系统服务完成的。

先了解一下其中四个比较关键的角色：

- 功率：power_profile.xml，Android 系统使用此文件来描述设备各个硬件模块的额定功率，包括上面提到的多档位功率和 CPU 电量算需要到的各种参数值。
- 时长：StopWatch & SamplingCounter，其中 StopWatch ⏱ 是用来计算 App 各种硬件模块的使用时长，而 SamplingCounter 则是用来采样统计 App 在不同 CPU Core 和不同 CpuFreq 下的工作时长。
- 计算：PowerCalculators，每个硬件模块都有一个相应命名的 PowerCalculator 实现，主要是用来完成具体的电量统计算法。
- 存储：batterystats.bin，电量统计服务相关数据的持久化文件。

#### BatteryStatsService 时长统计流程
BatteryStatsService 框架的核心是 ta 持有的一个叫 BatteryStats 的类，BatteryStats 又持有一个 Uid [] 数组，每一个 Uid 实例实际上对应一个 App，当我们安装或者卸载 App 的时候，BatteryStats 就会更新相应的 Uid 元素以保持最新的映射关系。同时 BatteryStats 持有一系列的 StopWatch 和 SamplingCounter，当 App 开始使用某些硬件模块的功能时，BatteryStats 就会调用相应 Uid 的 StopWatch 或 SamplingCounter 来统计其硬件使用时长。

这里以 Wifi 模块来举例：当 App 通过 WifiManager 系统服务调用 Wifi 模块开始扫描的时候，实际上会通过 `WifiManager#startScan() --> WifiScanningServiceImp --> BatteryStatsService#noteWifiScanStartedFromSource() --> BatteryStats#noteWifiScanStartedLocked(uid) `等一连串的调用，通知 BatteryStats 开启 App 相应 Uid 的 Wifi 模块的 StopWatch 开始计时。当 App 通过 WifiManager 停止 Wifi 扫描的时候又会通过类似的流程调用 BatteryStats#noteWifiScanStoppedLocked (uid) 结束 StopWatch 的计时，这样一来就通过 StopWatch 完成 App 对 Wifi 模块使用时长的统计。

####  BatteryStatsService 功耗计算流程
具体电量计算方面，BatteryStats 是通过 ta 依赖的一个 BatteryStatsHelper 的辅助类来完成的。BatteryStatsHelper 通过组合使用 Uid 里的时长数据、PoweProfile 里的功率数据（power_profile.xml 的解析实例）以及具体各个模块的 PowerCalculator 算法，计算出每一个 App 的综合电量消耗，并把计算结果保存在 BatterySipper [] 数组里（按计算值从大到小排序）。

还是以 Wifi 模块来举例：当需要计算 App 电量消耗的时候，BatteryStats 会通过调用 `BtteryStatsHelper#refreshStats() --> #processAppUsage()` 来刷新 BatterySipper [] 数组以计算最新的 App 电量消耗数据。而其中 Wifi 模块单独的电量统计就是在 processAppUsage 方法中通过 WifiPowerCalculator 来完成的：Wifi 模块电量 = PowerProfile 预置的 Idle 功率 + Uid 统计的 Wifi Idle 时间 + 上行功率 × 上行时间 + 下行功率 × 下行时间。

#### Android 系统 App 耗电排行
通过以上分析，我们其实已经知道 Android 系统 App 耗电排行是通过读取 BatteryStatsHelper 里的 BatterySipper [] 数据来实现排行的。一般情况下，BatteryStats 的统计口径是 STATS_SINCE_CHARGED, 也就距离上次设备充满电到现在的状态。不过个别 OEM 系统上这里的统计细节有所不同，有的 Android 设备系统可以显示最近数天甚至一周以上的 App 的电量统计数据，具体实现细节不得而知，姑且推断是根据 BatteryStatsHelper 自行定制的服务。

#### 线程监控
实际上，我们除了通过 SystemClock.currentThreadTimeMillis () 来获取当前 Java 线程的工作时间此外，并没有直接的办法能够直接获取 App 所有线程的工作时长和状态，幸运的是 Linux 的 proc 命令可以给我们提供一些帮助。

Linux 命令 `proc/[pid]/stat 和 proc/[pid]/task/[tid]/stat` 可以 Dump 当前 App 进程和线程的统计信息。

~~~ javascript
> cat /proc/<mypid>/task/<tid>/stat10966 (terycanary.test) S 699 699 0 0 -1 1077952832 6187 0 0 0 22 2 0 0 20 0 17 0 9087400 5414273024 24109 18446744073709551615 421814448128 421814472944 549131058960 0 0 0 4612 1 1073775864 1 0 0 17 7 0 0 0 0 0 421814476800 421814478232 422247952384 549131060923 549131061022 549131061022 549131063262 0
~~~
这里比较关键的数据是 **进程 / 线程名**、**进程 / 线程状态**，以及第 13 - 16 位的 **utime**、**stime**、**cutime** 和 **cstime。utime 和 stime 分别是进程 / 线程的用户时间和系统时间，而 cutime/cstime 是当前进程等在子进程的时间（在 Android 进程上大都是 0）。实际上我们对这些数据内容也不完全是陌生的，Logcat 里一些线程相关的 syslog 也有类似的输出。

在这里有一点需要单独拎出来讲：utime 和 stime 具体代表什么意义呢？ 我们已经知道它们是表示线程的工作时长，但实际上其单位 jiffy 并不是一个时间的单位，而是一个**频率**的单位！

Android Linux 上，100 Jiffies ≈ 1 Second

所以我们可以记住一个比较重要的结论：在 Android 系统上，Jiffy 和 Millis 的换算关系大概是 1 比 10。（100 Hz 是一个 Linux 系统的编译参数，在不同的 Linux 版本上这个值可能是不同的。）


### BatteryCanary使用
BatteryCanary 最核心的功能是通过监控线程异常来定位 App 的耗电 Bug，主要包括线程电量统计、堆栈信息和线程池问题细分等.

通过BatteryCanaryInitHelper这个类，
~~~ java
sBatteryConfig = new BatteryMonitorConfig.Builder()
                // Thread Activities Monitor
                .enable(JiffiesMonitorFeature.class)
                .enableStatPidProc(true)
                .greyJiffiesTime(3 * 1000L)
                .enableBackgroundMode(false)
                .backgroundLoopCheckTime(30 * 60 * 1000L)
                .enableForegroundMode(true)
                .foregroundLoopCheckTime(20 * 60 * 1000L)
                .setBgThreadWatchingLimit(5000)
                .setBgThreadWatchingLimit(8000)

                // CPU Stats
                .enable(CpuStatFeature.class)

                // App & Device Status Monitor For Better Invalid Battery Activities Configure
                .setOverHeatCount(1024)
                .enable(DeviceStatMonitorFeature.class)
                .enable(AppStatMonitorFeature.class)
                .setSceneSupplier(new Callable<String>() {
                    @Override
                    public String call() {
                        return "Current AppScene";
                    }
                })

                // AMS Activities Monitor:
                // alarm/wakelock watch
                .enableAmsHook(true)
                .enable(AlarmMonitorFeature.class)
                .enable(WakeLockMonitorFeature.class)
                .wakelockTimeout(2 * 60 * 1000L)
                .wakelockWarnCount(3)
                .addWakeLockWhiteList("Ignore WakeLock TAG1")
                .addWakeLockWhiteList("Ignore WakeLock TAG2")
                // scanning watch (wifi/gps/bluetooth)
                .enable(WifiMonitorFeature.class)
                .enable(LocationMonitorFeature.class)
                .enable(BlueToothMonitorFeature.class)
                .enable(NotificationMonitorFeature.class)

                // BatteryStats
                .enable(BatteryStatsFeature.class)
                .setRecorder(new BatteryRecorder.MMKVRecorder(mmkv))
                .setStats(new BatteryStats.BatteryStatsImpl())
                .enable(HealthStatsFeature.class)

                // Lab Feature:
                // network monitor
                // looper task monitor
                .enable(TrafficMonitorFeature.class)
                .enable(LooperTaskMonitorFeature.class)
                .addLooperWatchList("main")
                .useThreadClock(false)
                .enableAggressive(true)
                .enable(TopThreadFeature.class)

                // Monitor Callback
                .setCallback(new BatteryStatsListener())
                .build();
~~~

总结下BatteryCanary 的功能有：
- 监控现场CPU使用情况，使用Jiffies 衡量
- CPU状态监控
- 设备状态监控
- App状态监控
- Alarm、WakeLock监控
- Wifi使用监控
- Location使用监控
- Bluetooth使用监控
- Notification使用监控
- 电池状态监控
  - 使用mmkv记录电池状态
  - 电池健康状态，包括温度
- 实验功能
  - 流量使用监控
  - looper任务监控（主线程）
  - Top线程监控

初次运行Demo可能会有疑惑，不知道都有哪些功能，简单介绍下：

- 第一个按钮：Dump BatteryStats Report
  - 对应 BatteryStatsFeature，使用compositeMonitors dump数据，在log里打印，并写入BatteryStatsFeature 的BatteryRecords 里
  ~~~ java
  // 将数据打印到日志
        final Printer printer = new Printer();
        printer.writeTitle();
        new BatteryMonitorCallback.BatteryPrinter.Dumper().dump(compositeMonitors, printer);
        printer.writeEnding();

        // 将数据写入 BatteryStatsFeature 的 BatteryRecorder里
        BatteryStatsFeature statsFeat = BatteryCanary.getMonitorFeature(BatteryStatsFeature.class);
        if (statsFeat != null) {
            statsFeat.statsMonitors(compositeMonitors);
        }
  ~~~
- 第二个：Checkout BatteryStats Report
  - 同样 BatteryStatsFeature，获取当前进程的 BatteryRecords
  ~~~ java
    BatteryCanary.getMonitorFeature(BatteryStatsFeature.class, new Consumer<BatteryStatsFeature>() {
            @Override
            public void accept(BatteryStatsFeature batteryStatsFeature) {
                List<BatteryRecord> records = batteryStatsFeature.readRecords(dayOffset, mProc);
                if (mFilter != null) {
                    records = mFilter.filtering(records);
                }
                BatteryRecords batteryRecords = new BatteryRecords();
                batteryRecords.date = BatteryStatsFeature.getDateString(dayOffset);
                batteryRecords.records = records;
                add(batteryRecords);
            }
        });
  ~~~
- 第三个：Checkout BatteryStats Report
  - 同第二个，只是展示不同进程的数据
- 第四个、第五个：Show/Close TOP indicator
  - 展示或取消 top indicator
  - 对应 TopThreadFeature，获取下面各个feature的数据并展示
  ~~~ java
  BatteryCanary.getMonitorFeature(TopThreadFeature.class, new Consumer<TopThreadFeature>() {
      @Override
      public void accept(TopThreadFeature topThreadFeat) {
          topThreadFeat.top(seconds, new Supplier<CompositeMonitors>() {
              @Override
              public CompositeMonitors get() {
                  CompositeMonitors monitors = new CompositeMonitors(mCore, CompositeMonitors.SCOPE_TOP_INDICATOR);
                  monitors.metric(JiffiesMonitorFeature.UidJiffiesSnapshot.class);
                  monitors.metric(CpuStatFeature.CpuStateSnapshot.class);
                  monitors.metric(CpuStatFeature.UidCpuStateSnapshot.class);
                  monitors.metric(HealthStatsFeature.HealthStatsSnapshot.class);
                  monitors.metric(TrafficMonitorFeature.RadioStatSnapshot.class);
                  monitors.sample(DeviceStatMonitorFeature.CpuFreqSnapshot.class, 500L);
                  monitors.sample(DeviceStatMonitorFeature.BatteryCurrentSnapshot.class, 500L);
                  monitors.sample(TrafficMonitorFeature.RadioBpsSnapshot.class, 500L);
                  return monitors;
              }
          }, new ContinuousCallback() {
              @Override
              public boolean onGetDeltas(final CompositeMonitors monitors, long windowMillis) {
                  refresh(monitors);
                  if (mRootView == null || !mRunningRef.get(hashcode, false)) {
                      return true;
                  }
                  return false;
              }
          });
      }
  });
  ~~~

综上是 BatteryCanary 的主要功能，后续有时间再开一篇介绍下其中原理


### 参考
https://cloud.tencent.com/developer/article/1855937