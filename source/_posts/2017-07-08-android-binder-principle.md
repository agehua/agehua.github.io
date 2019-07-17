---
layout: post
title:  Android Binder机制分析（一）
category: accumulation
tags:
  - AIDL
  - Binder
keywords: AIDL, Binder
banner: http://cdn.conorlee.top/Blossoming%20Almond%20Tree.jpg
thumbnail: http://cdn.conorlee.top/Blossoming%20Almond%20Tree.jpg
toc: true
---

### 背景分析
最近在学习Android非侵入Hook机制，

6月30日，360开源RePlugin，安卓进入“全面插件化”时代：https://www.itcodemonkey.com/article/278.html

同一天，滴滴开源Android端插件化框架VirtualAPK：https://www.itcodemonkey.com/article/277.html

然后本人发现竟然可以不在清单文件中注册就可以启动Activity，还有这种操作？哈哈。后面会有单独的文章介绍这种骚操作。

在搜索中发现了这篇博客，[
Android系统篇之—-Binder机制和远程服务调用机制分析](http://www.wjdiankong.cn/android%E7%B3%BB%E7%BB%9F%E7%AF%87%E4%B9%8B-binder%E6%9C%BA%E5%88%B6%E5%92%8C%E8%BF%9C%E7%A8%8B%E6%9C%8D%E5%8A%A1%E8%B0%83%E7%94%A8%E6%9C%BA%E5%88%B6%E5%88%86%E6%9E%90/)。本文大部分转载自原来博客，结合自己的分析，是研究Hook机制前的知识储备。


### Android中远程服务调用分析
简单介绍，跨进程调用一个远程服务需要下面这几步：
<!--more-->
#### 1.定义一个AIDL文件：Demo.aidl
> 类似于定义接口类型，这个AIDL文件将在本地和远端都要使用到

~~~ Java
package com.agehua.aidldemo;
interface Demo {
    int sendData(String data);
    String getData();
}
~~~


#### 2.定义远程服务
在远程服务中的onBind方法，实现AIDL接口的具体方法，并且返回Binder对象
~~~ Java
//远程服务，应该定义在另个一进程中
public class DemoService extends Service {

    @Override
    public IBinder onBind(Intent intent) {
       //返回远程的Binder对象，并且实现类
       return new Demo.Stub() {
          @Override
          public int sendData(String data) throws RemoteException {
            return 0;
          }

          @Override
          public String getData() throws RemoteException {
            return "";
          }
       }
    }
}
~~~

> 接口方法的具体传递实现都是在远端服务中。

#### 3.本地创建连接对象
本地创建一个服务连接对象，实现ServiceConnection接口，在连接成功之后，会得到一个远端传递过来的Binder对象，就是上面的远端服务onBind方法返回的，得到Binder对象之后在进行转化就可以得到AIDL对象，然后即可调用方法。

~~~ Java
//连接远程服务的回调
public class DemoConnection implements ServiceConnection {

    @Override
    public void onServiceConnected(Component name, IBinder service) {
      //连接成功后，会传递远端的Binder对象
      Demo demo  =Demo.Stub.asInterface(service);
      try {
        demo.setData(" ");
        demo.getData();
      }catch (RemoteException e) {
        e.printStackTrace();
      }
    }
    @Override
    public void onServiceDisconnected(Component name) {
      //断开连接
    }
}
~~~
> 连接成功后，从远端服务中获取到了Binder对象，然后在转化成本地接口对象，即可调用方法。

#### 4、连接服务
连接服务也是比较简单的，这时候把上面的连接对象传递进去即可

~~~ Java
Intent intent = new Intent(this, DemoService.class);
bindService(intent, new DemoConnection(), Context.BIND_AUTO_CREATE);
~~~

### AIDL实现机制分析
上面的步骤就可以实现一个远程服务调用了。但是有一个核心的地方就是**Demo.Stub类**，这个类起着重要的作用，下面来分析一下它的实现：

> 每次定义了AIDL接口文件之后，编译一下就会在build/generated/source/目录中产生对应的java文件了：

~~~ Java
package com.agehua.aidldemo;
//IInterface接口由AIDL类去实现。IInterface接口包含一个方法asBinder()
public interface Demo extends android.os.IInterface {
    /**
     * Local-side IPC implementation stub class.
     * 由Stub类实现Binder类和AIDL接口
     */
    public static abstract class Stub extends android.os.Binder implements com.agehua.aidldemo.Demo {
        private static final java.lang.String DESCRIPTOR = "com.agehua.aidldemo.Demo";

        public Stub() {
            this.attachInterface(this, DESCRIPTOR);
        }

        /**
         * Cast an IBinder object into an com.agehua.aidldemo.Demo interface,
         * generating a proxy if needed.
         * 将远端传过来的Binder对象转化成本地对象
         */
        public static com.agehua.aidldemo.Demo asInterface(android.os.IBinder obj) {
            if ((obj == null)) {
                return null;
            }
            //如果本地进程和服务端都在一个进程中，那么直接返回当前类的IInterface
            android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
            if (((iin != null) && (iin instanceof com.agehua.aidldemo.Demo))) {
                return ((com.agehua.aidldemo.Demo) iin);
            }
            //如果本地进程和服务端不在一个进程中，则返回一个代理对象给客户端
            return new com.agehua.aidldemo.Demo.Stub.Proxy(obj);
        }

        @Override
        public android.os.IBinder asBinder() {
            return this;
        }

        //处理客户端发过来的请求方法，这里不详细展开了
        @Override
        public boolean onTransact(int code, android.os.Parcel data, android.os.Parcel reply, int flags) throws android.os.RemoteException {
            switch (code) {
                case INTERFACE_TRANSACTION: {
                    reply.writeString(DESCRIPTOR);
                    return true;
                }
                case TRANSACTION_sendData: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _arg0;
                    _arg0 = data.readString();
                    int _result = this.sendData(_arg0);
                    reply.writeNoException();
                    reply.writeInt(_result);
                    return true;
                }
                case TRANSACTION_getData: {
                    data.enforceInterface(DESCRIPTOR);
                    java.lang.String _result = this.getData();
                    reply.writeNoException();
                    reply.writeString(_result);
                    return true;
                }
            }
            return super.onTransact(code, data, reply, flags);
        }

        //稍后分析这个类
        private static class Proxy implements android.app.IServiceConnection {
          /**
          ...
          **/
        }

    }

    //Demo.aidl提供的方法，由Proxy类去实现，这里不用实现
    public int sendData(java.lang.String data) throws android.os.RemoteException;
    //Demo.aidl提供的方法，由Proxy类去实现，这里不用实现
    public java.lang.String getData() throws android.os.RemoteException;
}
~~~
#### 1、AIDL接口必须实现IInterface接口

