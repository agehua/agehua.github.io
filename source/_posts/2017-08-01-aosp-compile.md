---
layout: post
title:  Mac下Android源码编译（AOSP）
category: accumulation
tags:
  - AOSP
  - sdk compile
keywords: aosp, sdk compile
banner: http://cdn.conorlee.top/Child%20with%20Orange.jpg
thumbnail: http://cdn.conorlee.top/Child%20with%20Orange.jpg
toc: true
---

## Mac下源码编译
作为一名安卓开发人员，有能力阅读Android源码，是向高手进阶的重要一步。
本文介绍在Mac系统下，进行Android源码编译需要的操作。
### 编译需要的步骤

android源码编译的四个流程: 1.搭建编译环境; 2.源码下载; 3.编译源码; 4运行.
下文也将按照该流程讲述.

### 搭建编译环境
使用本地工作环境来编译 Android 源文件，需要使用 Linux 或 Mac OS。目前不支持在 Windows 环境下进行编译。

<!--more-->
系统要求和JDK要求等，参考[Google源码编译要求](https://source.android.com/source/requirements)

#### 设置Mac OS编译环境
在默认安装过程中，Mac OS 会在一个保留大小写但不区分大小写的文件系统中运行。Git 并不支持此类文件系统。
因此，我们建议您始终在区分大小写的文件系统中对 AOSP 源文件进行操作。使用下文中介绍的磁盘映像可以非常轻松地做到这一点。

#### 创建区分大小写的磁盘映像
通过 shell 使用以下命令创建磁盘映像：
~~~ JavaScript
hdiutil create -type SPARSE -fs 'Case-sensitive Journaled HFS+' -size 40g ~/android.dmg
~~~
如果您以后需要更大的存储卷，还可以使用以下命令来调整稀疏映像的大小：
~~~ C++
hdiutil resize -size 100g ~/android.dmg.sparseimage
~~~
我编译的是7.1.1版本的系统源码，狠心之下给了100G空间

对于存储在主目录下的名为 android.dmg 的磁盘映像，您可以向 ~/.bash_profile 中添加辅助函数：

~~~ JavaScript
# mount the android file image
function mountAndroid { hdiutil attach ~/android.dmg -mountpoint /Volumes/android; }
# unmount the android file image
function umountAndroid() { hdiutil detach /Volumes/android; }
~~~
- 1.要在执行 mountAndroid 时装载磁盘映像
- 2.要在执行 umountAndroid 时卸载磁盘映像

> 注意：如果系统创建的是 .dmg.sparseimage 文件，请将 ~/android.dmg 替换成 ~/android.dmg.sparseimage。

装载 android 存储卷后，您将在其中开展所有工作。您可以像对待外接式存储盘一样将其弹出（卸载）。

### Android源码下载
Android源码的下载要使用到repo工具，这部分主要参考了下面两篇文章：
[CSDN——自己动手编译最新Android源码及SDK](http://blog.csdn.net/dd864140130/article/details/51718187)
[Source-Android——谷歌下载源代码](https://source.android.com/source/downloading)

#### repo工具下载及安装

repo工具负责下载AOSP源码，总结来说，repo就是这么一种工具，由一系列python脚本组成，通过调用Git命令实现对AOSP项目的管理.

在命令行工具里，依次执行下面的命令，实现repo工具的下载和安装
~~~ JavaScript
mkdir ~/bin
PATH=~/bin:$PATH
curl https://storage.googleapis.com/git-repo-downloads/repo > ~/bin/repo
chmod a+x ~/bin/repo
~~~
> 关于repo工具和AOSP项目的组织方式，感兴趣的看上面的那篇CSDN博客

> curl是一个利用URL规则在命令行下工作的文件传输工具，可以说是一款很强大的http命令行工具。它支持文件的上传和下载，是综合传输工具，但按传统，习惯称curl为下载工具。

#### 创建源码目录

在上面创建好的磁盘映像里，创建一个目录，用来存放repo工具下载下来的代码。后面编译出的产物也都放在这里。
~~~ JavaScript
mkdir source
cd source
~~~

#### 初始化仓库
我们将上面的source文件夹作为仓库，现在需要来初始化这个仓库了。通过执行初始化仓库命令可以获取AOSP项目master上最新的代码并初始化该仓库，命令如下（需要特殊的翻墙技巧）:

~~~ JavaScript
repo init -u https://android.googlesource.com/platform/manifest
~~~

如果执行上面的命令，提示无法连接到 gerrit.googlesource.com，`Operation timed out`。那么我们需要编辑 ~/bin/repo文件，找到REPO_URL这一行,然后将其内容修改为:
~~~ JavaScript
REPO_URL = 'https://gerrit-google.tuna.tsinghua.edu.cn/git-repo'
~~~

---
`注意:` 上面的地址已经不能用了，要使用下面的地址，具体详情参看：[Git Repo 镜像使用帮助](https://mirror.tuna.tsinghua.edu.cn/help/git-repo/)
~~~ JavaScript
REPO_URL = 'https://mirrors.tuna.tsinghua.edu.cn/git/git-repo/'
~~~
---

然后在使用下面的命令初始化仓库：
~~~ JavaScript
repo init -u https://aosp.tuna.tsinghua.edu.cn/platform/manifest
~~~
> 注意，上面不带参数的manifest命令用于获取master上最新的代码。

可以通过-b参数指定获取某个特定的android版本
~~~ JavaScript
repo init -u https://aosp.tuna.tsinghua.edu.cn/platform/manifest -b android-4.0.1_r1
~~~

AOSP项目当前所有的分支列表参看：https://source.android.com/source/build-numbers#source-code-tags-and-builds

#### 同步源码到本地
初始化仓库之后,就可以开始正式同步代码到本地了,命令如下:
~~~ JavaScript
repo sync
~~~
以后如果需要同步最新的远程代码到本地，也只需要执行该命令即可。在同步过程中,如果因为网络原因中断,使用该命令继续同步即可。

同步过程大概需要5个小时，看你的网速了 :)

### Android Build系统

代码下载完成以后，不着急编译，先了解下Android Build系统。

Android Build 系统用来编译 Android 系统，Android SDK 以及相关文档。该系统主要由 Make 文件，Shell 脚本以及 Python 脚本组成，其中最主要的是 Make 文件。
Build 系统中最主要的处理逻辑都在 Make 文件中，而其他的脚本文件只是起到一些辅助作用。
整个 Build 系统中的 Make 文件可以分为三类：

- 第一类是 Build 系统核心文件，此类文件定义了整个 Build 系统的框架，而其他所有 Make 文件都是在这个框架的基础上编写出来的。
- 第二类是针对某个产品（一个产品可能是某个型号的手机或者平板电脑）的 Make 文件，这些文件通常位于 device 目录下，该目录下又以公司名以及产品名分为两级目录。对于一个产品的定义通常需要一组文件，这些文件共同构成了对于这个产品的定义。例如，/device/sony/it26 目录下的文件共同构成了对于 Sony LT26 型号手机的定义。
- 第三类是针对某个模块的 Make 文件。整个系统中，包含了大量的模块，每个模块都有一个专门的 Make 文件，这类文件的名称统一为“Android.mk”，该文件中定义了如何编译当前模块。Build 系统会在整个源码树中扫描名称为“Android.mk”的文件并根据其中的内容执行模块的编译。

更多编译系统的内容，请看IBM的这篇文章：[Android Build系统](https://www.ibm.com/developerworks/cn/opensource/os-cn-android-build/index.html)

### 编译 Android 系统

#### 初始化编译环境
执行：
~~~ C++
source build/envsetup.sh
~~~
这句话的意思是引入build/envsetup.sh脚本。该脚本的作用是初始化编译环境，并引入一些辅助的 Shell 函数，这其中就包括下面使用 lunch 函数。
#### 选择编译目标
执行：
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
  编译目标的格式:BUILD-BUILDTYPE,比如上面的第一个，aosp_arm-eng的BUILD是aosp_arm，BUILDTYPE是eng.

编译目标的选择，决定了模拟器的响应速度，那么选择哪个编译目标呢？
这里[StackOverflow上有提问](https://stackoverflow.com/questions/27572817/aosp-build-with-fastest-emulator-boot)，建议编译**aosp_x86-userdebug** with HAXM installed

但是我的lunch并没有列出这个编译目标：

所以这里可以使用choosecombo命令,`记住也要先执行envsetup.sh脚本哟`：

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

#### 编译

选择好编译目标后，调用“make -j*”进行编译就可以了.

可以看到线程数为8.
> make 的参数“-j”指定了同时编译的 Job 数量，这是个整数，该值通常是编译主机 CPU 支持的并发线程总数的 1 倍或 2 倍（例如：在一个 4 核，每个核支持两个线程的 CPU 上，可以使用 make -j8 或 make -j16）

可以使用下面的命令查看本机CPU的内核数与线程数：

~~~ C++
$ sysctl machdep.cpu
machdep.cpu.core_count: 2
machdep.cpu.thread_count: 4
machdep.cpu.tsc_ccc.numerator: 242
machdep.cpu.tsc_ccc.denominator: 2
~~~

可以看到我的电脑线程数为4，所以我采用 `make -8`

编译成功后，使用 `emulator` 命令就可以运行编译出来的模拟器啦。

下面介绍几个常用到的命令：

如果make失败，或是想换一个BUILD类型，使用下面的命令：
~~~ C++
make clobber //清理out目录下的文件
~~~

**ncdu**，当电脑磁盘空间不够用时，用下面的命令，遍历大文件

~~~ C++
sudo ncdu //查看硬盘中的大文件
~~~

安装方式：

~~~ C++
brew install ncdu
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

![](http://blog.conorlee.top/blogimages/2017/aosp_make_single_module.png)

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

#### 使用 IntelliJ Debug Android 源码
参考这篇文章：[使用 IntelliJ Debug Android 源码](http://www.jianshu.com/p/7c2ab3d9498c)

文中，有一个地方跟我的电脑上不太一样：
#### 打开 monitor 选择 debug 进程
我电脑上的monitor不是用命令行打开，而是有monitor.app。
位置在：/Library/Android_sdk/tools/lib/monitor-x86_64/monitor.app
如下图：

![](http://blog.conorlee.top/blogimages/2017/aosp_monitor_position.png)

到这里Android源码部分的内容就整理完了，下篇文章利用Android源码分析下**Activity的启动流程**。

#### ccache造成硬盘空间不足
ccache 是一个是适用于 C 和 C++ 的编译器缓存，有助于提高编译速度。但是它本身是采用**牺牲空间换取编译速度**的方式。如果你设置了 ccache，而且经常使用 make clean，或者经常在不同的编译产品之间切换（比如choosecombo 或 lunch命令），会导致大量的硬盘存储空间被占用。

默认情况下，ccache 的缓存将存储在 ~/.ccache 下。如果你用的是MAC 电脑编译的 AOSP，可以看下这个目录有多大 :(

要想清理空间，直接删除 ~/.ccache 下的内容即可。

具体介绍参考google：[设置 ccache](https://source.android.com/source/initializing#setting-up-ccache)

### 错误收集

1.**Try increasing heap size with java option '-Xmx<size>'**

这个是说明需要增加Java heap size，具体参见[stackoverflow](https://stackoverflow.com/questions/35579646/android-source-code-compile-error-try-increasing-heap-size-with-java-option)

解决方法是：

~~~ C
export JACK_SERVER_VM_ARGUMENTS="-Dfile.encoding=UTF-8 -XX:+TieredCompilation -Xmx4g"
./prebuilts/sdk/tools/jack-admin kill-server
./prebuilts/sdk/tools/jack-admin start-server
~~~

2.**failed to build aosp. says subcommand failed**
MAC系统升级到**high sierra**，在用make指令编译源码的时候报错，没有更多提示，只有下面的日志：
~~~ C
...  system/tools/aidl/aidl_language_y.yy
... aidl_language_l.cpp
ninja: build stopped: subcommand failed.
15:57:14 ninja failed with: exit status 1 

#### failed to build some targets (05:20 (mm:ss)) ####
~~~
关键错误信息是`aidl_language_y.yy`和`aidl_language_l.cpp`

解决方案：


- Step1. Patch [bison fix](https://android-review.googlesource.com/c/platform/external/bison/+/517740) for High Sierra and build bison:
~~~ JavaScript
    1. cd /Volumes/AOSP/external/bison
    2. git cherry-pick c0c852bd6fe462b148475476d9124fd740eba160
    3. mm
~~~

- Step2. Replace prebuilt bison binary with patched binary
~~~ JavaScript
    1. cp /Volumes/AOSP/out/host/darwin-x86/bin/bison /Volumes/AOSP/prebuilts/misc/darwin-x86/bison/
~~~

- Step3. Build
~~~ JavaScript
    1. make  -j4
~~~

如果mm命令无法执行，需要回去执行以下命令:
~~~ JavaScript
source build/envsetup.sh
lunch   #之后回到external/bison目录
mm
~~~

参考自：http://www.jianshu.com/p/35f840dd7869 和https://groups.google.com/forum/#!topic/android-building/D1-c5lZ9Oco