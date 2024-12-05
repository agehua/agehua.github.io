---
layout: post
title: Android App Bundle(AAB) 文件介绍
category: accumulation
tags:
    - Android APP Bundles
keywords: Android APP Bundles, AAB
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Evening%20The%20Watch%20after%20Millet.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Evening%20The%20Watch%20after%20Millet.jpg
toc: true
---

在2018年的Google I/O大会上，Google向 Android 引入了新 App 动态化框架（即Android App Bundle，缩写为AAB）
![Dynamic Delivery示意效图](/images/blogimages/2019/640.gif)
<!--more-->
### 关于Android App Bundles
Android App Bundle 是一种包含编译后代码和资源文件的新的上传格式（.aab），它推迟了APK的生成和签名，由google play来完成

Google Play推出新app交付模式，叫做动态交付 (Dynamic Delivery)，它根据每个用户的设备信息，使用开发者上传的app bundle来生成对应的apk文件
`使用aab文件会将apk的大小限制增加到500MB，这个限制不是指aab文件的大小，而是下载apk时的大小`
此外，也可以在app bundle中添加dynamic feature modules。这些模块可以包含新的功能和资源。开发者可以决定用户第一次安装时需不需要下载这些资源
`构建一个Android App Bundle，只需要几次点击，但是添加dynamic feature模块，可能需要重构整个项目`

#### Android App Bundle != APK
App Bundle 纯粹是为了上传设计的文件，用户无法直接安装和使用它。
它是一个 zip 文件，Google Play 从中生成优化的 APK 并将其提供给设备进行安装
从 APK 切换到App Bundle是一个无缝过程

