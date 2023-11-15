---
layout: post
title: 跨进程通信Bridge建立
category: accumulation
tags:
  - multi-process
keywords: multi-process
banner: https://cdn.conorlee.top/Harvest%20in%20Provence%2C%20at%20the%20Left%20Montmajour.jpg
thumbnail: https://cdn.conorlee.top/Harvest%20in%20Provence%2C%20at%20the%20Left%20Montmajour.jpg
toc: true
---

本文介绍了Android跨进程通信的原理，并对[Hermes](https://github.com/Xiaofei-it/Hermes)的源码进行分析。

先简单总结下：Hermes也是通过AIDL的方式来最终实现跨进程通信，通信的内容是Gson。其中用到的了**动态代理技术**。
后面再详细分析下为什么要用到动态代理

<!--more-->

### 手动创建IInterface类
如何创建AIDL文件本文就不在讲了，感兴趣的可以看我之前的文章：[Android Binder 机制分析（一）](https://conorlee.top/2017/07/08/android-binder-principle/)


### 代理

#### 什么是代理
代理就是客户类不再直接和委托类打交道, 而是通过一个中间层来访问, 这个中间层就是代理。为啥要这样呢, 是因为使用代理有两个优势：

- 1.可以隐藏委托类的实现
- 2.可以实现客户与委托类之间的解耦, 在不修改委托类代码的情况下能够做一些额外的处理

#### 静态代理

接口：
~~~ java
public interface HelloService {
    String sayHello(String content);
}
~~~

接口实现类

~~~ java
public class HelloServiceImpl implements HelloService {
    @Override
    public String sayHello(String content) {
        return "hello " + content;
    }
}
~~~

代理类

~~~ java
public class HelloServiceProxy implements HelloService {
    private HelloService helloService;
    
    public HelloServiceProxy(HelloService helloService) {
        this.helloService = helloService;
    }
    
    @Override
    public String sayHello(String content) {
        System.out.println("before invoke");
        String result = helloService.sayHello(content);
        System.out.println("after invoke");
        
        return result;
    }
}
~~~
这里的代理类其实也满足了一个面向对象的设计原则，即组合优先于集成。



#### 动态代理
Java动态代理是一种在运行时创建**代理类**的方法。它可以在不修改源代码的情况下，在程序运行期间为接口创建代理类。它基于 Java 反射机制实现。

动态代理是一种很有用的工具，可以帮助您在不更改目标对象代码的情况下对其进行包装。

动态代理是为接口创建代理类，所以要提前定义一个接口：
~~~ java
public interface HelloService {
    String sayHello(String content);
}
~~~

HelloServiceImpl实现类
~~~ java
public class HelloServiceImpl implements HelloService {

    @Override
    public String sayHello(String content) {
        return "hello " + content;
    }
}
~~~
使用InvocationHandler，实现动态代理
~~~ java
public class MyInvocationHandler implements InvocationHandler {
    // 真实对象
    private Object obj;

    public MyInvocationHandler(Object obj) {
        this.obj = obj;
    }

    @Override
    public Object invoke(Object proxy, Method method, Object[] args) throws Throwable {
        System.out.println("before invoke......");
        // 此处通过反射，调用真实对象的相关方法
        Object result = method.invoke(obj, args);
        System.out.println("after invoke......");
        return result;
    }
}
~~~
如何使用：
~~~ java
public class DynamicProxyTest {

    public static void main(String[] args) {
        String content = "dynamic proxy";
        // 创建真实实现对象
        HelloService helloService = new HelloServiceImpl();
        // 创建调用器
        InvocationHandler handler = new MyInvocationHandler(helloService);
        // 创建代理实例
        Object proxy = Proxy.newProxyInstance(helloService.getClass().getClassLoader(), helloService.getClass()
                .getInterfaces(), handler);
        System.out.println("代理类名称:" + proxy.getClass().getName());
        System.out.println("代理类接口:" + proxy.getClass().getInterfaces()[0].getName());
        // 类型转换
        HelloService proxyService = (HelloService) proxy;
        // 执行代理逻辑
        System.out.println(proxyService.sayHello(content));
    }
}
~~~
输出：
~~~ java
代理类名称:com.sun.proxy.$Proxy0
代理类接口:com.sankuai.meituan.zcm.depot.proxy.HelloService
before invoke......
after invoke......
hello dynamic proxy
~~~

#### 动态代理分析
~~~ java
public final class $Proxy0 extends Proxy implements HelloService {
    private static Method m1;
    private static Method m3;
    private static Method m2;
    private static Method m0;

    public $Proxy0(InvocationHandler var1) throws  {
        super(var1);
    }

    public final boolean equals(Object var1) throws  {
        try {
            return (Boolean)super.h.invoke(this, m1, new Object[]{var1});
        } catch (RuntimeException | Error var3) {
            throw var3;
        } catch (Throwable var4) {
            throw new UndeclaredThrowableException(var4);
        }
    }
    
    // 对外的代理方法，下面的h是上面传入的handler
    public final String sayHello(String var1) throws  {
        try {
            return (String)super.h.invoke(this, m3, new Object[]{var1});
        } catch (RuntimeException | Error var3) {
            throw var3;
        } catch (Throwable var4) {
            throw new UndeclaredThrowableException(var4);
        }
    }

    public final String toString() throws  {
        try {
            return (String)super.h.invoke(this, m2, (Object[])null);
        } catch (RuntimeException | Error var2) {
            throw var2;
        } catch (Throwable var3) {
            throw new UndeclaredThrowableException(var3);
        }
    }

    public final int hashCode() throws  {
        try {
            return (Integer)super.h.invoke(this, m0, (Object[])null);
        } catch (RuntimeException | Error var2) {
            throw var2;
        } catch (Throwable var3) {
            throw new UndeclaredThrowableException(var3);
        }
    }

    static {
        try {
            m1 = Class.forName("java.lang.Object").getMethod("equals", Class.forName("java.lang.Object"));
            m3 = Class.forName("com.sankuai.meituan.zcm.depot.proxy.HelloService").getMethod("sayHello", Class.forName("java.lang.String"));
            m2 = Class.forName("java.lang.Object").getMethod("toString");
            m0 = Class.forName("java.lang.Object").getMethod("hashCode");
        } catch (NoSuchMethodException var2) {
            throw new NoSuchMethodError(var2.getMessage());
        } catch (ClassNotFoundException var3) {
            throw new NoClassDefFoundError(var3.getMessage());
        }
    }
}
~~~

#### 怎么拿到动态生成的代理class
~~~ java
import sun.misc.ProxyGenerator;
import java.io.FileOutputStream;
import java.io.IOException;

public class ProxyUtils {
    /**
     * 将根据类信息动态生成的二进制字节码保存到硬盘中，默认的是clazz目录下
     * params: clazz 需要生成动态代理类的类
     * proxyName: 为动态生成的代理类的名称
     */
    public static void generateClassFile(Class clazz, String proxyName) {
        // 根据类信息和提供的代理类名称，生成字节码
        byte[] classFile = ProxyGenerator.generateProxyClass(proxyName, clazz.getInterfaces());
        String paths = clazz.getResource(".").getPath();
        System.out.println(paths);
        FileOutputStream out = null;
        try {
            //保留到硬盘中
            out = new FileOutputStream(paths + proxyName + ".class");
            out.write(classFile);
            out.flush();
        } catch (Exception e) {
            e.printStackTrace();
        } finally {
            try {
                out.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}
~~~

然后运行下面的代码就可以：
~~~ java
// 保存JDK动态代理生成的代理类，类名保存为 UserServiceProxy
ProxyUtils.generateClassFile(userServiceImpl.getClass(), "UserServiceProxy");
~~~