IInterface接口包含一个asBinder()方法，由这个方法进行转化对象功能，把当前的AIDL对象转化成一个IBinder对象。
~~~ Java
package android.os;

public interface IInterface
{
    /**
     * Retrieve the Binder object associated with this interface.
     * You must use this instead of a plain cast, so that proxy objects
     * can return the correct result.
     */
    public IBinder asBinder();
}
~~~
#### 2、AIDL接口中肯定有一个静态实现类Stub

这个类必须实现Binder类，以及本身的AIDL接口类型。那么这个类就具备了Binder类中的四个功能：

- 1.可以将Binder对象转化成AIDL对象，调用asInterface方法，可以看到这个方法其实和上面的asBinder方法对立的

- 2.通信方法onTransact实现，这个方法是最核心的用于通信之间的逻辑实现

- 3.通过queryLocalInterface方法可以根据类的描述符(字符串可以唯一标识这个远端服务的名称即可)获取到对应的AIDL对象(其实是IInterface类型的)

- 4.在构造方法中必须调用Binder中的attachInterface方法把当前服务对象和描述符进行关联

#### 3、Stub类只是中间者，由Proxy类生成服务端的代理
> 为什么说是由Proxy类生成服务端的代理的呢？

因为在上面的DemoConnection类中，生成本地Demo对象，是调用了Demo.Stub.asInterface(IBinder)这个方法。
前面提到，服务端和客户端不在同一个进程的时候，asInterface()方法实际上调用了Demo.Stub.Proxy(IBinder)这个方法。

而且Demo.aidl中定义的抽象方法，具体都是由Proxy类去实现的。

Stub类，其实只是远端服务Binder对象的一个中间者，下面看代码：

~~~ Java
//实现了aidl接口类。
private static class Proxy implements com.agehua.aidldemo.Demo {
      //保存了一个mRemote变量，这个变量就是由服务端传递过来的IBinder对象
      private android.os.IBinder mRemote;

      Proxy(android.os.IBinder remote) {
          mRemote = remote;
      }

      @Override
      public android.os.IBinder asBinder() {
          return mRemote;
      }

      public java.lang.String getInterfaceDescriptor() {
          return DESCRIPTOR;
      }

      @Override
      public int sendData(java.lang.String data) throws android.os.RemoteException {
          android.os.Parcel _data = android.os.Parcel.obtain();
          android.os.Parcel _reply = android.os.Parcel.obtain();
          int _result;
          try {
              _data.writeInterfaceToken(DESCRIPTOR);
              _data.writeString(data);
              //调用Binder的transact()方法，会调用上面Stub类中的onTransact方法进一步处理
              mRemote.transact(Stub.TRANSACTION_sendData, _data, _reply, 0);
              _reply.readException();
              _result = _reply.readInt();
          } finally {
              _reply.recycle();
              _data.recycle();
          }
          return _result;
      }

