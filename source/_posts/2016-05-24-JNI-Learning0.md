---
layout: post
title: android JNI学习② JNI调用过程
category: accumulation
tags: Android JNI
keywords: jni, android源码
description: android Jni调用过程
banner: http://obxk8w81b.bkt.clouddn.com/Cottages%20Reminiscence%20of%20the%20North.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Cottages%20Reminiscence%20of%20the%20North.jpg
---


###  1.Android JNI调用过程

#### 1.1 由Android系统加载的JNI

Android系统在启动启动过程中，先**启动Kernel创建init进程**，紧接着由init进程fork第一个横穿Java和C/C++的进程，即Zygote进程。Zygote启动过程中会**AndroidRuntime.cpp中的startVm**创建虚拟机，VM创建完成后，紧接着调用startReg完成虚拟机中的JNI方法注册。

<!--more-->

在AndroidRuntime.cpp中：

~~~ C++
int AndroidRuntime::startReg(JNIEnv* env)
{
    //设置线程创建方法为javaCreateThreadEtc
    androidSetCreateThreadFunc((android_create_thread_fn) javaCreateThreadEtc);

    env->PushLocalFrame(200);
    //进程NI方法的注册
    if (register_jni_procs(gRegJNI, NELEM(gRegJNI), env) < 0) {
        env->PopLocalFrame(NULL);
        return -1;
    }
    env->PopLocalFrame(NULL);
    return 0;
}
~~~

**register_jni_procs(gRegJNI, NELEM(gRegJNI), env)**这行代码的作用就是就是循环调用gRegJNI数组成员所对应的方法。

~~~ C++
static int register_jni_procs(const RegJNIRec array[], size_t count, JNIEnv* env)
{
    for (size_t i = 0; i < count; i++) {
        if (array[i].mProc(env) < 0) {
            return -1;
        }
    }
    return 0;
}
~~~

   gRegJNI数组，有100多个成员变量，定义在AndroidRuntime.cpp：

~~~ C++
static const RegJNIRec gRegJNI[] = {
    REG_JNI(register_android_os_MessageQueue),
    REG_JNI(register_android_os_Binder),
    ...
};
~~~

   该数组的每个成员都代表一个类文件的jni映射，其中REG_JNI是一个宏定义，该宏的作用就是调用相应的方法。

比如MessageQueue和Binder方法都是Android系统启动时就已经注册，所以在AndroidRuntime.cpp中可以找到相应的native方法，见**AndroidRuntime.cpp的gRegJNI数组**。这些注册方法命令格式为：

~~~ Java
register_[包名]_[类名]
~~~

##### 示例一：以MessageQueue.java中的nativePollOnce方法为例，

~~~ Java
private native void nativePollOnce(long ptr, int timeoutMillis);
~~~
方法名：**android.os.MessageQueue.nativePollOnce()**，而相对应的native层方法名只是将点号替换为下划线，可得**android_os_MessageQueue_nativePollOnce()。**

- 前面说MessageQueue.java所定义的jni注册方法名应该是**register_android_os_MessageQueue**，的确存在于gRegJNI数组，说明这次JNI注册过程是有开机过程完成的。该方法在AndroidRuntime.cpp申明为extern方法：

~~~ C++
extern int register_android_os_MessageQueue(JNIEnv* env);
~~~

这些extern方法绝大多数位于/framework/base/core/jni/目录，大多数情况下native文件命名方式：

~~~ C++
[包名]_[类名].cpp
[包名]_[类名].h
~~~

> **Tips**： MessageQueue.java ==> android_os_MessageQueue.cpp

- 打开android_os_MessageQueue.cpp文件，搜索android_os_MessageQueue_nativePollOnce方法，这便找到了目标方法：

~~~ C++
static void android_os_MessageQueue_nativePollOnce(JNIEnv* env, jobject obj,
        jlong ptr, jint timeoutMillis) {
    NativeMessageQueue* nativeMessageQueue = reinterpret_cast<NativeMessageQueue*>(ptr);
    nativeMessageQueue->pollOnce(env, obj, timeoutMillis);
}
~~~

##### 示例二：对于native文件命名方式，有时并非[包名]\_[类名].cpp，比如Binder.java

Binder.java所对应的native文件：**android_util_Binder.cpp**

~~~ Java
public static final native int getCallingPid();
~~~

   根据示例一方式，找到getCallingPid ==> android_os_Binder_getCallingPid()，并且在AndroidRuntime.cpp中的gRegJNI数组中找到register_android_os_Binder。