#### Get started
构建 app bundles和支持动态交付，需要下面几步：
- 1.Android Studio 3.2 or higher
- 2.支持动态交付，需要包含一个base module，重新组织代码和资源，生成configuration APKs，此外可以添加 dynamic feature模块
- 3.生成 aab 文件（使用 AS 或 命令行工具）
- 4.使用 bundletool 来测试aab生成apk或发布到测试机上
- 5.使用 Google Play 应用签名
要使用推荐的应用发布格式 Android App Bundle，您需要先注册 Google Play 应用签名，然后才能在 [Play Console](https://support.google.com/googleplay/android-developer/answer/7384423)上传您的 app bundle 文件。
- 6.上传Play Console

### Split APKs
Split APKs是Android 5.0开始提供多apk构建机制，是 Dynamic Delivery 功能的最基本组件。

Split APKs将原来一个APK中多个模块共享同一份资源的模型，分离成多个APK使用各自的资源，并且可以继承Base APK中的资源，多个APK有相同的data，cache目录，多个dex文件，相同的进程，在Settings.apk中只显示一个APK，并且使用相同的包名。

Split APK可以将一个庞大的APK，按屏幕密度，ABI等形式拆分成多个独立的APK，在应用程序更新时，不必下载整个APK，只需单独下载某个模块即可安装更新。

- Base apks: 包含了一些基本功能，和其他split apks公用的资源和代码。Google play从你的项目的 app module来生成base apk。
当用户请求下载完整apk时，base apk是第一个被下载和安装的。 如果你想减少初始下载的大小，那就需要减少你的 app module 的代码
- Configuration apks: 包括不同的 native 代码，特定屏幕尺寸，CPU架构或者多语言。每当用户要下载这个应用的时候，只会下载真正对应其设备信息的apk。
配置apk不用单独的新建 module，google play会自动生成对应apk
- Dynamic feature apks: 每个apk包含一个特性，对应一个module。Play Core Library负责为用户按需安装这些特性。
这个 Dynamic feature 不是第一次安装需要的，Google play 从Dynamic feature module来生成动态特性apk文件

下图描述了为了提供一个完整app体验，Split Apks产生的依赖结构，并可能会在同一个设备上安装多个apk文件。
![](/images/blogimages/2019/apk_splits_tree-2x.png)

#### Devices running Android 4.4 (API level 19) and lower
Android 4.4 及以下设备并不支持下载和安装Split APKs。Google Play提供一个根据设备信息优化后的apk文件，叫做multi-APK
对于多语言，multi-APK会包含所有支持的语音，避免切换手机语言后还要下载对应语言的apk问题
`multi-APK不支持按需下载动态特性（dynamic features），所以在创建dynamic features模块的时候，需要考虑是否禁用按需下载或打开Fusing`

![Android Studio Fusing 选项](/images/blogimages/2019/app_bundle_fusing.png)

### The Android App Bundle format
Android App Bundle 是一种发布格式 —— 精确地说，是一个带有 .aab 扩展名的 zip 文件。它包含应用支持的所有设备的代码和资源，例如 DEX 文件、本地代码库、清单文件、各种资源文件等
> 注意：每个APP bundle对应一个独立的app或applicationID。因此，如果你使用了product flavors来创建不同版本的app，并且每个版本app都对应一个不同的applicationID，那你就需要为每个app构建一个app bundle

![](/images/blogimages/2019/aab_format-2x.png)
`蓝色方框区域就是configuration apks支持的配置项`

### Play Core Library

Play Core Library是AAB提供的核心库，用于下载、安装dynamic feature模块。
Android studio3.3 开始也支持为 Instant apps 按需下载模块

在Android Studio上选择 New -> Module -> 选择 Instant Dynamic Feature Module，会自动创建一个 Instant Module，对应的 AndroidManifest.xml 文件内容如下：
~~~ Java
<manifest xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:dist="http://schemas.android.com/apk/distribution"
    package="com.google.android.samples.instantdynamicfeatures">

    <!-- The dist:instant="true" makes this module instant enabled. Assuming the "app" module is
        also instant enabled, when this bundle is uploaded to the instant track on the
        Google Play Developer Console, this module will be included in the instant app. Any non
        instant enabled modules will be excluded -->
    <dist:module
        dist:instant="true"
        dist:onDemand="false"
        dist:title="@string/module_instant_feature_split_install">
        <dist:fusing dist:include="true" />
    </dist:module>

    <application>
        <activity android:name="com.google.android.samples.instantdynamicfeatures.SplitInstallInstantActivity"></activity>
    </application>

</manifest>
~~~

### Bundletool 介绍
[Bundletool](https://github.com/google/bundletool) 是一个用于处理 Android App Bundle 的命令行工具。使用 bundletool，您可以构建 AAB，提取连接的设备配置，生成 APK Set 文件 (.apks)，从 APK Set 文件中提取或安装 APK，以及验证App Bundle。由于 AAB 仅是一种发布格式，因此 bundletool 用于生成和测试 APK。

#### 用Bundletool工具测试AAB
- 1.安装bundletool：  `brew install bundletool`
- 2.切换到aab所在文件夹，生成 apks 文件
`bundletool build-apks --bunlde=<bundle_name>.aab --output=./<bundle_name>.apks`
- 3.如果要build签名的apks文件，则需要加上keystore设置
`bundletool build-apks --bundle=<bundle_name>.aab --output=<bundle_name>.apks --ks=<your_keystorename>.jks/.keystore --ks-pass=pass:<your_keystore_pass> --ks-key-alias=<your_keystore_alias> --key-pass=pass:<your_keystore_pass>`
- 4.将apks安装在设备上
`bundletool install-apks --apks=<bundle_name>.apks`

另外介绍两个有用的功能：
- 输出当前设备对应的设备信息
`bundletool get-device-spec --output=tcl.json  --adb=/Library/Android_sdk/platform-tools/adb`

- 生成当前设备对应的apks
`bundletool build-apks --connected-device --bundle=app.aab --output=./app1.apks --adb=/Library/Android_sdk/platform-tools/adb`

### Compressed download size restriction
- App首次下载安装，整个app（比如 base.apk+configuration.apk）大小不能超过 150MB
- 后续按需下载，每次也不能超过150MB
- Android App Bundles不支持apk扩展文件（*.obb）

> OBB（Opaque Binary Blob）文件格式，是安卓游戏通用数据包。在一些大型游戏上较为常见，同时还附以Data文件，亦或是md5.dat文件出现。通常在游戏开始前，程序会自动将obb解包/sdcard/Android/obb 目录下。但也有游戏不会解包，直接读取。

### Merits and demerits

#### Benifit of App Bundle
- Size更小（最高可以减少50%应用体积）
- 安装更快 （ base.apk + split apks）
- Android App Bundle 是单一工件，因此无需构建、签名或管理多个 APK
- 支持动态发布(为特定用户添加动态功能，而无需增加安装时的应用体积)
- 官方提供的系统级插件化方案

#### Limitations and costs
- 仅限于通过 Google Play 发布的应用，（Google进一步巩固自身生态）
- 需要加入到 Google 的 beta program
- 最低支持版本Android 5.0 (API level 21)
- 低于Android 5.0 (API level 21) 的版本GooglePlay会优化Size，但不支持动态交付。
- 需要升级到Android Studio 3.2以上，修改工程结构以便支持App Bundle格式
- 集成Play Core Library

#### Known issues
- 不支持动态修改resource tables
- 在dynamic feature 模块的清单文件里，不能引用不在base模块里的资源
- 在AS中，更改base模块的 build variant，并不会自动更改依赖base的其他模块，所以构建时可能会报错
- 不同模块 build configuration 不同，可能会有冲突，导致构建或运行时出错，`比如 buildTypes.release.debuggable = true`
- 由于AAB可能包含多个apk文件，所以在把apk发送到设备时可能会因找不到资源，导致运行时异常，`比如 adb刷机（ sideloading ）`
- 动态下载模块需要安装最新的Play Store app，所以有很小概率下载app时会回滚为下载一个multi-APK`（为 Android 4.4以下设备提供）`


### Ref
[BundleTool Github地址](https://github.com/google/bundletool)

[简书-Android动态化框架App Bundles](https://www.jianshu.com/p/57cccc680bb6)

[爱奇艺组件化探索之原理篇](https://zhuanlan.zhihu.com/p/34346219)

[【Android】函数插桩（Gradle + ASM）](https://www.jianshu.com/p/16ed4d233fd1)