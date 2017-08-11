---
layout: post
title: android JNI学习① 基础知识
category: accumulation
tags: Android JNI
keywords: jni, aes 加密
description: android JNI学习，实现aes加密
banner: http://obxk8w81b.bkt.clouddn.com/Avenue%20of%20Plane%20Trees%20near%20Arles%20Station.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Avenue%20of%20Plane%20Trees%20near%20Arles%20Station.jpg
---



本文只是用来记录，写的不好还请见谅。

### 1.JNI介绍
JNI概念 : Java本地接口,Java Native Interface, 它是一个协议, 该协议用来沟通Java代码和外部的本地C/C++代码, 通过该协议 Java代码可以调用外部的本地代码, 外部的C/C++ 代码可以调用Java代码;

C和Java的侧重 :

- C语言 : C语言中最重要的是 函数 function;
- Java语言 : Java中最重要的是 JVM, class类, 以及class中的方法;

C与Java如何交流 :

- JNI规范 : C语言与Java语言交流需要一个适配器, 中间件, 即 JNI, JNI提供了一种规范;
- C语言中调用Java方法 : 可以让我们在C代码中找到Java代码class中的方法, 并且调用该方法;
- Java语言中调用C语言方法 : 同时也可以在Java代码中, 将一个C语言的方法映射到Java的某个方法上;
- JNI桥梁作用 : JNI提供了一个桥梁, 打通了C语言和Java语言之间的障碍;

<!--more-->

JNI中的一些概念 :

- native : Java语言中修饰本地方法的修饰符, 被该修饰符修饰的方法没有方法体;
- Native方法 : 在Java语言中被native关键字修饰的方法是Native方法;
- JNI层 : Java声明Native方法的部分;
- JNI函数 : JNIEnv提供的函数, 这些函数在jni.h中进行定义;
- JNI方法 : Native方法对应的JNI层实现的 C/C++方法, 即在jni目录中实现的那些C语言代码;

### 2.NDK简单介绍
C代码执行 : C代码被编译成库文件之后, 才能执行, 库文件分为动态库 和静态库 两种;

- 动态库 : unix环境下.so后缀的是动态库, windows环境下.dll 后缀的是动态库; 动态库可以依赖静态库加载一些可执行的C代码;
- 静态库 :.a后缀是静态库的扩展名;

库文件来源 : C代码 进行 编译 链接操作之后, 才会生成库文件, 不同类型的CPU 操作系统 生成的库文件是不一样;

- CPU分类 : arm结构, 嵌入式设备处理器; x86结构, pc服务器处理器; 不同的CPU指令集不同;
- 交叉编译 :windows x86编译出来的库文件可以在arm平台运行的代码;
- 交叉编译工具链 : Google提供的 NDK 就是交叉编译工具链, 可以在linux环境下编译出在arn平台下执行的二进制库文件;

NDK作用 : 是Google提供了交叉编译工具链, 能够在linux平台编译出在arm平台下执行的二进制库文件;

NDK版本介绍 : android-ndk-windows 是在windows系统中的cygwin使用的, android-ndk-linux 是在linux下使用的;

想深入了解NDK开发的同学，可以去安装一下cygwin，本文只涉及简单的C语言代码，不需要使用cygwin。

