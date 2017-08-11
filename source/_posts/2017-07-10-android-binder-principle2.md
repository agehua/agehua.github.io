---
layout: post
title:  Android Binder机制分析（二）
category: accumulation
tags: AIDL
keywords: AIDL, Binder
banner: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Branches.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Branches.jpg
toc: true
---

### 背景
[上篇文章](http://agehua.github.io/2017/07/08/android-binder-principle/
)分析到了Binder机制，分别介绍了自定义AIDL服务和调用系统的远程服务和他们之间的区别。
本文承接上篇文章，继续介绍ServiceManager和系统服务的注册流程，最后对Binder机制进行分析。


> 关于系统服务的注册流程，大都转载自[这篇文章](http://www.wjdiankong.cn/android%E7%B3%BB%E7%BB%9F%E7%AF%87%E4%B9%8B-binder%E6%9C%BA%E5%88%B6%E5%92%8C%E8%BF%9C%E7%A8%8B%E6%9C%8D%E5%8A%A1%E8%B0%83%E7%94%A8%E6%9C%BA%E5%88%B6%E5%88%86%E6%9E%90/)

### 服务大管家ServiceManager
> ServiceManager.java的源码可以在谷歌源码中看到，[点击这里](https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/os/ServiceManager.java)

[上篇文章](http://agehua.github.io/2017/07/08/android-binder-principle/
)末尾提到，系统服务的IBinder对象都是由ServiceManager统一管理的。为什么这么说呢，先看下ServiceManager.getService方法:
<!--more-->
~~~ Java
/**
 * Returns a reference to a service with the given name.
 *
 * @param name the name of the service to get
 * @return a reference to the service, or <code>null</code> if the service doesn't exist
 */
public static IBinder getService(String name) {
   try {
       IBinder service = sCache.get(name);
       if (service != null) {
           return service;
       } else {
           return getIServiceManager().getService(name);
       }
   } catch (RemoteException e) {
       Log.e(TAG, "error in getService", e);
   }
   return null;
}
~~~

sCache是一个本地静态Map对象，作为缓存池：
~~~ Java
private static HashMap<String, IBinder> sCache = new HashMap<String, IBinder>();
~~~
> ServiceManager本身会维护一个IBinder缓存池，也是为了效率高考虑，对于一个应用频繁的使用一些服务的话效率就会高很多。

然后最核心的获取服务的方法是getIServiceManager方法：
~~~ Java
private static IServiceManager getIServiceManager() {
    if (sServiceManager != null) {
        return sServiceManager;
    }
    // Find the service manager
    sServiceManager = ServiceManagerNative.asInterface(BinderInternal.getContextObject());
    return sServiceManager;
}
~~~
> 在上面的代码中，调用了ServiceManagerNative.asInterface()方法，是不是说明ServiceManager也通过远端服务来取得对应的服务呢？

具体在看一下ServiceManagerNative.java方法：
~~~ Java
public abstract class ServiceManagerNative extends Binder implements IServiceManager
{
    /**
     * Cast a Binder object into a service manager interface, generating
     * a proxy if needed.
     */
    static public IServiceManager asInterface(IBinder obj)
    {
        if (obj == null) {
            return null;
        }
        IServiceManager in =
            (IServiceManager)obj.queryLocalInterface(descriptor);
        if (in != null) {
            return in;
        }

        return new ServiceManagerProxy(obj);
    }

    //....
}    
~~~

> 看到上面的代码，基本可以确认，在获取ServiceManager对象也是通过了远程调用。只是名字改成了ServiceManagerNative，本应该叫ServiceManagerService的。

看到这里的ServiceManager也是通过远端服务获取到他的IBinder对象，然后在转化成本地对象进行使用的。那么刚刚看到系统的服务都是通过ServiceManager管理获取的，而现在ServiceManager本身是怎么获取到的IBinder对象的呢？这个就要从系统启动的时机看了，众所周知系统启动的时候是根据init.rc文件进行操作的：

~~~ C++
service servicemanager /system/bin/servicemanager
    class core
    user system
    group system
    critical
    onrestart restart healthd
    onrestart restart zygote
    onrestart restart media
    onrestart restart surfaceflinger
    onrestart restart drm
~~~

这里会启动一个servicemanager服务，那么就要去**service_manager.c**程序中的入口程序看了：

![service_manager服务启动流程](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/servicemanager-init.png)

这个入口其实包含了Binder机制的重要信息，而主要就是三件事：

- 1、打开底层的Binder驱动程序，这个后面介绍Binder机制在介绍
- 2、通过向binder程序发送命令：**BINDER_SET_CONTEXT_MGR**，告诉binder程序，我要成为大管家
- 3、进入循环监听上层应用的服务请求处理，所以这里可以看到其实ServiceManager是一个守护进程在后台默默监听

在第二步中成为大管家的代码深入看一看：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/binder-service-manager.png)

