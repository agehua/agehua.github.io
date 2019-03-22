---
layout: post
title:  使用JNI获取publickey实现
category: technology
tags:
  - JNI
  - publickey
keywords: JNI, publickey
banner: http://cdn.conorlee.top/Blossoming%20Almond%20Branch%20in%20a%20Glass%20with%20a%20Book.jpg
thumbnail: http://cdn.conorlee.top/Blossoming%20Almond%20Branch%20in%20a%20Glass%20with%20a%20Book.jpg
---


### 1.前言
之前写过一篇文件是关于[JNI学习和积累](http://agehua.github.io/2016/JNI-Learning)的文章。
这篇文章主要讲在使用JNI取得Publickey过程中遇到的问题和一些总结。

### 2.JNI获取Publickey实现
在上篇文章中，最终采用的加密方法来自[github项目](https://github.com/MasonLiuChn/AndroidUltimateEncrypt)。

但该项目中在4.0.4版本的手机上，取得publickey遇到兼容性问题。

<!--more-->

正常的Publickey字段样式是这样的：

~~~ Java
OpenSSLRSAPublicKey{modulus=a0d169cedabdaac3527c2099eeac4cbd74bb9b14c2571bcf6755f49e1d8c8439a37e009b0cb1b1ae9bf164dc976ddc4ee050621c746610d995185accbe8b3c09cc4f8c7afd990db47be814f7f45ec9c489be5b0933e89ff58070c29c98199331160bdb3a19e0687d36e850ee686c565737be4f61328264a58792e198d873b023ec11cb26a73305eea631ab18ec3ce746eb253e47c145503e933ee5da892326ecbb29b5a005aacef2d30d720611f7014aa3f2e40954b9e4deeaca1823dd2f7b2230670acabda70d2f14b1adc9480cf0a48d47866a1aa745ae97d0ab906cd76e5ab76916d03ef153b35edc2ae7284a613b3a3a312c73cf0b98e0ce9a8cf6a682fa,publicExponent=10001}
~~~

但是某些低版本手机上（我遇到的是三星GT-S7568 Android版本4.0.4）的Publickey字段样式是：

~~~ Java
RSA Public Key
      modulus: a0d169cedabdaac3527c2099eeaca0d169cedabdaac3527c2099eeac4cbd74bb9b14c2571bcf6755f49e1d8c8439a37e009b0cb1b1ae9bf164dc976ddc4ee050621c746610d995185accbe8b3c09cc4f8c7afd990db47be814f7f45ec9c489be5b0933e89ff58070c29c98199331160bdb3a19e0687d36e850ee686c565737be4f61328264a58792e198d873b023ec11cb6a73305eea631ab18ec3ce746eb23e47c145503e933ee5da892326ecbb29b5a005aacef2d30d720611f7014aa3f2e40954b9e4deeaca1823dd2f7b2230670acabda70d2f14b1adc9410cf0a48d47866a1aa745ae97d0ab906cd76e5ab76916d03ef153b35edc2ae7284a613b3a3a313c73cf0b98e0ce9a8cf6a682aff0b98e0ce9a8cf6a682fa
      public exponent: 10001
~~~
第一个字段modulus是一致的，第二个字段一个是“**publicExponent**”，另一个是“**public exponent**”。

所以我在代码中的处理是这样的

~~~ C++
jmethodID substring_mid = env->GetMethodID(string_cls,"substring", "(II)Ljava/lang/String;");
jmethodID indexOf_mid = env->GetMethodID(string_cls,"indexOf", "(Ljava/lang/String;)I");

jint bb = env->CallIntMethod(publicKey_str,indexOf_mid,param2);
jstring publicKey2_str =NULL;
if (bb ==-1){
	jstring param3 = env->NewStringUTF("public exponent");
	jint cc = env->CallIntMethod(publicKey_str,indexOf_mid,param3);
	__android_log_print(ANDROID_LOG_ERROR, TAG, "CC value is %d", cc);
	publicKey2_str= static_cast<jstring>(env->CallObjectMethod(publicKey_str,substring_mid,aa+8,cc-1));
}else {
	publicKey2_str = static_cast<jstring>(env->CallObjectMethod(publicKey_str,substring_mid,aa+8,bb-1));
}
~~~

#### 2.1 在JNI native代码中打印日志

上面提供的JNI方法中涉及到了JNI日志打印（“\__android_log_print”）

~~~ C++
__android_log_print(ANDROID_LOG_ERROR, TAG, "PublicKey value is %s",  jstringTostring(env,publicKey2_str));
~~~

- 该方法第一个参数以日志级别，有：ANDROID_LOG_INFO，ANDROID_LOG_DEBUG和ANDROID_LOG_ERROR
- 第二个参数设置过滤的标签，可以在LogCat视图中过滤。
- 后面参数是具体的日志内容

jstringTostring是将jstring转换成char*的方法：

~~~ C++
char* jstringTostring(JNIEnv* env, jstring jstr)
{
	char* rtn = NULL;
  jclass clsstring = env->FindClass("java/lang/String");
  jstring strencode = env->NewStringUTF("utf-8");
  jmethodID mid = env->GetMethodID(clsstring, "getBytes", "(Ljava/lang/String;)[B");
  jbyteArray barr= (jbyteArray)env->CallObjectMethod(jstr, mid, strencode);
  jsize alen = env->GetArrayLength(barr);
  jbyte* ba = env->GetByteArrayElements(barr, JNI_FALSE);
  if (alen > 0)
  {
  rtn = (char*)malloc(alen + 1);

  memcpy(rtn, ba, alen);
  rtn[alen] = 0;
  }
  env->ReleaseByteArrayElements(barr, ba, 0);
  return rtn;
}
~~~

jstring是Java提供的一个本地类型（Native Type），对应Java语言中的string类型

想了解JNI其他数据类型的，请看这里
[JNI学习积累之二 ---- 数据类型映射、域描述符说明](http://blog.csdn.net/qinjuning/article/details/7599796)

要使用JNI日志打印还需要：

- 1.导入.h文件及定义宏
在输出日志的.c文件中加入:

~~~ C++
#include <android/log.h>

#define LOG_TAG   "From JNI ===>>"
~~~

- 2.在Android.mk中引用日志库
加入下面这行代码:

~~~ C++
LOCAL_LDLIBS   := -lm -llog -ljnigraphics
~~~

具体学习可以移步这两篇文章：

[JNI的native代码中打印日志到eclipse的logcat中](http://www.cnblogs.com/liuling/p/2015-8-5-1.html)

[JNI中使用LOGCAT 进行Debug](http://blog.csdn.net/llwdslal/article/details/31733035)

#### 2.2 完整代码

下面是我修改后的完整代码

~~~ C++
jstring getPublicKey(JNIEnv* env, jobject thiz,jobject context) {
	jclass context_cls = env->GetObjectClass(context);

	jmethodID get_package_manager_mid = env->GetMethodID(context_cls,
			"getPackageManager", "()Landroid/content/pm/PackageManager;");

	jmethodID get_package_name_mid = env->GetMethodID(context_cls,
			"getPackageName", "()Ljava/lang/String;");
	env->DeleteLocalRef(context_cls);
	jobject pm_obj = env->CallObjectMethod(context, get_package_manager_mid);
	jclass pm_cls = env->FindClass("android/content/pm/PackageManager");

	jmethodID get_package_info_mid = env->GetMethodID(pm_cls, "getPackageInfo",
			"(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
	jstring package_name = reinterpret_cast<jstring>(env->CallObjectMethod(
			context, get_package_name_mid));
	jfieldID flag_fid = env->GetStaticFieldID(pm_cls, "GET_SIGNATURES", "I");
	jint flag = env->GetStaticIntField(pm_cls, flag_fid);
	env->DeleteLocalRef(pm_cls);
	jobject pi_obj = env->CallObjectMethod(pm_obj, get_package_info_mid,
			package_name, flag);
	env->DeleteLocalRef(package_name);

	jclass pi_cls = env->FindClass("android/content/pm/PackageInfo");
	jfieldID signatures_fid = env->GetFieldID(pi_cls, "signatures",
			"[Landroid/content/pm/Signature;");
	env->DeleteLocalRef(pi_cls);
	jobject sig_obj = env->GetObjectField(pi_obj, signatures_fid);
	env->DeleteLocalRef(pi_obj);

	jobjectArray sigs = reinterpret_cast<jobjectArray>(sig_obj);

	jclass signature_cls = env->FindClass("android/content/pm/Signature");
	jmethodID to_byte_array_mid = env->GetMethodID(signature_cls, "toByteArray",
			"()[B");

	jbyteArray sig_bytes = reinterpret_cast<jbyteArray>(env->CallObjectMethod(
			env->GetObjectArrayElement(sigs, 0), to_byte_array_mid));
	jclass certificate_factory_cls = env->FindClass(
			"java/security/cert/CertificateFactory");
	jmethodID get_certificate_instance_mid = env->GetStaticMethodID(
			certificate_factory_cls, "getInstance",
			"(Ljava/lang/String;)Ljava/security/cert/CertificateFactory;");

	jobject certificate_factory_obj = env->CallStaticObjectMethod(
			certificate_factory_cls, get_certificate_instance_mid,
			env->NewStringUTF("X509"));
	jmethodID generate_certificate_mid = env->GetMethodID(
			certificate_factory_cls, "generateCertificate",
			"(Ljava/io/InputStream;)Ljava/security/cert/Certificate;");
	env->DeleteLocalRef(certificate_factory_cls);

	jclass certificate_cls = env->FindClass("java/security/cert/Certificate");
	jclass byte_input_stream_cls = env->FindClass(
			"java/io/ByteArrayInputStream");
	jmethodID new_sig_bytes_is_mid = env->GetMethodID(byte_input_stream_cls,
			"<init>", "([B)V");
	jobject sig_bytes_is = env->NewObject(byte_input_stream_cls,
			new_sig_bytes_is_mid, sig_bytes);
	env->DeleteLocalRef(sig_bytes);
	env->DeleteLocalRef(byte_input_stream_cls);
	jobject cert = env->CallObjectMethod(certificate_factory_obj,
			generate_certificate_mid, sig_bytes_is);
	env->DeleteLocalRef(sig_bytes_is);
	env->DeleteLocalRef(certificate_factory_obj);
	jmethodID get_pubic_key_mid = env->GetMethodID(certificate_cls,
			"getPublicKey", "()Ljava/security/PublicKey;");
	env->DeleteLocalRef(certificate_cls);

	jobject publicKey  = env->CallObjectMethod(cert, get_pubic_key_mid);
	jclass publicKey_cls = env->GetObjectClass(publicKey);
	jmethodID toString_mid = env->GetMethodID(publicKey_cls,"toString", "()Ljava/lang/String;");
	jstring publicKey_str = static_cast<jstring>(env->CallObjectMethod(publicKey,toString_mid));
	env->DeleteLocalRef(cert);
	env->DeleteLocalRef(publicKey_cls);
	env->DeleteLocalRef(publicKey);


	jclass string_cls = env->GetObjectClass(publicKey_str);
	jmethodID indexOf_mid = env->GetMethodID(string_cls,"indexOf", "(Ljava/lang/String;)I");
	jstring param = env->NewStringUTF("modulus");
	jint aa = env->CallIntMethod(publicKey_str,indexOf_mid,param);
	jstring param2 = env->NewStringUTF("publicExponent");

	jmethodID substring_mid = env->GetMethodID(string_cls,"substring", "(II)Ljava/lang/String;");

	__android_log_print(ANDROID_LOG_ERROR, TAG, "PublicKey String is %s", jstringTostring(env,publicKey_str));

	jint bb = env->CallIntMethod(publicKey_str,indexOf_mid,param2);
	jstring publicKey2_str =NULL;
	if (bb ==-1){
		jstring param3 = env->NewStringUTF("public exponent");
		jint cc = env->CallIntMethod(publicKey_str,indexOf_mid,param3);
		__android_log_print(ANDROID_LOG_ERROR, TAG, "CC value is %d", cc);
		publicKey2_str= static_cast<jstring>(env->CallObjectMethod(publicKey_str,substring_mid,aa+8,cc-1));
	}else {
		publicKey2_str = static_cast<jstring>(env->CallObjectMethod(publicKey_str,substring_mid,aa+8,bb-1));
	}

	__android_log_print(ANDROID_LOG_ERROR, TAG, "PublicKey value is %s",  jstringTostring(env,publicKey2_str));
	return publicKey2_str;
}
~~~


### 3.附赠版本兼容的方法问题（随时更新）

低于Jellybean版本报如下错误：

    01-20 08:53:00.141: E/AndroidRuntime(24005): java.lang.NoSuchMethodError:
		android.view.ViewTreeObserver.removeOnGlobalLayoutListener

正确调用方式应该是：

~~~ Java
if (Build.VERSION.SDK_INT < Build.VERSION_CODES.JELLY_BEAN) {
		ViewTreeObserver.removeGlobalOnLayoutListener(this);
}else
		ViewTreeObserver.removeOnGlobalLayoutListener(this);
~~~

还有一个setBackground方法：

~~~ Java
if(Build.VERSION.SzDK_INT >= Build.VERSION_CODES.JELLY_BEAN) {
		View.setBackground(new ColorDrawable(Color.parseColor("#CCCCCC")));
} else {
		View.setBackgroundColor(Color.parseColor("#CCCCCC"));
}
~~~