### 3.环境准备，使用android studio还是Eclipse
推荐使用Eclipse，这篇文章讲了使用Eclipse生成.h文件和生成so文件的配置过程。配置成功后可以远离命令行[Eclipse ADT插件生成.h/.so文件](http://blog.csdn.net/jspping/article/details/47780307)

#### 3.1 Java调用C流程

- a. 定义 Native 方法 : 比如在com.packagename.jni.JNITest.java 类中定义 Native 方法 public native int add(int x, int y);
- b. 生成方法签名 : 进入 AndroidProject/bin/classes 目录, 使用 javah com.packagename.jni.JNITest 命令, 便生成了头文件, 该头文件引用了 jni.h, 以及定义好了对应的 Native 方法, 生成 JNIEXPORT jint JNICALL Java_com_packagename_jni_JNITest_add (JNIEnv \*, jobject, jint, jint);  

Java中定义的方法 :

~~~ Java
//将Java中的两个int值 传给C语言, 进行相加后, 返回java语言 shuliang.han.ndkparameterpassing.DataProvider  
public native int add(int x, int y);  
~~~
对应C语言中定义的方法 :

~~~ C++
#include <jni.h>  

//方法签名, Java环境和调用native方法的类必不可少, 后面的参数就是native方法的参数  
jint Java_com_packagename_jni_JNITest_add(JNIEnv * env, jobject obj, jint x, jint y)  
{  
    return x + y;  
}  
~~~

#### 3.2 生成.so文件

Android.mk 文件：

~~~ C++
LOCAL_PATH := $(call my-dir)    

include $(CLEAR_VARS)    

LOCAL_MODULE    := hello-jni    
LOCAL_SRC_FILES := hello-jni.c    

include $(BUILD_SHARED_LIBRARY)  
~~~

- 文件内容解释：

  **获取当前文件内容** : $(call my-dir) 是编译器中的宏方法, 调用该宏方法, 就会返回前的目录路径;

  **赋值符号** : " := " 是赋值符号, 第一句话 是 返回当前文件所在的当前目录, 并将这个目录路径赋值给 LOCAL_PATH;

  **初始化编译模块参数** : $(CLEAR_VARS) 作用是将编译模块的参数初始化, LOCAL_MODULE LOCAL_SRC_FILES 也是这样的参数;

  **指定编译模块** : LOCAL_MODULE    := hello-jni , 指定编译后的 so 文件名称, 编译好之后系统会在该名称前面加上 "lib", 后缀加上 ".so";

  **指定编译源文件** : LOCAL_SRC_FILES := hello-jni.c 告诉编译系统源文件, 如果有多个文件那么就依次写在后面即可;

  **编译成静态库** : include $(BUILD_SHARED_LIBRARY), 作用是告诉系统, 将编译的结果编译成.so后缀的静态库;

  **静态库引入** : NDK的platform中有很多 ".a" 结尾的动态库, 我们编译动态库的时候, 可以将一些静态库引入进来;

- 生成 动态库 so 文件 : 进入 Android.mk 所在目录, 在该目录执行ndk下的ndk-build命令;
- Java代码加载动态库 : 在 Java 代码中调用该类的类前面, 在类的一开始, 不在方法中, 加入

~~~ C++
static{ System.loadLibrary("hello"); } ;
~~~

- Application.mk 文件内容为（不写这个文件也可以）:

~~~ Java
APP_STL := stlport_static
APP_ABI := all
~~~

<!-- ![]({{ site.url }}/assets/img/jni_structure.png  =338x461) -->
<!-- 指定图片大小有问题 -->

<!-- [![Pure CSS Happy Hacking Keyboard](/assets/img/jni_structure.png)](http://codepen.io/P233/pen/qEagi) -->
![Eclipse JNI目录结构](http://oui2w5whj.bkt.clouddn.com/blogimages/2016/jni_structure.png)

[这篇文章](http://blog.csdn.net/hejinjing_tom_com/article/details/8125648)是使用javah导出头文件过程中，常见错误和解决办法，这里做一个记录。


### 4.字符串的处理

- Java中的String转为C语言中的char字符串
下面的工具方法可以在C程序中解决这个问题：

~~~ Javascript
// java中的jstring, 转化为c的一个字符数组  
char* Jstring2CStr(JNIEnv* env, jstring jstr) {  
//声明了一个字符串变量 rtn  
char* rtn = NULL;  
//找到Java中的String的Class对象  
jclass clsstring = (*env)->FindClass(env, "java/lang/String");  
//创建一个Java中的字符串 "GB2312"  
jstring strencode = (*env)->NewStringUTF(env, "GB2312");  
/*
 * 获取String中定义的方法 getBytes(), 该方法的参数是 String类型的, 返回值是 byte[]数组
 * "(Ljava/lang/String;)[B" 方法前面解析 :
 * -- Ljava/lang/String; 表示参数是String字符串
 * -- [B : 中括号表示这是一个数组, B代表byte类型, 返回值是一个byte数组
 */  
jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes",  
		"(Ljava/lang/String;)[B");  
//调用Java中的getBytes方法, 传入参数介绍 参数②表示调用该方法的对象, 参数③表示方法id , 参数④表示方法参数  
jbyteArray barr = (jbyteArray)(*env)->CallObjectMethod(env, jstr, mid,  
		strencode); // String .getByte("GB2312");  
//获取数组的长度  
jsize alen = (*env)->GetArrayLength(env, barr);  
//获取数组中的所有的元素 , 存放在 jbyte*数组中  
jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);  
//将Java数组中所有元素拷贝到C的char*数组中, 注意C语言数组结尾要加一个 '\0'  
if (alen > 0) {  
	rtn = (char*) malloc(alen + 1); //new   char[alen+1]; "\0"  
	memcpy(rtn, ba, alen);  
	rtn[alen] = 0;  
}  
(*env)->ReleaseByteArrayElements(env, barr, ba, 0); //释放内存  
	return rtn;  
}  
~~~

- Jstring2CStr方法讲解 :
   - a. 获取Java中String类型的class对象 : 参数 : 上下文环境 env, String类完整路径 ;
~~~ Javascript
jclass clsstring = (*env)->FindClass(env, "java/lang/String");  
~~~
   - b.创建Java字符串 : 使用 NewStringUTF 方法;
~~~ Javascript
jstring strencode = (*env)->NewStringUTF(env, "GB2312");  
~~~   
   - c.获取String中的getBytes()方法 : 参数介绍 ① env 上下文环境 ② 完整的类路径 ③ 方法名 ④ 方法签名, 方法签名 Ljava/lang/String; 代表参数是String字符串, [B  中括号表示这是一个数组, B代表byte类型, 返回值是一个byte数组;
~~~ Javascript
jmethodID mid = (*env)->GetMethodID(env, clsstring, "getBytes",  
    "(Ljava/lang/String;)[B");  
~~~       
   - d. 获取数组的长度 :
~~~ Javascript
jsize alen = (*env)->GetArrayLength(env, barr);  
~~~
   - e. 获取数组元素 : 获取数组中的所有的元素 , 存放在 jbyte*数组中;
~~~ Javascript
jbyte* ba = (*env)->GetByteArrayElements(env, barr, JNI_FALSE);  
~~~
   - f.数组拷贝: 将Java数组中所有元素拷贝到C的char*数组中, 注意C语言数组结尾要加一个 '\0';
~~~ Javascript
if (alen > 0) {  
    rtn = (char*) malloc(alen + 1); //new   char[alen+1]; "\0"  
    memcpy(rtn, ba, alen);  
    rtn[alen] = 0;  
}  
~~~
   - g.释放内存 :
~~~ Javascript
(*env)->ReleaseByteArrayElements(env, barr, ba, 0); //释放内存
~~~

### 5.JNI方法命名规则(标准JNI规范)

- **JNI实现的方法与Java中Native方法的映射关系 :**

  使用方法名进行映射, 可以使用javah工具进入bin/classes目录下执行命令, 即可生成头文件;

- **JNI方法参数介绍:**

  参数① : 第一个参数是JNI接口指针JNIEnv;

  参数② : 如果Native方法是非静态的, 那么第二个参数就是对Java对象的引用, 如果Native方法是静态的, 那么第二个参数就是对Java类的Class对象的引用;

- **JNI方法名规范:**  

  返回值+Java前缀+全路径类名+方法名+参数① JNIEnv+参数② jobject+其它参数;

  注意分隔符 : Java前缀 与 类名 以及类名之间的包名 和 方法名之间 使用 "\_" 进行分割;

- **声明 非静态 方法:**

  Native方法 : public int hello (String str, int i);

  JNI方法: jint Java_shuliang_han_Hello_hello(JNIEnv * env, jobject obj, jstring str, jint i);

- **声明 静态 方法 :**

  Native方法 : public static int hello (String str, int i);

  JNI方法 : jint Java_shuliang_han_Hello_hello(JNIEnv * env, jobject clazz, jstring str, jint i);

- **两种规范 :**

  以上是Java的标准JNI规范, 在Android中还有一套自定义的规范, 该规范是Android应用框架层 和 框架层交互使用的JNI规范, 依靠方法注册 映射 Native方法 和 JNI方法;

- **JNIEnv作用 :**

  JNIEnv 是一个指针,指向了一组JNI函数, 这些函数可以在jni.h中查询到,通过这些函数可以实现 Java层 与 JNI层的交互 , 通过JNIEnv 调用JNI函数 可以访问java虚拟机, 操作java对象;

- **JNI线程相关性 :**

  JNIEnv只在当前的线程有效,JNIEnv不能跨线程传递, 相同的Java线程调用本地方法, 所使用的JNIEnv是相同的, 一个Native方法不能被不同的Java线程调用;

- **JNIEnv结构体系 :**

  JNIEnv指针指向一个线程相关的结构,线程相关结构指向一个指针数组,指针数组中的每个元素最终指向一个JNI函数.

### 6.AES加密实现
网上有几种AES实现的方式：

- 1.这个是我现在项目中使用的方法，在Github上有这个工程，这种方式是使用JNI生成一个与设备相关的密码，可以将该密码作为AES的密钥。[链接地址](https://github.com/MasonLiuChn/AndroidUltimateEncrypt)

- 2.网上还有一种方式是由JNI生成keyValue和iv，Java层使用：[链接地址](http://blog.csdn.net/why_2012_gogo/article/details/40055245)

主要代码：

~~~ Java
static {
	System.loadLibrary("cwtlib");
	keyValue = getKeyValue();
	iv = getIv();

	if(null != keyValue &&
		null != iv) {
		KeyGenerator kgen;  
        try {  
            kgen = KeyGenerator.getInstance("AES");  
            kgen.init(128, new SecureRandom(keyValue));  
            key = kgen.generateKey();  
            paramSpec = new IvParameterSpec(iv);  
            ecipher = Cipher.getInstance("AES/CBC/PKCS5Padding");         
        } catch (NoSuchAlgorithmException e) {  
        } catch (NoSuchPaddingException e) {  
        }  
	}
}   

public static native byte[] getKeyValue();
public static native byte[] getIv();
~~~

  这种方式，在android app程序完全退出后，再进入该app时，之前加密好的字符串**无法解密。**

- 3.还有一种是直接由C或C++实现AES整个算法，直接使用网上代码并不知道靠不靠谱

  所以，我们项目最终使用了第一种方法

### 7.JNI混淆问题

  检查下 C/C++代码中没有直接访问Java代码的类或者类的成员变量、类的成员函数。

  如果有的话，这些就不能混淆

~~~ Java
//保留jni的回调类
-keep class com.your.jnicallback.class { *; }
//这个不用更改，直接复制就可以
-keepclasseswithmembernames class * {
    native <methods>;
}
~~~

### 8.总结
android 实现JNI入门并不难，笔者也刚刚入门，但要深入了解还是需要很长的路要走。

#### 8.1 更新内容，JNI获取publickey实现
在本文中最终使用第6点中的第一种方式，但原方法在4.0.4手机上遇到兼容性问题，详情请看我的这篇博客[使用JNI获取publickey实现](https://agehua.github.io/2016/05/24/JNI-Learning0/)
