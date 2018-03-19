
## Android B角色入门：

以现象分析本质

一个安卓App本质是一个压缩包，格式是.apk结尾。用压缩包方式打开可以看到里面主要有：
- 1.class文件，这里是.dex结尾的文件。谷歌为android设计了一个特殊的虚拟机，叫做dalvik。
- 2.assets目录，里面主要是静态html，字体文件(ttf)和图片
- 3.res目录，里面是开发时自己创建的各种资源，如图片（png）、音乐（MP3）
- 4.AndroidManifest.xml文件，这个是android工程的清单文件，四大组件的声明要写在这里

在Android studio工程中，看源码比上面稍微复杂一点.
Android studio的特点是，一个工程，多个模块。Eclipse是一个工作空间，多个工程，下面以我们Fidea的工程来简单介绍下：
- 1.一个主模块（module）是 “app”
- 2.一个目录gradle，里面有这个工程要使用的gradle的版本信息
- 3.一个build目录，这个工程的构建目录，里面是临时的生成的一些东西
- 4.还有两个重要的文件build.gradle，settings.gradle

Android studio使用Gradle构建工程。工程目录有build.gradle，settings.gradle两个gradle文件。
而且，每个module模块根目录也有一个build.gradle文件。

关于gradle构建的知识以后再说。
接下来看module ”app”：

- libs目录：里面是要依赖的库
- src目录：里面是代码和资源文件

打开src/mian/目录，里面有：
- assets目录，对应刚才apk压缩包里的assets目录。
- java目录，对应生成的class.dex
- res目录，对应apk里面的res目录

android 应用层开发主要使用java做为开发语言。
app界面使用xml格式进行布局。

一个Activity


Hi  各位好
以下是我总结的Fidea B角色应该知道的android入门清单，若有不足之处请指正。

一 语言：
1，Java

二 开发工具及环境
1，Android studio 2.X（https://developer.android.com/studio/index.html?hl=zh-cn#mac-bundle）。电脑上没有android sdk的话，初次打开Android studio会让你下载。
2，Fidea android工程是使用Android7.1.1 API25进行编译的，需要下载这个版本的sdk.
3，由于Android studio使用Gradle构建，Fidea项目使用的Gradle版本是3.3-all。在studio里面下载可能比较困难，可以在https://services.gradle.org/distributions/ 这里下载到本地，然后具体配置可以联系我 :)
4，没有安卓手机的话，模拟器推荐使用Genymotion，响应速度快，但同时需要下载VirtualBox(会占用一定的磁盘空间)。有安卓手机，需要打开手机的开发者模式，打开USB调试选项，然后用数据线连接电脑即可
5，CornerStone  SVN代码管理工具

三 框架
1，bcprov-jdk15on-155.jar  纯Java实现的加密算法包
2，google-zxing  二维码扫描框架
3，EventBus 发布者订阅者模式的消息传递框架
4，com.zhy:autolayout 布局约束框架
5，fastjson  处理Json和对象框架
6，materialloadingprogressbar  符合材料设计的下拉刷新和加载更多的框架
7，PhotoView  统一的手机相册风格框架
8，play-services-maps 使用谷歌地图服务
9，okhttp，okio 处理网络请求的框架
10，rxvolley，rxvolley-OKhttp 管理网络请求调度的框架
11，glide 图片加载框架
12，nineoldandroids  android开源动画库
13，multidex 多dex文件支持框架


备注说明： 还有一些谷歌提供的support view，和第三方封装的view这里没有列出来

四 推荐学习资料——建议有一点java基础
1，书籍：《第一行代码》作者郭霖，手把手教你入门，清晰易懂，
  Brian Hardy，Bill Phillips《 Android编程权威指南》京东地址：http://item.jd.com/11431307.html
2, 文档教程：Android官方培训课程中文版：http://hukai.me/android-training-course-in-chinese/index.html
  Android基础课堂 https://yq.aliyun.com/articles/60495
3，视频教程：慕课网——Android工程师  http://www.imooc.com/course/programdetail/pid/33
  极客学院——Android入门：http://www.jikexueyuan.com/path/android/
5，Android 开源App：Google sample：https://github.com/googlesamples?utf8=%E2%9C%93&query=android
  知乎问答：https://www.zhihu.com/question/26687642
6，Android studio教程：stormzhang：http://stormzhang.com/devtools/2014/11/25/android-studio-tutorial1/，
  Tikitoo：http://www.jianshu.com/p/36cfa1614d23


最后，安利一下我自己的博客: https://agehua.github.io/，欢迎大家浏览，哈哈！


## OCS分享会


## chatbot调研