      @Override
      public java.lang.String getData() throws android.os.RemoteException {
          android.os.Parcel _data = android.os.Parcel.obtain();
          android.os.Parcel _reply = android.os.Parcel.obtain();
          java.lang.String _result;
          try {
              _data.writeInterfaceToken(DESCRIPTOR);
              //调用Binder的transact()方法，会调用上面Stub类中的onTransact方法进一步处理
              mRemote.transact(Stub.TRANSACTION_getData, _data, _reply, 0);
              _reply.readException();
              _result = _reply.readString();
          } finally {
              _reply.recycle();
              _data.recycle();
          }
          return _result;
      }
}
~~~
> Proxy是Stub类中的一个静态类，Proxy对象就是远端传递过来的Binder对象在本地的代理。这里用到的是静态代理模式。

在服务连接成功后，在onServiceConnected()方法中，返回一个服务端Binder对象，本地通过asInterface()方法生成的一个代理；
~~~ Java
Demo demo = Demo.Stub.asInterface(IBinder);
~~~
这个demo对象，就是客户端这边用户和服务端交互的中间者。我们在前面的**Stub类的asInterface()方法实现**中可以看到：

借助**queryLocalInterface()**方法根据服务描述符来获取对象，会把远端传递过来的Binder对象转化成一个本地对象：
~~~ Java
public IInterface queryLocalInterface(String descriptor) {
    if (mDescriptor.equals(descriptor)) {
        return mOwner;
    }
    return null;
}
~~~

而这个mOwner和mDescriptor之间的对应关系就在attachInterface方法中进行初始化的，也就是在Stub类的构造方法中
~~~ Java
public void attachInterface(IInterface owner, String descriptor) {
       mOwner = owner;
       mDescriptor = descriptor;
}
~~~


那么现在就清楚了，如果客户端和服务端是在一个进程中，那么其实queryLocalInterface获取的就是Stub对象，如果不在一个进程queryLocalInterface查询的对象肯定为null，因为**new Demo.Stub()**和**Demo.Stub.asInterface(IBinder)**方法分别是在**远端进程**和**本地进程**中调用的，在不同进程有不同虚拟机，肯定查不到mOwner对象的，所以这时候其实是返回的Proxy对象了。