按实例(一)方式则native文名应该为android_os_Binder.cpp，可是在/framework/base/core/jni/目录下**找不到该文件**，这是例外的情况。其实真正的文件名为**android_util_Binder.cpp**，这就是例外，这一点有些费劲，不明白为何google要如此打破规律的命名。

~~~ C++
static jint android_os_Binder_getCallingPid(JNIEnv* env, jobject clazz)
{
    return IPCThreadState::self()->getCallingPid();
}
~~~

有人可能好奇，既然如何遇到打破常规的文件命令，怎么办？这个并不难，首先，可以尝试在/framework/base/core/jni/中搜索，对于binder.java，可以直接搜索binder关键字，其他也类似。如果这里也找不到，可以通过grep全局搜索android_os_Binder_getCallingPid这个方法在哪个文件。

jni存在的常见目录：

- /framework/base/core/jni/
- /framework/base/services/core/jni/
- /framework/base/media/jni/


#### 1.2 加载自定义的JNI方法
前面两种都是在Android系统启动之初，便已经注册过JNI所对应的方法。 那么如果程序自己定义的jni方法，该如何查看jni方法所在位置呢？下面以MediaPlayer.java为例，其包名为android.media：

~~~ Java
public class MediaPlayer{
    static {
        System.loadLibrary("media_jni");
        native_init();
    }

    private static native final void native_init();
    ...
}
~~~

- 通过static静态代码块中System.loadLibrary方法来加载动态库，库名为media_jni, Android平台则会自动扩展成所对应的libmedia_jni.so库

- 接下来便要查看libmedia_jni.so库定义所在文件，一般都是通过Android.mk文件定义**LOCAL_MODULE:= libmedia_jni**，可以采用grep或者mgrep来搜索包含libmedia_jni字段的Android.mk所在路径。

搜索可知，libmedia_jni.so位于/frameworks/base/media/jni/Android.mk。用前面实例(一)中的知识来查看相应的文件和方法名分别为：

~~~ C++
android_media_MediaPlayer.cpp
android_media_MediaPlayer_native_init()
~~~

- 再然后，你会发现果然在该Android.mk所在目录/frameworks/base/media/jni/中找到android_media_MediaPlayer.cpp文件，并在文件中存在相应的方法：

~~~ C++
static void
android_media_MediaPlayer_native_init(JNIEnv *env)
{
    jclass clazz;
    clazz = env->FindClass("android/media/MediaPlayer");
    fields.context = env->GetFieldID(clazz, "mNativeContext", "J");
    ...
}
~~~

> **Tips**：MediaPlayer.java中的native_init方法所对应的native方法位于/frameworks/base/media/jni/目录下的android_media_MediaPlayer.cpp文件中的android_media_MediaPlayer_native_init方法。

**总结：System.loadLibrary()的作用就是调用相应库中的JNI_OnLoad()方法**。

#### 1.3 说说**JNI_OnLoad()**过程。

[-> android_media_MediaPlayer.cpp]

~~~ C++
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    if (register_android_media_MediaPlayer(env) < 0) {
        goto bail;
    }
    ...
}
~~~

详细说一下**register_android_media_MediaPlayer**

[-> android_media_MediaPlayer.cpp]

~~~ C++
static int register_android_media_MediaPlayer(JNIEnv *env)
{
    //【见3.4】
    return AndroidRuntime::registerNativeMethods(env,
                "android/media/MediaPlayer", gMethods, NELEM(gMethods));
}
~~~

虚拟机相关的变量中有两个非常重要的量JavaVM和JNIEnv:

- 1.JavaVM：是指进程虚拟机环境，每个进程有且只有一个JavaVM实例
- 2.JNIEnv：是指线程上下文环境，每个线程有且只有一个JNIEnv实例

其中gMethods，记录java层和C/C++层方法的一一映射关系。

~~~ C++
static JNINativeMethod gMethods[] = {
    {"prepare",      "()V",  (void *)android_media_MediaPlayer_prepare},
    {"_start",       "()V",  (void *)android_media_MediaPlayer_start},
    {"_stop",        "()V",  (void *)android_media_MediaPlayer_stop},
    {"seekTo",       "(I)V", (void *)android_media_MediaPlayer_seekTo},
    {"_release",     "()V",  (void *)android_media_MediaPlayer_release},
    {"native_init",  "()V",  (void *)android_media_MediaPlayer_native_init},
    ...
};
~~~

这里涉及到结构体**JNINativeMethod**，其定义在jni.h文件：