其实这里的逻辑也是比较简单的，首先创建一个属于servicemanager的binder节点，然后在创建一个binder链表，而这个链表的作用就是存放上层中需要系统服务的所有binder对象的节点，这样ServiceManager就可以实现了服务的增加和查询操作了。

再来看看ServiceManager的添加服务操作：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/servicemanager-add-service.png)

添加服务比较复杂，首先查看这个服务有没有注册权限限制，不是所有的服务都能注册的，然后在查看这个服务是不是已经被注册过了，最后在通知binder驱动程序注册一个服务即可。

然后在来看看ServiceManager的查找服务功能：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/servicemanager-find-service.png)

查找服务就比较简单了，直接通过服务的描述符名称遍历binder链表节点即可。

- 1、Service Manager能集中管理系统内的所有服务，它能被施加权限控制，并不是任何进程都能注册服务的。
- 2、Service Manager支持通过字符串名称来查找对应的Service。
- 3、由于各种原因的影响，Server进程可能生死无常。如果有了Service Manager做统一的管理，那么Client只要向Service Manager做查询，就能得到Server的最新信息。

### 系统服务注册流程分析
这部分内容也可以参考博客：[Android深入浅出之Binder机制](http://www.cnblogs.com/innost/archive/2011/01/09/1931456.html)

下面来看一下一些系统服务是如何进程注册的，这里用MediaService来进行查看吧。

系统中的MediaService服务的启动也是在init.rc中的
~~~ C++
service media /system/bin/mediaserver
  class main
  user media
  group audio camera inet net_bt net_bt_admin net_bw_acct drmrpc mediadrm
  ioprio rt 4
~~~
查看Main_mediaserver.cpp源码的main函数：
~~~ C++
int main(int argc, char** argv)
{
  //获得一个ProcessState实例
  sp<ProcessState> proc(ProcessState::self());

  //得到一个ServiceManager对象
  sp<IServiceManager> sm = defaultServiceManager();
  MediaPlayerService::instantiate();//初始化MediaPlayerService服务
  ProcessState::self()->startThreadPool();
  IPCThreadState::self()->joinThreadPool();
}
~~~
> sp是google搞出来的为了方便C/C++程序员管理指针的分配和释放的一套方法，就把它当做一个普通的指针看待，sp&lt;XXX&gt;就看成是XXX*就可以了

这里用MediaPlayerService来看看注册操作：
~~~ C++
void MediaPlayerService::instantiate() {
  defaultServiceManager()->addService(
    String16("media.player"), new MediaPlayerService());
  )
}
~~~

看到熟悉的代码了把，这里通过ServiceManager来进行服务注册了，那么这里是如何获取到ServiceManager的？
~~~ C++
sp<IServiceManager> defaultServiceManager() {
  if (gDefaultServiceManager != NULL) return gDefaultServiceManager;

  {
    AutoMutex _l(gDefaultServiceManagerLock); //--->锁保护
    while (gDefaultServiceManager == NULL) {
      gDefaultServiceManager = interface_cast<IServiceManager> (
        ProcessState::self()->getContextObject(NULL));
      if (gDefaultServiceManager == NULL)
        sleep(1);
    }
  }
  return gDefaultServiceManager;
}
~~~
看看ProcessState.cpp的源码：
~~~ C++
sp<IBinder> ProcessState::getContextObject(const sp<IBinder>& caller) {
  return getStrongProxyForHandle(0);
}
~~~
看看getStrongProxyForHandle方法实现：

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/getStrongProxyForHandle.png)

这里看到了，会使用IPCThreadState的transact方法和底层的Binder进行通信的，然后使用一个句柄handle构造一个BpBinder对象，而BpBinder对象其实就是native层实现的Binder对象，以后只要看到Bp开头的就是代理对象对应Java层的Proxy对象，Bn开头的就是native对象对应Java层的Stub对象。

