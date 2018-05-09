---
layout: post
title: 代理(Proxy)在Anroid中的应用
category: english
tags:
    - Proxy
    - android
keywords: 投马, toastmaster
toc: true
---

一.

tb:电面一

- dp是什么，sp呢，有什么区别
即dip，全称device independent pixel。设备独立像素，是一种虚拟像素单位，用于以密度无关方式表示布局维度或位置，以确保在不同密度的屏幕上正常显示UI。在160dpi的设备上，1dp=1px。

sp 
缩放独立像素，全称scale independent pixel。类似于dp，一般用于设置字体大小，可以根据用户设置的字体大小偏好来缩放。

- 自定义View，ViewGroup注意那些回调？
public View(Context context, @Nullable AttributeSet attrs)，在attrs.xml中有声明一个属性

void onFinishInflate()；当系统解析XML中声明的View后回调此方法，调用顺序：内层View->外层View,如果是viewgroup,适合在这里获取子View。
`如果View没有在XML中声明而是直接在代码中构造的，则不会回调此方法。此时无法获取到View的宽高和位置`

void onAttachedToWindow()；
当view 被添加到window中回调，调用顺序：外层View->内层View。在XML中声明或在代码中构造，并调用addview（this view）方法都会回调该方法。
注意：
`此时View仅仅被添加到View，而没有开始绘制所以同样获取不到宽高和位置`

void onDetachedFromWindow()；
看名字就知道是与void onAttachedToWindow()；对应的方法，在VIew从Window中移除时回调，如执行removeView（）方法。
`如果一个View从window中被移除了，那么其内层View（如果有）也会被一起移除，都会回调该方法，且会先回调内层View的onDetachedFromWindow()方法。`

void onWindowFocusChanged(boolean hasWindowFocus)；
当View所在的Window获得或失去焦点时被回调此方法。除了常见的设置view的onGlobalLayoutListener，也可以通过这个方法取到VIew的宽高和位置；也适合在判断当失去焦点时停止一些工作，如图片轮播，动画执行等，当获取到焦点后继续执行。
hasWindowFocus：View所在Window是否获取到焦点,当该Window获得焦点时，hasWindowFocus等于true，否则等于false。

boolean onTouchEvent(MotionEvent event)；
当手指触摸View的时候回调该方法，前提是触摸事件没有被拦截或者被子View消费掉。该方法是事件分发流程中最后的消费者。
event：触摸事件对象，包含了该事件的所有信息，如触摸的类型（down、move、up），触摸位置等。
返回值:true：事件被消费了，false：没有被消费，事件传递到外层View，super方法：同false。

onMeasure( int ,  int ) 确定所有子元素的大小
onLayout( boolean ,  int ,  int ,  int ,  int ) 当View分配所有的子元素的大小和位置时触发
onSizeChanged( int ,  int ,  int ,  int ) 当view的大小发生变化时触发
onDraw(Canvas) view渲染内容的细节
onWindowVisibilityChanged(int) 当窗口中包含的可见的view发生变化时触发

- 界面卡顿的原因以及解决方法
UI线程有耗时操作
布局 Layout 过于复杂，无法在 16ms 内完成渲染
overdraw 导致像素在同一帧的时间内被绘制多次
自定义View没有优化，比如不能再onDraw方法里创建对象
同一时间View动画过多，导致CPU或GPU 负载过重
View 频繁的触发 measure、layout，导致 measure、layout 累计耗时过多和整个 View 频繁的重新渲染
Activity的onCreate和onResume回调尽量避免耗时操作
频繁的触发 GC 操作导致线程暂停，会使得安卓系统在 16ms 内无法完成绘制
冗余资源及逻辑等导致加载和执行缓慢
ANR

- android中的存储类型
sharedpreferences、文件存存储、数据库存储等、网络存储、ContentProvider存储，keystore也可以存储密钥

- service用过么，基本调用方法

startService：
onCreate()
onStartCommand(Intent intent, int flags, int startId)
onDestroy()

