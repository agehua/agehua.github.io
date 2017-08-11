---
layout: post
title:  Android免root实现hook系统服务拦截方法
category: accumulation
tags: HOOK
keywords: hook, system service, Binder
banner: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Chestnut%20Branches.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Chestnut%20Branches.jpg
toc: true
---

本文转载自博文：[Android系统篇之—-免root实现Hook系统服务拦截方法](http://blog.csdn.net/jiangwei0910410003/article/details/52523679)，结合最新开源的滴滴插件化方案 [VirtualApk](https://github.com/didi/VirtualAPK) 对hook系统服务进行分析。

360之前开源了一款插件框架 [Droid Plugin](https://github.com/DroidPluginTeam/DroidPlugin)。与滴滴同一天，360又开源了一款插件框架 [RePlugin](https://github.com/Qihoo360/RePlugin)。

有关新闻介绍在这里：https://www.itcodemonkey.com/article/278.html
                https://www.itcodemonkey.com/article/277.html

关于360两款插件框架有什么不同，可以查看RePlugin里的[readme](https://github.com/Qihoo360/RePlugin/blob/master/README_CN.md)

本文简单结合**VirtualApk**源码来记录实现hook系统服务方法。

### Android免root实现hook系统服务拦截方法

#### Binder机制回顾
<!--more-->
在之前一篇文章中介绍了 Android中的Binder机制和系统远程服务调用机制，本文将继续借助上一篇的内容来实现Hook系统服务拦截指定方法的逻辑，了解了上一篇文章之后，知道系统的服务其实都是一个远程Binder对象，而这个对象都是由ServiceManager大管家管理的，用户在使用系统服务的时候，会通过指定服务的Stub方法的asInterface把远程的Binder对象转化成本地化对象即可使用，而在这个过程中，我们也知道因为系统服务是在system_server进程中的，所以这个系统服务使用过程中属于跨进程调用，那么返回的对象其实就是Proxy代理对象。

#### 系统中服务使用流程
本文主要就是借助这个知识点，通过Hook系统的服务来拦截服务方法，下面我们就通过**系统剪切板服务**案例作为分析
~~~ Java
//获取剪切板服务
ClipboardManager cm =(ClipboardManager)getSystemService(Context.CLIPBOARD_SERVICE);
//设置剪切板内容
cm.setPrimaryClip(ClipData.newPlainText("data","jack"));
//获取剪切板数据对象
ClipData cd = cm.getPrimaryClip();
String msg = cd.getItemAt(0).getText().toString();
Log.d("jw", "msg:"+ msg);
~~~

这里看到了，使用系统服务的时候都是用了getSystemService方法，通过定义在Context中的服务描述符常量来获取服务对象，而getSystemService方法定义在ComtextImpl.java类中：
~~~ Java
@Override
public Object getSystemService(String name) {
   ServiceFetcher fetcher = SYSTEM_SERVICE_MAP.get(name);
   return fetcher == null ? null : fetcher.getService(this);
}
~~~
这里维护了一个ServiceFetcher的Map结构，看看这个结构在哪里填充数据的：
~~~ Java
private static void registerService(String serviceName, ServiceFetcher fetcher) {
    if (!(fetcher instanceof StaticServiceFetcher)) {
        fetcher.mContextCacheIndex = sNextPerContextServiceCacheIndex++;
    }
    SYSTEM_SERVICE_MAP.put(serviceName, fetcher);
}
~~~
在registerService方法中添加一个服务名称和一个ServiceFetcher对象，而这个方法在静态代码块中进行调用的:
~~~ Java
static {
    registerService(ACCESSIBILITY_SERVICE, new ServiceFetcher() {
            public Object getService(ContextImpl ctx) {
                return AccessibilityManager.getInstance(ctx);
            }});
    //...
    registerService(CLIPBOARD_SERVICE, new ServiceFetcher() {
            public Object createService(ContextImpl ctx) {
                return new ClipboardManager(ctx.getOuterContext(),
                            ctx.mMainThread.getHandler());
            }});        
    //....

    registerService(CONNECTIVITY_SERVICE, new ServiceFetcher() {
            public Object createService(ContextImpl ctx) {
                IBinder b = ServiceManager.getService(CONNECTIVITY_SERVICE);
                return new ConnectivityManager(IConnectivityManager.Stub.asInterface(b));
            }});
    //....
}
~~~
> ClipboardManager这个服务也在这个代码块中注册了

这里其实是一个ClipboardManager对象，其实这个对象是内部封装了IClipboard.Stub功能，可以看看其他的服务，比如上面的联网服务，直接调用了IConnectivityManager.Stub类的asInterface方法获取Proxy对象。

下面就进去ClipboardManager.java中看看究竟：
~~~ Java
public void setPrimaryClip(ClipData clip) {
    try {
        if (clip != null) {
            clip.prepareToLeaveProcess();
        }
        getService().setPrimaryClip(clip, mContext.getOpPackageName());
    } catch (RemoteException e) {
    }
}
~~~
看到这里的设置剪切板内容的方法，其实内部是调用了getService方法获取对象然后在调用指定方法，那么可以大概知道了这个getService方法返回的应该就是IClipboard.Stub通过asInterface方法返回的Proxy对象：
~~~ Java
static private IClipboard getService() {
    synchronized (sStaticLock) {
        if (sService != null) {
            return sService;
        }
        IBinder b = ServiceManager.getService("clipboard");
        sService = IClipboard.Stub.asInterface(b);
        return sService;
    }
}
~~~
吧，果然是这样，这里通过ServiceManager获取到Clipboard的远端IBinder对象，然后通过asInterface方法返回一个Proxy对象即可。

到这里我们就简单的分析完了系统中的获取剪切板的服务，其实系统中的服务都是这么个逻辑，只是有的可能会在外面包装一层罢了，下面总结一下流程：

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/clipboard-ipc-process.png)

现在只要记住一点：每次获取系统服务的流程都是一样的，先通过ServiceManager的getService方法获取远端服务的IBinder对象，然后在通过指定服务的Stub类的asInterface方法转化成本地可使用对象，而这个对象其实就是一个Proxy对象，在这个过程中，Stub类继承了Binder对象和实现了AIDL接口类型，Proxy对象实现了AIDL接口类型，而AIDL接口类型实现了IInterface接口类型。

### Hook系统服务
上面分析完了Android中系统服务的使用流程以及原理解析，下面在来看一下android中实现Hook机制的方法和原理解析，我们知道其实在很多系统中都存在这样一个Hook技术，有的也叫作钩子，但是不管任何系统，Hook技术的核心点都是一样的，只有两点即可完成Hook技术：

- 1、找到Hook点，即你想Hook哪个对象，那么得先找到这个对象定义的地方，然后使用反射获取到这个对象实例。所以这里可以看到，一般Hook点都是一个类的**单例方法**或者是**静态变量**，因为这样的话Hook起来就非常方便，都是static类型，反射调用都比较方便无需具体的实例对象即可。而关于这个点也是整个Hook过程中最难的点，因为很难找到这个点。Android中主要是依靠分析系统源码类来做到的。
- 2、构造一个Hook原始对象的代理类，关于这个代理其实在Java中有两种方式，一种是静态代理，一种是动态代理。
  - 静态代理：代理类中维护一个原始对象的成员变量，每个方法调用之前调用原始对象的方法即可。无需任何条件限制
  - 动态代理：比静态代理复杂点就是有一个规则：就是原始对象必须要实现接口才可以操作，原理是因为动态代理其实是自动生成一个代理类的字节码，类名一般都是Proxy$0啥的，这个类会自动实现原始类实现的接口方法，然后在使用反射机制调用接口中的所有方法。

![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/service-proxy-common.png)

下面结合VirtualApk源码，分析这个原理：

    这里的代码与原博文里的代码有所不同，原博文是结合DroidPlugin进行分析


在VirtualApk的源码com/didi/virtualapk/delegate/目录下，有两个Proxy结尾的类，这两个类动态代理模式相同，我们只看一个。在IContentProviderProxy.java文件中
~~~ Java
public class IContentProviderProxy implements InvocationHandler {

  //.....代码省略
  public static IContentProvider newInstance(Context context, IContentProvider iContentProvider) {
      return (IContentProvider) Proxy.newProxyInstance(iContentProvider.getClass().getClassLoader(),
              new Class[] { IContentProvider.class }, new IContentProviderProxy(context, iContentProvider));
  }

  @Override
  public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
      Log.v(TAG, method.toGenericString() + " : " + Arrays.toString(args));
      wrapperUri(method, args);

      try {
          return method.invoke(mBase, args);
      } catch (InvocationTargetException e) {
          throw e.getTargetException();
      }
  }

  //...代码省略
}
~~~

看到这里，了解java AOP技术的已经知道IContentProviderProxy这个类使用了**java.lang.ref.proxy**方案来实现动态生成代理类。

> 不了解ASM和InvocationHandler的同学可以看IBM的这篇文章：[AOP 的利器：ASM 3.0 介绍](https://www.ibm.com/developerworks/cn/java/j-lo-asm30/index.html)

#### 简单介绍下java本身的动态代理机制

在java的动态代理机制中，有两个重要的类或接口，一个是**InvocationHandler(Interface)**、另一个则是**Proxy(Class)**，这一个类和接口是实现我们动态代理所必须用到的。

这里借用上面IBM的文章中的例子：
Account类是一个接口，具体操作由AccountImpl类实现
~~~ Java
public interface Account {
    void operation();
}
~~~
~~~ Java
public class AccountImpl extends Account{
    public void operation() {
        System.out.println("operation...");
        //TODO real operation
    }
}
~~~
现在的要求是在Account的operation()方法执行前加入一个checkSecurity()检查。

那么使用InvocationHandler和Proxy的实现方式就是：
~~~ Java
class SecurityProxyInvocationHandler implements InvocationHandler {
    private Object proxyedObject;
    public SecurityProxyInvocationHandler(Object o) {
        proxyedObject = o;
    }

    @Override
    public Object invoke(Object object, Method method, Object[] arguments)
        throws Throwable {             
        if (object instanceof Account && method.getName().equals("opertaion")) {
            SecurityChecker.checkSecurity();
        }
        return method.invoke(proxyedObject, arguments);
    }
}
~~~
~~~ Java
public static void main(String[] args) {
    Account account = (Account) Proxy.newProxyInstance(
        Account.class.getClassLoader(),
        new Class[] { Account.class },
        new SecurityProxyInvocationHandler(new AccountImpl())
    );
    account.function();
}
~~~
总结如下：
- InvocationHandler这个接口只有一个方法**invoke()**：
~~~ Java
Object invoke(Object proxy, Method method, Object[] args) throws Throwable
~~~
  - proxy:　　指代我们所代理的那个真实对象
  - method:　　指代的是我们所要调用真实对象的某个方法的Method对象
  - args:　　指代的是调用真实对象某个方法时接受的参数

每一个动态代理类都必须要实现InvocationHandler这个接口，当我们通过代理对象（这里就是account对象）调用一个方法的时候，这个方法的调用就会被转发为由InvocationHandler这个接口的**invoke**方法来进行调用

- Proxy类使用的最多的就是**newProxyInstance**这个方法：
~~~ Java
public static Object newProxyInstance(ClassLoader loader, Class<?>[] interfaces, InvocationHandler h) throws IllegalArgumentException
~~~
  - loader:　　一个ClassLoader对象，定义了由哪个ClassLoader对象来对生成的代理对象进行加载
  - interfaces:　　一个Interface对象的数组，表示的是我将要给我需要代理的对象提供一组什么接口，如果我提供了一组接口给它，那么这个代理对象就宣称实现了该接口(多态)，这样我就能调用这组接口中的方法了
  - h:　　一个InvocationHandler对象，表示的是当我这个动态代理对象在调用方法的时候，会关联到哪一个InvocationHandler对象上

Proxy类并不负责实例化对象，newProxyInstance()方法的作用是动态创建一个代理对象的类。

Proxy动态生成代理的不足之处在于：
- Proxy 是面向接口的，所有使用 Proxy 的对象都必须定义一个接口，而且用这些对象的代码也必须是对接口编程的：Proxy 生成的对象是接口一致的而不是对象一致的：例子中 **Proxy.newProxyInstance** 生成的是实现 Account接口的对象而不是 AccountImpl 的子类。这对于软件架构设计，尤其对于既有软件系统是有一定掣肘的。
- Proxy 毕竟是通过反射实现的，必须在效率上付出代价：有实验数据表明，调用反射比一般的函数开销至少要大10倍。而且，从程序实现上可以看出，对 proxy class 的所有方法调用都要通过使用反射的 invoke 方法。因此，对于性能关键的应用，使用 proxy class 是需要精心考虑的，以避免反射成为整个应用的瓶颈。

> 对比VirtualApk框架中的 IContentProviderProxy 类，和上面Account例子里的 SecurityProxyInvocationHandler 类，可以发现，IContentProviderProxy 使用了 **InvocationHandler 的动态代理机制**，而代理的具体内容就在**invoke()**回调里。

好的，现在在回到VirtualApk的IContentProviderProxy源码的invoke()方法里。invoke() 方法中调用了wrapperUri()，来看下这个方法：
~~~ Java
private void wrapperUri(Method method, Object[] args) {
    Uri uri = null;
    int index = 0;
    if (args != null) {
        for (int i = 0; i < args.length; i++) {
            if (args[i] instanceof Uri) {
                uri = (Uri) args[i];
                index = i;
                break;
            }
        }
    }

    //如果挂钩的是call方法
    Bundle bundleInCallMethod = null;
    if (method.getName().equals("call")) {
        bundleInCallMethod = getBundleParameter(args);
        if (bundleInCallMethod != null) {
            String uriString = bundleInCallMethod.getString(KEY_WRAPPER_URI);
            if (uriString != null) {
                uri = Uri.parse(uriString);
            }
        }
    }
    //...

    PluginManager pluginManager = PluginManager.getInstance(mContext);
    //这里根据uri找到对应的ContentProvider
    ProviderInfo info = pluginManager.resolveContentProvider(uri.getAuthority(), 0);
    if (info != null) {
        String pkg = info.packageName;
        LoadedPlugin plugin = pluginManager.getLoadedPlugin(pkg);
        String pluginUri = Uri.encode(uri.toString());
        StringBuilder builder = new StringBuilder(PluginContentResolver.getUri(mContext));
        builder.append("/?plugin=" + plugin.getLocation());
        builder.append("&pkg=" + pkg);
        builder.append("&uri=" + pluginUri);
        Uri wrapperUri = Uri.parse(builder.toString());
        if (method.getName().equals("call")) {
            bundleInCallMethod.putString(KEY_WRAPPER_URI, wrapperUri.toString());
        } else {
            args[index] = wrapperUri;
        }
    }
}
~~~

- 1.从wrapperUri()的第二个参数找到Uri
- 2.调用resolveContentProvider()方法，根据uri找到占坑的ContentProvider
  ~~~ Java
  public ProviderInfo resolveContentProvider(String name, int flags) {
    return this.mProviders.get(name);
  }
  ~~~
  ProviderInfo对象的解释就是：Holds information about a specific content provider
- 3.使用StringBuilder对，将uri，pkg，plugin等参数等拼接上去，替换到args中的uri，然后继续走原本的流程。

> 假设是调用了query方法，应该就可以到达占坑的provider的query方法了。这就是插件框架里传说中的占坑，即不用注册就可以启动插件里的组件啦

#### 剪切板实例
到此我们了解了Java中的Hook技术的核心知识点了，下面就用开始的剪切板服务来做实验，我们Hook系统的剪切板服务功能，拦截其方法，上面也说道了，既然要Hook服务，首先得找到Hook点，通过开始对Android中系统服务的调用流程分析知道，其实这些服务都是一些保存在ServiceManager中的远端IBinder对象，这其实是一个Hook点：

~~~ Java
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
其实ServiceManager中每次在获取服务的时候，其实是先从一个缓存池中查找，如果有就直接返回了：
~~~ Java
private static HashMap<String, IBinder> sCache = new HashMap<String, IBinder>();
~~~
> 这个缓存池正好是全局的static类型，所以就可以很好的使用反射机制获取到它了，然后进行操作了

接下来，我们就需要构造一个剪切板的服务IBinder对象了，然后在把这个对象放到上面得到的池子中即可。

那么按照上面的动态代理的流程（使用Proxy Java原生动态代理）
- 第一、原始对象必须实现一个接口，这里也正好符合这个规则，每个远程服务其实是实现了IBinder接口的。
- 第二、其次是要有原始对象，这个也可以，通过上面的缓存池即可获取。

有了这两个条件那么接下来就可以使用动态代理构造一个代理类了：
~~~ Java
try {
  //下面这一段的意思其实就是ServiceManager.getService("clipboard")
  //只不过ServiceManager这个类是@hide的
  Class<?> serviceManager = Class.forName("android.os.ServiceManager");
  Method getService = serviceManager.getDeclaredMethod("getService", String.class);
  //取得ServiceManager里的原始的clipboard binder对象
  //一般来说这是一个Binder代理对象
  IBinder rawBinder =(IBinder) getService.invoke(null, Context.CLIPBOARD_SERVICE);

  //Hook掉这个Binder代理的queryLocalInterface 方法
  //然后在queryLocalInterface返回一个IInterface对象，hook掉我们感兴趣的方法即可
  IBinder hookedBinder =(IBinder) Proxy.newProxyInstance(
      serviceManager.getClassLoader,
      new Class<?>[](IBinder.class),
      new IClipboardHookBinderHandler(rawBinder)
  );

  //放回ServiceManager中，替换掉原有的
  Field cacheField = serviceManager.getDeclaredField("sCache");
  cacheField.setAccessible(true);
  @SuppressWarnings({"unchecked"})
  Map<String, IBinder> cache = (Map<String,IBinder>) cacheField.get(null);
  cache.put(Context.CLIPBOARD_SERVICE, hookedBinder);
}catch (Exception e){

}
~~~

> Field.get()可以返回一个Object，字段不是静态字段的话，要传入反射类的对象。如果传null是会报 java.lang.NullPointerException。但是如果字段是静态字段的话,传入任何对象都是可以的,包括null

这里是通过反射去获取ServiceManager中的缓存池Binder对象。我们先获取到缓存池，然后得到剪切板服务Binder对象，构造一个代理类，最后在设置回去即可。

下面主要来看一下构造了代理类之后，如何拦截哪些方法？
~~~ Java
@Override
public Object invoke(Object object, Method method, Object[] args)
    throws Throwable {             
    if (method.getName().equals("queryLocalInterface")) {
        Log.d(TAG, "hook queryLocalInterface");

        //这里直接返回真正被hook掉的Service接口
        //这个代理类必须实现IInterface接口
        return Proxy.newProxyInstance(
            base.getClassLoader,
            new Class<?>[](this.iinterface),
            new HookBinderInvocationHander(base,stub)
            );
    }
    return method.invoke(base, args);
}
~~~
> 因为拦截的是queryLocalInterface()方法，这个方法返回的是一个远端的服务，还没有转化为本地对象，所以不能去拦截具体的服务方法

这里一定要注意了，有的同学可能想直接在这里拦截setPrimaryClip这样的剪切板方法不就可以了吗？想想是肯定不可以的，为什么呢？因为我们现在代理的是远端服务的Binder对象，他还没有转化成本地对象呢？如何会有这些方法呢，而我们真正要拦截的方法是IClipboardManager，其实就是Proxy类，而这个对象也是Stub类的asInterface方法得到的，所以我们现在的思路是有了远端服务的代理对象，拦截肯定是拦截这个代理对象Binder的一些方法，那么这个远端服务有哪些方法会在这个过程中被调用呢？我们再看看之前的一个简单AIDL的例子：
~~~ Java
public static com.agehua.aidldemo.Demo asInterface(android.os.IBinder obj) {
    if ((obj == null)) {
        return null;
    }
    //这里的obj就是远端对象，它通过queryLocalInterface()方法获取本进程中的服务，这里就是hook点
    android.os.IInterface iin = obj.queryLocalInterface(DESCRIPTOR);
    if (((iin != null) && (iin instanceof com.agehua.aidldemo.Demo))) {
        return ((com.agehua.aidldemo.Demo) iin);
    }
    return new com.agehua.aidldemo.Demo.Stub.Proxy(obj);
}
~~~

然后在想，我们如果想拦截IClipboardManager的setPrimaryClip方法，其实就是要拦截ClipboardManager$Proxy的这些方法，那么还需要做一次代理，代理ClipboardManager$Proxy类对象

- 第一、ClipboardManager$Proxy类实现了AIDL接口类型，符合规则。
- 第二、我们可以直接使用反射获取到IClipboardManager$Stub类，然后反射调用它的asInterface方法就可以得到了IClipboardManager$Proxy对象了，符合规则。

~~~ Java
public IClipboardHookBinderHandler(IBinder base) {
  this.base = base;
  try {
    this.stub =Class.forName("android.content.IClipboard$Stub");
    this.iinterface = Class.forName("android.content.IClipboard");
  }catch (ClassNotFoundException e){

  }
}
~~~
到这里，看来这个对象也符合了代理的条件，那么就简单了，继续使用动态代理机制产生一个代理类即可：
~~~ Java
public IClipboardHookBinderHandler(IBinder base, Class<?> stubClass) {
  try {
    Method asInterfaceMethod = stubClass.getDeclaredMethod("asInterface", IBinder.class);
    this.base = asInterfaceMethod.invoke(null, base);
  }catch (Exception e){
    throw new RuntimeException("hooked failed")
  }
}  
~~~
这个代理类的InvocationHandler中，先需要通过反射获取到Proxy原始对象：
~~~ Java
@Override
public Object invoke(Object proxy, Method method, Object[] args) throws Throwable{
  if ("getPrimaryClip".equals(method.getName())) {
      return ClipData.newPlainText(null, "you are hooked");
  }
  //欺骗系统，使之认为剪切板上一直有内容
  if ("hasPrimaryClip".equals(method.getName())) {
    return true;
  }
  //千万不要忘了调用原始对象的方法
  return method.invoke(base, args);
}
~~~
到这里就已经完成了hook剪切板服务的整个步骤，再看一下流程图：
![](http://oui2w5whj.bkt.clouddn.com/blogimages/2017/hook-clipboard-process.png)

- 1、我们的目的就是拦截系统的服务功能，那么最开始的入口就是服务大管家ServiceManager对象，而在他内部也正好有一个远端服务对象的IBinder缓存池，那么这个变量就是我们操作的对象了，可以先使用反射机制去获取到他，然后在获取到指定的剪切板服务IBinder对象实例。
- 2、下一步肯定是Hook这个剪切板服务的Binder对象，这里采用动态代理方式产生一个Binder对象代理类，符合两个规则：
  - 1) 这个Binder对象实现了IBinder接口类型
  - 2) 我们已经得到了原始的Binder对象实例
构造完代理类之后，我们拦截的方法是queryLocalInterface方法，为什么是这个方法呢？因为在整个服务使用过程中之后在Stub类中使用到了这个方法，很多同学会认为为什么不在这里直接拦截系统方法呢？这是一个误区，要想清楚，这里的代理对象是远程服务的Binder，还不是本地化对象，不能会有哪些系统方法的，所以得再做一次Hook，去Hook住系统的本地化对象。
- 3、在拦截了Binder对象的queryLocalInterface方法之后，再一次做一下本地化服务对象的代理生成操作，而这个本地化对象一般都是IClipboard$Proxy，那么动态代理的规则：
  - 1) 本地化服务对象都会实现AIDL接口类型(这里才有哪些我们想拦截的系统方法)
  - 2) 通过反射调用IClipboard$Stub类的asInterface方法得到IClipboard$Proxy类对象实例
符合这两个规则那么就可以产生代理对象了，然后开始拦截服务的指定方法即可。

> 这个Hook系统服务只对本应用有效。真正能够拦截系统并对所有应用有效的，需要hook进system_server进程中，所以就需要root权限

### 总结
到这里我们就介绍完了Android中Hook系统服务的流程，本文中主要介绍了Hook系统剪切板服务，拦截指定方法。
同时，对VirtualApk插件框架感兴趣的同学也可以看看**张鸿洋**的这篇文章：[ 滴滴插件化方案 VirtualApk 源码解析](http://blog.csdn.net/lmj623565791/article/details/75000580)