通过上面的讲解之后，发现多进程服务通信基准就是借助Binder对象，先传递Binder对象，然后在把Binder转成可以使用的原生对象即可调用了，而对于Stub类和Proxy类其实就是相当于是服务端和客户端的中间者，把一些逻辑封装起来，这种设计也会显得不是那么凌乱：
![](http://blog.conorlee.top/blogimages/2017/binder-principle.png)


### 分析系统服务调用流程
其实系统中的一些服务使用的时候其实也是跨进程使用，比如下面来看一下著名的PackageManager，IPackageManager，PackageManagerService体系：

PackageManagerService是Android系统中最常用的服务之一。它负责系统中Package的管理，应用程序的安装、卸载、信息查询等。PackageManager获取的信息即来自AndroidManifest.XML

![PackageManagerService体系](http://blog.conorlee.top/blogimages/2017/PackageManagerService.png)

~~~Java
interface IPackageManager {
  boolean isPackageAvailable(String packageName, int userId);
  PackageInfo getPackageInfo(String packageName, int flags, int userId);
  int getPackageUid(String packageName, int userId);
  int[] getPackageGids(String packageName);

  String[] currentToCanonicalPackageNames(String[] names);
  String[] canonicalToCurrentPackageNames(String[] names);

  PermissionInfo getPermissionInfo(String name, int flags)

  ParceledListSlice<PermissionInfo> queryPermissionsByGroup(String group,
            int flags)

  //...
}
~~~
上面代码在谷歌的源码中查到，详情点击[链接](https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/content/pm/IPackageManager.aidl)

因为我们还没有编译源码，所以看不到IPackageManager.java，这里可能需要AIDL工具单独编译才能看到了：
~~~ Java
public interface IPackageManager extends android.os.IInterface {
    //定义内部类Stub，派生自Binder，实现IPackageManager接口
    public static abstract class Stub extends android.os.Binder implements android.content.pm.IPackageManager {

        private static final java.lang.String DESCRIPTOR = "android.content.pm.IPackageManager";
        publicStub() {
             this.attachInterface(this,DESCRIPTOR);
        }

        //......

        //定义Stub的内部类Proxy，实现IPackageManager接口
        private static class Proxy implements android.content.pm.IPackageManager{
        //通过mRemote变量和服务端交互
        private android.os.IBinder mRemote;

           Proxy(android.os.IBinderremote) {
              mRemote = remote;
           }
           //......
        }
        //......
}
~~~
> 这里看到了熟悉的远端服务中间者Stub和本地端的中间者Proxy类了，而这两个类的规则都和上面一样的。

下面来看一下远端服务实现代码PackageManagerService.java，(这个类就可以在IDE中看到了)：
~~~ Java
public class PackageManagerService extends IPackageManager.Stub {
    static final String TAG = "PackageManager";
    static final boolean DEBUG_SETTINGS = false;
    static final boolean DEBUG_PREFERRED = false;
    static final boolean DEBUG_UPGRADE = false;
    static final boolean DEBUG_DOMAIN_VERIFICATION = false;
    //...
}
~~~
> 实现了上面的的Stub类功能。

下面我们再走一遍获取PackageManager的流程：
~~~ Java
PackageManager pm = getPackageManager();
~~~
而这个getPackageManager方法是在ContextImpl.java中实现的：

~~~ Java
@Override
public PackageManager getPackageManager() {
    if (mPackageManager != null) {
        return mPackageManager;
    }

    IPackageManager pm = ActivityThread.getPackageManager();
    if (pm != null) {
        // Doesn't matter if we make more than one instance.
        return (mPackageManager = new ApplicationPackageManager(this, pm));
    }

    return null;
}
~~~
具体内容实现是在ActivityThread.getPackageManager()方法中：
~~~ Java
public static IPackageManager getPackageManager() {
    if (sPackageManager != null) {
        //Slog.v("PackageManager", "returning cur default = " + sPackageManager);
        return sPackageManager;
    }
    IBinder b = ServiceManager.getService("package");
    //Slog.v("PackageManager", "default service binder = " + b);
    sPackageManager = IPackageManager.Stub.asInterface(b);
    //Slog.v("PackageManager", "default service = " + sPackageManager);
    return sPackageManager;
}
~~~

看到了吧，IPackageManager.Stub.asInterface(b)中的参数b由**ServiceManager.getService**方法获取到。然后在使用Stub的asInterface方法进行转化成本地的PackageManager对象，其实就是那个Proxy对象。然后就可以通过PackageManager来调用方法和远端的PackageManagerService服务进行通信了。

而DemoService是在DemoConnection（继承自ServiceConnection）的onServiceConnected回调中得到远端的IBinder对象，然后获得Proxy对象。

> 这里与自定义远程服务然后调用的区别就是，系统的远程服务都是由ServiceManager保存的，也就是由系统去创建和管理；而自定义的远程服务由开发者去创建、维护和销毁

通过上面的PackageManager案例可以分析，我们在使用系统中的服务的时候的流程都是如此：

![系统服务远程调用详情](http://blog.conorlee.top/blogimages/2017/binder-system-service.png)

总结一下，每个应用在使用系统服务的时候，都会走这么几步：

- 1、调用getService(String serviceName)方法获取服务对象
- 2、而getSystemService一般都是在ContextImpl类中实现的，其实是调用了ServiceManager的getService方法
- 3、调用ServiceManager的getService方法获取远端服务的IBinder对象
- 4、有了远端服务的IBinder对象之后，在使用远端服务的中间者类Stub进行转化对象asInterface方法
- 5、因为系统中的服务获取都是肯定是跨进程的，远端服务都是在system_server进程中的，所以asInterface方法中返回的是Proxy代理对象，也就是本地端的中间者。
- 6、最后返回的对象其实就是这个Proxy对象，而这个对象内部使用了静态代理方式，内部有一个来自远端的mRemote变量即IBinder对象。然后直接调用方法其实就是调用mRemote的transact方法进行通信了。

所以在这个过程中可以看到有两个对象很重要，一个是ServiceManager，一个是IBinder对象。[下篇文章](/2017/07/10/android-binder-principle2/)再来一一介绍



参考文献：

[Android系统篇之—-Binder机制和远程服务调用机制分析](http://www.wjdiankong.cn/android%E7%B3%BB%E7%BB%9F%E7%AF%87%E4%B9%8B-binder%E6%9C%BA%E5%88%B6%E5%92%8C%E8%BF%9C%E7%A8%8B%E6%9C%8D%E5%8A%A1%E8%B0%83%E7%94%A8%E6%9C%BA%E5%88%B6%E5%88%86%E6%9E%90/)

《Android上玩玩Hook？》: http://blog.csdn.net/yzzst/article/details/47318751

《进击的Android注入术<一>》:
http://blog.csdn.net/l173864930/article/details/38455951

极客学院——深入理解Android卷②：
http://wiki.jikexueyuan.com/project/deep-android-v2/powermanagerservice.html