~~~ C++
typedef struct {
    const char* name;  //Java层native函数名
    const char* signature; //Java函数签名，记录参数类型和个数，以及返回值类型
    void*       fnPtr; //Native层对应的函数指针
} JNINativeMethod;
~~~

### 2.JNI资源

JNINativeMethod结构体中有一个字段为**signature(签名)**，再介绍signature格式之前需要掌握各种数据类型在Java层、Native层以及签名所采用的签名格式。

#### 2.1 数据类型

- **基本数据类型**

| *Signature格式* |	*Java* |	*Native* |
|:--------:|:-------:|:--------:|
|B | byte	|jbyte|
|C | char	| jchar|
|D	| double	| jdouble|
|F	| float | jfloat|
|I	| int |	jint|
|S	| short |	jshort|
|J	| long	| jlong|
|Z	| boolean	| jboolean|
|V	| void	| void|

- **数组数据类型**

数组简称则是在前面添加**[**：

| *Signature格式* |	*Java* |	*Native*|
|:--------:|:-------:|:--------:|
|[B |	byte[] |	jbyteArray|
|[C |	char[] |	jcharArray|
|[D |	double[] |	jdoubleArray|
|[F |	float[] |	jfloatArray|
|[I |	int[] |	jintArray|
|[S |	short[] |	jshortArray|
|[J |	long[] |	jlongArray|
|[Z |	boolean[] | jbooleanArray|

- **复杂数据类型**

对象类型简称：**L+classname +**;

| Signature格式 | Java | Native |
|:--------:|:-------:|:--------:|
| Ljava/lang/String; | String | jstring |
| L+classname +;	| 所有对象|	jobject|
|[L+classname +;	| Object[]	| jobjectArray|
| Ljava.lang.Class; |	Class	|jclass |
|Ljava.lang.Throwable;	| Throwable	| jthrowable|

- **Signature**

有了前面的铺垫，那么再来通过实例说说函数签名： (输入参数...)返回值参数，这里用到的便是前面介绍的Signature格式。

| Java函数 | 对应的签名|
|:--------:|:--------:|
|void foo()	| ()V|
|float foo(int i) |	(I)F|
|long foo(int[] i)	| ([I)J|
|double foo(Class c) |	(Ljava/lang/Class;)D|
|boolean foo(int[] i,String s) |	([ILjava/lang/String;)Z|
|String foo(int i) |	(I)Ljava/lang/String;|

#### 2.2 其他

- **(一)垃圾回收**
对于Java开发人员来说无需关系垃圾回收，完全由虚拟机GC来负责垃圾回收，而对于JNI开发人员，对于内存释放需要谨慎处理，需要的时候申请，使用完记得释放内容，以免发生内存泄露。在JNI提供了三种Reference类型，Local Reference(本地引用)， Global Reference（全局引用）， Weak Global Reference(全局弱引用)。其中Global Reference如果不主动释放，则一直不会释放；对于其他两个类型的引用都是释放的可能性，那是不是意味着不需要手动释放呢？答案是否定的，不管是这三种类型的那种引用，都尽可能在某个内存不再需要时，立即释放，这对系统更为安全可靠，以减少不可预知的性能与稳定性问题。

  另外，ART虚拟机在GC算法有所优化，为了减少内存碎片化问题，在GC之后有可能会移动对象内存的位置，对于Java层程序并没有影响，但是对于JNI程序可要小心了，对于通过指针来直接访问内存对象是，Dalvik能正确运行的程序，ART下未必能正常运行。

- **(二)异常处理**
Java层出现异常，虚拟机会直接抛出异常，这是需要try..catch或者继续往外throw。但是对于JNI出现异常时，即执行到JNIEnv中某个函数异常时，并不会立即抛出异常来中断程序的执行，还可以继续执行内存之类的清理工作，直到返回到Java层时才会抛出相应的异常。

  另外，Dalvik虚拟机有些情况下JNI函数出错可能返回NULL，但ART虚拟机在出错时更多的是抛出异常。这样导致的问题就可能是在Dalvik版本能正常运行的程序，在ART虚拟机上由于没有正确处理异常而崩溃。

### 3.JNI知识积累

JNI学习积累之一 ---- 常用函数大全
http://blog.csdn.net/qinjuning/article/details/7595104

JNI学习积累之二 ---- 数据类型映射、域描述符说明
http://blog.csdn.net/qinjuning/article/details/7599796

JNI学习积累之三 ---- 操作JNI函数以及复杂对象传递
http://blog.csdn.net/qinjuning/article/details/7607214

JNI 实战全面解析
http://blog.csdn.net/banketree/article/details/40535325