在上面分析servicemanager的时候知道会维护一个binder节点链表，那里其实就有一个每个binder对应句柄handle，而后续进行通信的话都是通过这个句柄来标识是哪个服务的binder对象了，这样也就在通信的时候不会发生紊乱了，而servicemanager的句柄handle就是0。还有一个知识点就是可以看到IPC通信的时候传输数据使用的就是Parcel类，这个类就是为了跨进程通信产生的，他有一个方法readStrongBinder，就是可以从Parcel的数据中获取到Binder对象，这个也是在跨进程中传递Binder对象的核心地方。
~~~ Java
public final IBinder readStrongBinder() {
  return nativeReadStronBinder(mNativePtr);
}
~~~

好了，上面就通过系统的mediaserver服务来讲解了系统服务的注册流程：

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/media-server-process.png)

到这里就分析完了Android中的远程服务调用机制逻辑以及ServiceManager这个服务大管家的作用:

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/service-manager-activity.png)

- 1、首先跨进程通信的话，肯定会有两个对象：一个是本地端的中间者Proxy对象，一个是远程端的中间者Stub对象
- 2、Proxy对象通过静态代理模式维持一个远端传递过来的Binder对象，而Stub对象可以把远端传递过来的Binder对象转化成一个实际服务对象给应用使用
- 3、Android中在使用系统服务的时候通过getSystemService方法获取到的其实都是Stub把远端的Binder转化的对象，因为系统服务都是在system_server进程中，所以肯定是跨进程获取对象的，那么这个Binder对象其实就是上面的Proxy对象
- 4、系统的服务都是在一个指定的系统进程中system_server
- 5、服务大管家ServiceManager在系统启动的时候也是先获取自生的Binder对象，然后转化成实际操作对象，然后才可以操作系统服务的注册和查询功能

下面是系统一些服务的注册流程：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/system-service-register-process.png)

上面已经介绍了远程服务调用机制以及ServiceManager的实现原理，下面就要看看另外一个重点，也是上面提到的一个重要对象Binder，准确来说这个是Binder机制，在Android中Binder机制最复杂的一个架构系统了，它的设计很复杂，所以有很多同学在了解Binder机制的时候，总是看着看着就晕了，今天我们就直说重点，而且说得要相对明了简单。

### Binder机制解析
#### 第一、Android中的IPC为何要采用Binder机制

Binder是Android系统进程间通信(IPC)方式之一。Linux已经拥有的进程间通信IPC手段包括(Internet Process Connection)：管道(Pipe)、信号(Signal)和跟踪(Trace)、插口(Socket)、报文队列(Message)、共享内存(Share Memory)和信号量(Semaphore)。

Binder基于Client-Server通信模式，传输过程只需一次拷贝，为发送发添加UID,PID身份，既支持实名Binder也支持匿名Binder，安全性高。对Binder而言，Binder可以看成Server提供的实现某个特定服务的访问接入点， Client通过这个‘地址’向Server发送请求来使用该服务；对Client而言，Binder可以看成是通向Server的管道入口，要想和某个Server通信首先必须建立这个管道并获得管道入口。

#### 第二、Android中的Binder实现原理

其实Android中的Binder通信都是通过虚拟驱动设备程序/dev/binder来实现的，我们知道一些硬件都会对应一个驱动程序，而binder驱动程序没有对应的硬件，所以叫做**虚拟驱动设备程序**，其实他就是一个字符驱动设备，或者叫做miscdevice混杂设备驱动。

其实混杂驱动设备是字符设备的一种，它们共享一个主设备号(10)，但次设备号不同，所有的混杂设备形成一个链表，对设备访问时内核根据次设备号查找到相应的miscdevice设备。例如:触摸屏，LED，按键，串口。即：为了节约主设备号，将某些设备用链表的形式连接在一起，最后通过查找次设备区分。这里用主设备无法匹配出设备驱动，只能找到链表，再通过次设备号，才能找到设备驱动。而之前所学的，一般字符设备，通过主设备号，就能找到设备驱动了。我们可以通过命令查看/dev/binder驱动的主设备号：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/device-number.png)

#### 第三、Android中Binder通信机制

先来看一张图，我们可以大体的了解到了客户端和服务端通过Binder驱动进行通信

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/binder-drive-principle.png)

首先不管是客户端进程还是服务端进程都是在用户空间的，而binder驱动是在内核空间的，通信的数据是有规定格式也叫作IPC数据，既然是一种通信机制，肯定是需要协议，数据格式等基础结构信息的：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/IPC-data-format.png)

