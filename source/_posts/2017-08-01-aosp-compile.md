---
layout: post
title:  Mac下Android源码编译（AOSP）
category: accumulation
tags:
  - AOSP
  - sdk compile
keywords: aosp, sdk compile
banner: http://obxk8w81b.bkt.clouddn.com/Child%20with%20Orange.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Child%20with%20Orange.jpg
toc: true
---


### Android源码下载

Mac上源码下载没有太大问题，主要参考下面两篇文章：
[自己动手编译最新Android源码及SDK](http://blog.csdn.net/dd864140130/article/details/51718187)
[谷歌下载源代码](https://source.android.com/source/downloading)

Mac上创建磁盘空间，直接区分大小写
~~~ C++
hdiutil resize -size 100g ~/android.dmg.sparseimage
~~~
我编译的是7.1.1版本的系统源码，所以给了100G空间


### Android Build系统
<!--more-->
代码下载完成以后，不着急编译，先了解下Android Build系统，看下IBM的这篇文章：[Android Build系统](https://www.ibm.com/developerworks/cn/opensource/os-cn-android-build/index.html)


Android Build 系统用来编译 Android 系统，Android SDK 以及相关文档。该系统主要由 Make 文件，Shell 脚本以及 Python 脚本组成，其中最主要的是 Make 文件。

#### 主要编译步骤
初始化编译环境：
~~~ C++
source build/envsetup.sh
~~~
这句话的意思是引入build/envsetup.sh脚本。该脚本的作用是初始化编译环境，并引入一些辅助的 Shell 函数，这其中就包括下面使用 lunch 函数。
~~~ C++
lunch
~~~
执行完lunch函数，可以看到下面的结果：
~~~ C++
You're building on Darwin

Lunch menu... pick a combo:
     1. aosp_arm-eng
     2. aosp_arm64-eng
     3. aosp_mips-eng
     4. aosp_mips64-eng
     5. aosp_x86-eng
     6. aosp_x86_64-eng
     7. full_fugu-userdebug
     8. aosp_fugu-userdebug
     9. mini_emulator_arm64-userdebug
     10. m_e_arm-userdebug
     11. m_e_mips-userdebug
     12. m_e_mips64-eng
     13. mini_emulator_x86-userdebug
     14. mini_emulator_x86_64-userdebug
     15. aosp_dragon-userdebug
     16. aosp_dragon-eng
     17. aosp_marlin-userdebug
     18. aosp_sailfish-userdebug
     19. aosp_flounder-userdebug
     20. aosp_angler-userdebug
     21. aosp_bullhead-userdebug
     22. hikey-userdebug
     23. aosp_shamu-userdebug

Which would you like? [aosp_arm-eng]
~~~
这里的lunch是选择编译目标，直接输入数字就代表选中
> 编译目标格式说明:
  编译目标的格式:BUILD-BUILDTYPE,比如上面的第一个，aosp_arm-eng的BUILD是aosp_arm,BUILDTYPE是eng.

那么选择哪个编译目标呢？
这里[StackOverflow上有提问](https://stackoverflow.com/questions/27572817/aosp-build-with-fastest-emulator-boot)，建议编译**aosp_x86-userdebug** with HAXM installed

但是我的lunch并没有列出这个编译目标：

所以这里不使用lunch选择，而是使用choosecombo命令：

        记住也要先执行envsetup.sh脚本哟
~~~ C++
choosecombo
Build type choices are:
     1. release
     2. debug

Which would you like? [1]
//这里直接输入2，选择第二个


Which product would you like? [aosp_arm]
//这里并没有对应的BUILD，所以直接自己输入：aosp_x86

Variant choices are:
     1. user
     2. userdebug
     3. eng
//这里选择第二个，直接输入2
~~~
这样就选择好了编译目标，对应的Terminal的顶部标题也已经改变
然后调用“make -j4”进行编译就可以了
> make 的参数“-j”指定了同时编译的 Job 数量，这是个整数，该值通常是编译主机 CPU 支持的并发线程总数的 1 倍或 2 倍（例如：在一个 4 核，每个核支持两个线程的 CPU 上，可以使用 make -j8 或 make -j16）

下面介绍几个常用到的命令：
当电脑磁盘空间不够用时，用下面的命令，遍历大文件
~~~ C++
sudo ncdu //查看硬盘中的大文件
~~~

如果make失败，或是想换一个BUILD类型，使用下面的命令：
~~~ C++
make clobber //清理out目录下的文件
~~~

### 调试Android源码：
参考这篇文章：[自己动手调试Android源码](http://blog.csdn.net/dd864140130/article/details/51815253)

将Android源码导入Android Studio：
~~~ C++
source build/envsetup.sh
mmm development/tools/idegen/
sudo ./development/tools/idegen/idegen.sh
~~~
上面代码执行完后，会在源码目录下生成IEDA工程配置文件: android.ipr,android.iml及android.iws.
可以发现android.ipr文件的应用图标应该改成了Android Studio样式。

> 但这里不用Android Studio来打开工程，而是使用IntelliJ来查看和DebugAndroid源码，具体参考这篇文章[使用 IntelliJ 查看 Android 源码](http://www.jianshu.com/p/1d1b8d0de1ed)


#### 单独编译Android源代码工程的模块
在Android源码找到目录/packages/experimental，在这个目录下有Google提供基于前面提到的Android BUILD模式构建的apk工程。
如下图：

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/aosp_make_single_module.png)

我们选择一个工程：MultiPackageApk，要把它安装到模拟器上需要执行下面的命令：
~~~ C++
source build/envsetup.sh
mmm packages/experimental/MultiPackageApk    
make snod  
~~~
重启下模拟器，刚才安装的应用就会出现了

单独编译源代码工程出错：

~~~ C++
ninja: error: 'out/target/common/obj/JAVA_LIBRARIES/core-junit_intermediates/classes.dex.toc', needed by 'out/target/common/obj/APPS/MultiPackageApk_intermediates/with-local/classes.dex', missing and no known rule to make it
make: *** [ninja_wrapper] Error 1
~~~
解决方案：应该选择好编译环境后（用lunch或choosecombo命令），再执行单独编译源码工作，最后别忘了执行**make snod**


#### make failed to build some targets (4 seconds) ####

#### 使用 IntelliJ Debug Android 源码
参考这篇文章：[使用 IntelliJ Debug Android 源码](http://www.jianshu.com/p/7c2ab3d9498c)

文中，有一个地方跟我的电脑上不太一样：
#### 打开 monitor 选择 debug 进程
我电脑上的monitor不是用命令行打开，而是有monitor.app。
位置在：/Library/Android_sdk/tools/lib/monitor-x86_64/monitor.app
如下图：

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/aosp_monitor_position.png)

到这里Android源码部分的内容就整理完了，下篇文章利用Android源码分析下**Activity的启动流程**。