bindService：
onCreate()
onBind()
onUnbind()
onDestroy()

onStartCommand()是当一个service调用startService()之后由系统调用的，开发者不用直接调用这个方法，但是可以重写这个方法。
方法的返回值有四个int值，分别是：START_STICKY, START_NOT_STICKY, START_REDELIVER_INTENT, or START_STICKY_COMPATIBILITY.

- Handler机制
post(runnable)
sendMessage(message)
[Android进阶——Android消息机制之Looper、Handler、MessageQueue](http://blog.csdn.net/qq_30379689/article/details/53394061)


- LinearLayout、FrameLayout、RelativeLayout性能对比，为什么
相同层级下 LinearLayout = FrameLayout < RelativeLayout
越是复杂的层级，FrameLayout = LinearLayout>RelativeLayout。

RelativeLayout不如LinearLayout快的根本原因是RelativeLayout需要对其子View进行两次measure过程。而LinearLayout则只需一次measure过程，所以显然会快于RelativeLayout，但是如果LinearLayout中有weight属性，则也需要进行两次measure，但即便如此，应该仍然会比RelativeLayout的情况好一点。

用RelativeLayout时候尽量使用padding代替margin效率高。


- Activity的生命周期，finish调用后其他生命周期还会走么？
onCreate onStart onResume onPause onStop onDestory
onPostCreate、onPostResume
onPostCreate方法是指onCreate方法彻底执行完毕的回调，onPostResume类似

在onCreate中：onCreate->onDestroy
在onStart中：onCreate->onStart->onStop->onDestroy
在onResume中：onCreate->onStart->onResume->onPause->onStop->onDestroy


当Activity中有Fragment，然后在onCreate 方法中调用 finish：
未调用 finish 方法
Activity.onCreate -> Fragment.onAttach -> Fragment.onCreate -> Fragment.onCreateView -> Fragment.onViewCreated -> Fragment.onActivityCreated -> Fragment.onStart -> Activity.onStart -> Activity.onPostCreate -> Activity.onResume -> Fragment.onResume -> Activity.onPostResume；

息屏后又点亮： 
Fragment.onPause -> Activity.onPause -> Fragment.onStop -> Activity.onStop -> Activity.onRestart -> Fragment.onStart -> Activity.onStart -> Activity.onResume -> Fragment.onResume -> Activity.onPostResume；

关闭页面： 
Fragment.onPause -> Activity.onPause -> Fragment.onStop -> Activity.onStop -> Fragment.onDestroyView -> Fragment.onDestroy -> Fragment.onDetach -> Activity.onDestroy。

调用 Activity#finish() 方法
在 Activity#onCreate() 方法中调用 finish() 方法。 
Activity.onCreate -> Fragment.onAttach -> Fragment.onCreate -> Fragment.onDestroy -> Fragment.onDetach -> Activity.onDestroy。

- FW层熟悉么，源码看过么
看过AMS，简单了解Activity的启动流程和 Binder 机制

- GC回收机制熟悉么，分代算法知道么
  1.标记清除复制：用于青年代
  2.标记整理：用于老年代
  虚拟机栈分区：
         1.局部变量表
         2.操作数栈
         3.动态链接
         4.方法返回地址

- Java的类类加载原理
    JVM在加载类时默认采用的是双亲委派机制。首先判断该类型是否已经被加载，如果没有被加载，就委托给父类加载或者委派给启动类加载器加载  

内存泄漏如何排查，MAT分析方法以及原理，各种泄漏的原因是什么比如

- Handler为什么会泄漏
Handler已内部类的方式创建，会持有一个Activity引用，当Handler发送一个耗时消息或操作时，因为持有Activity的引用，导致当前Activity关闭时，无法及时释放内存。
内存泄漏与OOM（内存溢出）不同，内存溢出是指程序在申请内存时，没有足够的空间供其使用

- 为什么Dialog不能用Application的Context
如果使用Application的上下文来创建Dialog，在调用它的show方法时程序会报异常： android.view.WindowManager$BadTokenException: Unable to add window -- token null is not for an application

这里提到的Token主是指窗口令牌（Window Token），是一种特殊的Binder令牌，Wms用它唯一标识系统中的一个窗口。
跟Activity对应的窗口一样，Dialog有一个PhoneWindow的实例。Dialog 的类型是TYPE_APPLICATION，属于应用窗口类型。

Dialog最终也是通过系统的WindowManager把自己的Window添加到WMS上。在addView前，Dialog的token是null（上面提到过的w.setWindowManager第二参数为空）。
系统对TYPE_APPLICATION类型的窗口，要求必需是Activity的Token，不是的话系统会抛出BadTokenException异常。Dialog 是应用窗口类型，Token必须是Activity的Token。

- gradle熟悉么，自动打包知道么
使用productFlavors 分渠道打包

- 介绍下先的app架构和通信
app架构有 MVP等
Volley +okhttp的方式

- 自己负责过哪些模块，跟同事相比自己的优势是什么
GCM通知、一些业务流程、喜欢用英语检索

- 遇到过什么印象深刻的问题，怎么解决的
WebView内存泄漏问题，使用多进程

现场面试：三场
First：

最近都做了哪些工作？

遇到了什么印象深刻的问题。A:会顺着你介绍的项目问下具体实现。

推送消息有富文本么？

- 热修复了解么，用的什么？
代码修复主要有三个方案，分别是底层替换方案、类加载方案和Instant Run方案。

apk包大小有限制么？怎么减少包大小？

工作中有没有用过或者写过什么工具？脚本，插件等等

比如：多人协同开发可能对一些相同资源都各自放了一份，有没有方法自动检测这种重复之类的

写过native的底层代码么

view的绘制熟悉么，介绍下

gc相关的算法

anr是因为什么产生的，怎么排查

界面上的话，有什么优化措施么？比如列表展示之类的，平时遇到过内存问题吗，怎么优化的？

平时用过哪些设计模式？


Second：

介绍下最近一年主要做了什么工作

会对简历上突出的技能进行详情的询问：
比如：音频合成的具体步骤，以及遇到的一些问题和细节处理。
会根据面试发散一些问题，问到，seek方法播放到末尾后重新播放会有一些卡顿的不流畅问题，怎么避免，从交互设计或者技术角度。（个人表示没怎么关注这种）。

项目团队多少人，怎么分配工作

线程之间怎么通信的？

app的架构是怎么样的，并且为什么这样，有什么优缺点？

算法熟悉么？给了一个二叉排序树，出了一个给定节点找到它的下一个元素（指的是大小顺序的下一个）的算法题。

为什么找工作，自己的优势是什么


Third：
技术问题不再局限于简历，可能根据简历和回答情况渐进并扩散。

感觉各个技术面试官之前并没有沟通，可能会问到类似的问题


介绍下自己主要负责的工作

Activity的生命周期有哪些，知道onRestart么，介绍下

savedInstanceState知道么，干什么用的，什么时候有值，什么时候为空，平时是怎么用的

View绘制熟悉么，介绍下，能说下是实现原理么？

平时用过什么开发工具，分析工具？

ANR是怎么回事？怎么查？Service会引起ANR么？

Activity的启动模式有哪些？栈里是A-B-C，先想直接到A，BC都清理掉，有几种方法可以做到？这几种方法产生的结果是有几个A的实例？

有什么工具可以看到Activity栈信息么？多个栈话，有方法分别得到各个栈的Activity列表么

都熟悉哪些命令？知道怎么用命令启动一个Activity么?

SharedPrefrences的apply和commit有什么区别

java里带$的函数见过么，是什么意思

MD5是加密方法么，Base64呢

有博客和github，主要是写的什么？有哪些关注

android 8.0 有哪些新特性

差不多就这些吧。。最后每个面试官都会让你问他问题。



二.

glide缓存策略？同一个图片跟size有关么

android中的动画有哪些

View事件传递机制

界面卡顿怎么排查和优化？

Fragment的replace和end？？的区别？

MVP，MVVM，MVC解释和实践

项目之外的，对技术的见解，拓展知识


二面：

微信跳一跳外挂怎么实现，检测怎么做的？

一张纯色背景下怎么有效检测各个矩形？

对接的so算法了解么，有接触过相关的库么？

三个算法题选一个并写出测试用例：打印n-m之间所有的素数；计算n-m之间1出现的次数；指定数字序列的排序；

android api层的源码熟悉哪些？解释一下

ACTION_CANCEL什么时候触发，触摸button然后滑动到外部抬起会触发点击事件吗，在+ + 滑动回去抬起会么

怎么处理嵌套View的滑动冲突问题

热修复相关的原理，框架熟悉么

gradle打包流程熟悉么

任意提问环节：其实可以问之前面试中遇到的问题：比如，多模块开发的时候不同的负责人可能会引入重复资源，相同的字符串，相同的icon等但是文件名并不一样，怎样去重？


三.

NetBase：

Canvas的底层机制，绘制框架，硬件加速是什么原理，canvas lock的缓冲区是怎么回事

surfaceview， suface，surfacetexure等相关的，以及底层原理

android文件存储，各版本存储位置的权限控制的演进，外部存储，内部存储

上层业务activity和fragment的遇到什么坑？？页面展示上的一些坑和优化经验

网络请求的开源框架：OKHttp介绍，写过拦截器么



四.

Netbase：AI

数据层有统一的管理么，数据缓存是怎么做的，http请求等有提供统一管理么？

有用什么模式么，逻辑什么的都在Activity层？怎么分离的

如果用了一些解耦的策略，怎么管理生命周期的？

有什么提高编译速度的方法？

对应用里的线程有做统一管理么？

jni的算法提供都是主线程的？是不是想问服务类的啊

边沿检测用的啥？深度学习相关的有了解么？

上线后的app性能分析检测有做么



五.

yz:

进程间通信方式？Binder的构成有几部分？

HttpClient和HttpConnection的区别

View的事件传递机制

MVC，MVP，MVVM分别是什么？

Android中常用的设计模式，说三个比较高级的？

内存优化，OOM的原因和排查方法

想改变listview的高度，怎么做

Https是怎么回事？

除了日常开发，其他有做过什么工作？比如持续化集成，自动化测试等等



六.

DiDi:比较全面

ActivityA跳转ActivityB然后B按back返回A，各自的生命周期顺序，A与B均不透明。

Synchronize关键字后面跟类或者对象有什么不同。

单例的DCL方式下，那个单例的私有变量要不要加volatile关键字，这个关键字有什么用

JVM的引用树，什么变量能作为GCRoot？GC垃圾回收的几种方法

ThreadLocal是什么？Looper中的消息死循环为什么没有ANR？

Android中main方法入口在哪里

jdk1.5？SparseArray和ArrayMap各自的数据结构，前者的查找是怎么

实现的，与HashMap的区别

Runnable与Callable、Future、FutureTask的区别，AsyncTask用到哪个？AsyncTask是顺序执行么，for循环中执行200次new AsyncTask并execute，会有异常吗

IntentService生命周期是怎样的，使用场合等

RecyclerView和ListView有什么区别？局部刷新？前者使用时多重type场景下怎么避免滑动卡顿。懒加载怎么实现，怎么优化滑动体验。

SQLite的数据库升级用过么

开放问题：如果提高启动速度，设计一个延迟加载框架或者sdk的方法和注意的问题。

Scroller有什么方法，怎么使用的。

分享下项目中遇到的问题

webwiew了解？怎么实现和javascript的通信？相互双方的通信。@JavascriptInterface在？版本有bug，除了这个还有其他调用android方法的方案吗？

ReactiveNative了解多少

JNI和NDK熟悉么？Java和C方法之前的相互调用怎么做？