上面在分析了ServiceManager的启动的时候说到了，第一步是打开驱动程序，具体打开函数在binder.c中：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/binder-open-drive.png)

在使用一个驱动之前，肯定要先打开驱动，然后把驱动程序映射到内存中，接着借助IPCTreadState.cpp和binder驱动进行通信了：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/transact-binder.png)

所以看到这里IPCThreadState也是需要进入后台进行监听的，处理来自客户端和服务端的数据传输消息

最后再来看一下通信时序图。
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/communication-timing.png)

到这里我们就介绍完了Binder机制了，关于Binder机制最好不要看太深，因为越深你觉得越复杂越难理解，其实你只要了解到他是一个通信工具，通信采用的是驱动操作，通过传输IPC数据来进行通信即可。其他的关于他的详细数据格式和通信协议，感兴趣的同学可以了解一下，但是太过复杂而且在实际中也没多大用途，所以这里就不介绍了。

### 技术点概要
#### 理解远程服务通信机制

通过案例先了解到本地端和服务端跨进程通信，主要就是借助Binder进行功能调用，而在这里主要有两个核心类，一个是Stub类，这个类是继承了Binder类具备了将远程传递的Binder对象转化成本地实际对象asInterface方法即可，同时实现了IXXX接口，需要实现AIDL中的功能方法，还有一个类就是Proxy类，实现了IXXX接口，同时内部保留着远端传递的Binder对象，然后通过这个对象调用远端方法。这里Stub类就是服务端的中间者，而Proxy就是本地端的中间者。

#### 系统服务调用流程

通过分析了跨进程通信机制原理之后，再去看看Android系统中在使用一些服务的时候，通过getSystemService方法获取服务对象，其实这内部就是通过跨进程获取到了远端服务的Binder对象，然后转化成系统服务对象给应用调用，而这些系统服务的Binder对象在系统启动的时候服务会自动注册到ServiceManager中。

#### 服务大管家ServiceManager

在整个远程服务调用过程中两个重要对象，一个是Binder对象，一个就是ServiceManager类，这个类是管理系统服务的类，他可以注册服务，查询服务，系统服务在系统启动的时候会通过addService进行服务注册，然后应用就可以通过getService进行服务查询，而在这个过程中，底层会维护一个这些服务的binder链表结构，同时每个服务的binder对象都一个句柄handle，通过这个句柄来表示通信标识，这样通信才不会紊乱。

#### 底层通信核心Binder

最后分析了底层真正实现跨进程通信的机制Binder，其实是通过虚拟驱动程序/dev/binder进行通信的。一个通信机制肯定有通信协议，传输的数据结构，但是这里并没有介绍这些知识，原因是我们后面的需求并不会用到这些，其次是这些知识点太详细介绍也不好，因为会越看越乱。

### 总结

本文介绍的东东有点多，但是如果掌握了Android中的Binder机制和远程服务调用机制对后面拦截系统api做了铺垫，说到结束了才告诉大家为什么要介绍这个知识点，是因为最近在研究如何拦截系统启动Activity的事，那么就必须了解Activity的启动流程，但是在这个过程中有一个对象就是ActivityManagerService，而他就和Binder以及远程服务调用机制紧密联系了，如果不了解Binder机制，后面工作是没办法进行的，好了，说到最后再来一张神图算是总结了本文内容：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/binder-summary.png)

这张图非常好的表达了Android中应用使用系统服务的一个流程，也是最好的最全的解释了。看懂这张图之后，那么对Android中的binder机制和远程服务调用机制就可以掌握了，可以进行后续的拦截操作了。

参考文献：
[Android系统篇之—-Binder机制和远程服务调用机制分析](http://www.wjdiankong.cn/android%E7%B3%BB%E7%BB%9F%E7%AF%87%E4%B9%8B-binder%E6%9C%BA%E5%88%B6%E5%92%8C%E8%BF%9C%E7%A8%8B%E6%9C%8D%E5%8A%A1%E8%B0%83%E7%94%A8%E6%9C%BA%E5%88%B6%E5%88%86%E6%9E%90/)

[进击的Android注入术《五》](http://blog.csdn.net/L173864930/article/details/38468433)

[Android深入浅出之Binder机制](http://www.cnblogs.com/innost/archive/2011/01/09/1931456.html)
