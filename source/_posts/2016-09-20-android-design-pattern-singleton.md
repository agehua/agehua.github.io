---
layout: post
title:  ANDROID设计模式之单例模式
category: accumulation
tags:
  - ANDROID
  - Singleton
keywords: 单例模式, ANDROID
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Blossoming%20Almond%20Tree.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Blossoming%20Almond%20Tree.jpg
toc: true
---


本文转载自[stormzhang ANDROID设计模式之单例模式 ](http://stormzhang.com/designpattern/2016/03/27/android-design-pattern-singleton/)，内容稍有补充

### 常用单例模式

什么是设计模式？其实简单的理解就是前人留下来的一些经验总结而已，然后把这些经验起了个名字叫Design Pattern，翻译过来就是设计模式的意思，通过使用设计模式可以让我们的代码复用性更高，可维护性更高，让你的代码写的更优雅。设计模式理论上有23种，但是我只会针对Android平台上常用的一些设计模式做分享，今天就先来分享下最常用的单例模式。

<!--more-->
#### 饿汉式

~~~ Java
public class Singleton{

    private static Singleton instance = new Singleton();

    private Singleton(){}

    public static Singleton newInstance(){
        return instance;
    }
}
~~~

饿汉式 是最简单的实现方式，这种实现方式适合那些在初始化时就要用到单例的情况，这种方式简单粗暴，如果单例对象初始化非常快，而且占用内存非常小的时候这种方式是比较合适的，可以直接在应用启动时加载并初始化。 但是，如果单例初始化的操作耗时比较长而应用对于启动速度又有要求，或者单例的占用内存比较大，再或者单例只是在某个特定场景的情况下才会被使用，而一般情况下是不会使用时，使用饿汉式的单例模式就是不合适的，这时候就需要用到懒汉式的方式去按需延迟加载单例。

#### 懒汉式

~~~ Java
public class Singleton{
    private static Singleton instance = null;

    private Singleton(){}

    public static Singleton newInstance(){
        if(null == instance){
            instance = new Singleton();
        }
        return instance;
    }
}
~~~

懒汉式与饿汉式的最大区别就是将单例的初始化操作，延迟到需要的时候才进行，这样做在某些场合中有很大用处。比如某个单例用的次数不是很多，但是这个单例提供的功能又非常复杂，而且加载和初始化要消耗大量的资源，这个时候使用懒汉式就是非常不错的选择。

### 多线程下的单例模式

上面介绍了一些单例模式的基本应用方法，但是上面所说的那些使用方式都是有一个隐含的前提，那就是他们都是应用在单线程条件下，一旦换成了多线程就有出错的风险。

如果在多线程的情况下，饿汉式不会出现问题，因为JVM只会加载一次单例类，但是懒汉式可能就会出现重复创建单例对象的问题。为什么会有这样的问题呢？因为懒汉式在创建单例时是 线程不安全的，多个线程可能会并发调用他的newInstance方法导致多个线程可能会创建多份相同的单例出来。

那有没有办法，使懒汉式的单利模式也是线程安全的呢？答案肯定是有的，就是使用加同步锁的方式去实现。

#### 懒汉式同步锁

~~~ Java
public class Singleton {

    private static Singleton instance = null;

    private Singleton(){
    }

    public static Singleton getInstance() {
        synchronized (Singleton.class) {
            if (instance == null) {
                instance = new Singleton();
            }
        }

        return instance;
    }
}
~~~

这种是最常见的解决同步问题的一种方式，使用同步锁synchronized (Singleton.class)防止多线程同时进入造成instance被多次实例化。举个在Android使用这种方式的例子：

#### InputMethodManager示例

~~~ Java
public final class InputMethodManager {
    //内部全局唯一实例  
    static InputMethodManager sInstance;

    //对外api  
    public static InputMethodManager getInstance() {
        synchronized (InputMethodManager.class) {
            if (sInstance == null) {
                IBinder b = ServiceManager.getService(Context.INPUT_METHOD_SERVICE);
                IInputMethodManager service = IInputMethodManager.Stub.asInterface(b);
                sInstance = new InputMethodManager(service, Looper.getMainLooper());
            }
            return sInstance;
        }
    }
}
~~~

以上是Android源码中输入法类相关的单例使用方式。

- 加锁的懒汉模式看起来即解决了线程并发问题，又实现了延迟加载，然而它存在着性能问题，依然不够完美。
- synchronized修饰的同步方法比一般方法要慢很多，如果多次调用getInstance()，累积的性能损耗就比较大了。

因此就有了双重校验锁，先看下它的实现代码。

#### 双重校验锁

~~~ Java
public class Singleton {

    private static volatile Singleton instance = null;

    private Singleton(){
    }

    public static Singleton getInstance() {
        // if already inited, no need to get lock everytime
        if (instance == null) {
            synchronized (Singleton.class) {
                if (instance == null) {
                    instance = new Singleton();
                }
            }
        }

        return instance;
    }
}
~~~

可以看到上面在synchronized (Singleton.class)外又添加了一层if，这是为了在instance已经实例化后下次进入不必执行synchronized (Singleton.class)获取对象锁，从而提高性能。

> 我们看到双重校验锁即实现了延迟加载，又解决了线程并发问题，同时还解决了执行效率问题，是否真的就万无一失了呢？

- 这里要提到**Java中的指令重排优化**。所谓指令重排优化是指在不改变原语义的情况下，通过调整指令的执行顺序让程序运行的更快。
- JVM中并没有规定编译器优化相关的内容，也就是说JVM可以自由的进行指令重排序的优化。
- 这个问题的关键就在于**由于指令重排优化的存在，导致初始化Singleton和将对象地址赋给instance字段的顺序是不确定的**。
- 在某个线程创建单例对象时，在构造方法被调用之前，就为该对象分配了内存空间并将对象的字段设置为默认值。
- 此时就可以将分配的内存地址赋值给instance字段了，然而该对象可能还没有初始化。若紧接着另外一个线程来调用getInstance，取到的就是状态不正确的对象，程序就会出错。
 
#### volatile关键字
JDK5的修正：以上就是双重校验锁会失效的原因，不过还好在JDK1.5及之后版本增加了volatile关键字。

**volatile的一个语义是禁止指令重排序优化**，也就保证了instance变量被赋值的时候对象已经是初始化过的，从而避免了上面说到的问题。
Java中的volatile 变量是什么？
理解volatile关键字的作用的前提是要理解Java内存模型，volatile关键字的作用主要有两个：
 
- 多线程主要围绕可见性和原子性两个特性而展开，使用volatile关键字修饰的变量，保证了其在多线程之间的可见性，
即每次读取到volatile变量，一定是最新的数据
- 代码底层执行不像我们看到的高级语言—-Java程序这么简单，
    - 它的执行是Java代码–>字节码–>根据字节码执行对应的C/C++代码–>C/C++代码被编译成汇编语言–>和硬件电路交互，
    - 现实中，为了获取更好的性能JVM可能会对指令进行重排序，多线程下可能会出现一些意想不到的问题。
    - 使用volatile则会对禁止语义重排序，当然这也一定程度上降低了代码执行效率
- 从实践角度而言，volatile的一个重要作用就是和CAS结合，保证了原子性，
    - 详细的可以参见java.util.concurrent.atomic包下的类，比如AtomicInteger。
    - CAS（Compare and swap）比较和替换是设计并发算法时用到的一种技术。
    - 简单来说，比较和替换是使用一个期望值和一个变量的当前值进行比较，如果当前变量的值与我们期望的值相等，就使用一个新值替换当前变量的值。
- volatile是一个特殊的修饰符，只有成员变量才能使用它。
    - 在Java并发程序缺少同步类的情况下，多线程对成员变量的操作对其它线程是透明的。
    - volatile变量可以保证下一个读取操作会在前一个写操作之后发生。

来源： http://blog.csdn.net/fly910905/article/details/79283557

代码如下：
~~~ Java
public class Singleton {
    private static volatile Singleton instance = null;
    private Singleton(){}
    public static Singleton getInstance() {
        if (instance == null) { // Single Checked
            synchronized (Singleton.class) {
                if (instance == null) { // Double checked
                    instance = new Singleton();
                }
 
            }
        }
        return instance;
    }
}
~~~

以上三种方式还是挺麻烦的，我们不禁要问，有没有更好的实现方式呢？答案是肯定的。 我们可以利用JVM的类加载机制去实现。在很多情况下JVM已经为我们提供了同步控制，比如：

在static{}区块中初始化的数据
访问final字段时
等等
因为在JVM进行类加载的时候他会保证数据是同步的，我们可以这样实现：

采用内部类，在这个内部类里面去创建对象实例。这样的话，只要应用中不使用内部类 JVM 就不会去加载这个单例类，也就不会创建单例对象，从而实现懒汉式的延迟加载和线程安全。

实现代码如下：

#### 静态内部类

~~~ Java
public class Singleton{
    //内部类，在装载该内部类时才会去创建单利对象
    private static class SingletonHolder{
        public static Singleton instance = new Singleton();
    }

    private Singleton(){}

    public static Singleton newInstance(){
        return SingletonHolder.instance;
    }

    public void doSomething(){
        //do something
    }
}
~~~

这样实现出来的单例类就是线程安全的，而且使用起来很简洁，麻麻再也不用担心我的单例不是单例了。

然而这还不是最简单的方式，Effective Java中推荐了一种更简洁方便的使用方式，就是使用枚举。

#### 枚举类型单例模式

~~~ Java
public enum Singleton{
    //定义一个枚举的元素，它就是Singleton的一个实例
    instance;

    public void doSomething(){
        // do something ...
    }    
}
~~~

使用方法如下：

~~~ Java
public static void main(String[] args){
   Singleton singleton = Singleton.instance;
   singleton.doSomething();
}
~~~

默认枚举实例的创建是线程安全的(创建枚举类的单例在JVM层面也是能保证线程安全的), 所以不需要担心线程安全的问题，所以理论上枚举类来实现单例模式是最简单的方式。

### 总结

一般单例模式包含了5种写法，分别是饿汉、懒汉、双重校验锁、静态内部类和枚举。相信看完之后你对单例模式有了充分的理解了，根据不同的场景选择最你最喜欢的一种单例模式吧！


关于synchronized关键字，推荐学习这篇文章：[Java基础笔记 – 线程同步问题 解决同步问题的方法 synchronized方法 同步代码块](http://www.itzhai.com/java-based-notebook-thread-synchronization-problem-solving-synchronization-problems-synchronized-block-synchronized-methods.html)


使用synchronized关键字，该关键字修饰的方法叫做同步方法。

Java中每个对象都有一个锁或者称为监视器，当访问某个对象的synchronized方法时，表示将**该对象上锁**，而不仅仅是为该方法上锁。

这样如果一个对象的synchronized方法被某个线程执行时，其他线程无法访问该对象的任何synchronized方法（`但是可以调用其他非synchronized的方法`）。直至该synchronized方法执行完。

静态的synchronized方法调用情况：
当调用一个对象的静态synchronized方法时，它锁定的并不是synchronized方法所在的对象，而是synchronized方法所在对象对应的**Class对象**。这样，其他线程就不能调用**该类的其他静态synchronized方法**了，但是可以调用非静态的synchronized方法。

结论：**执行静态synchronized方法锁方法所在对象，执行非静态synchronized方法锁方法所在对象对应的Class对象。**

使用synchronized创建同步代码块：
通过使用synchronized同步代码块，锁定一个**对象**，该对象作为可执行的标志从而达到同步的效果。

synchronized方法和synchronized同步代码块的区别：
synchronized同步代码块只是锁定了该代码块，代码块外面的代码还是可以被访问的。
synchronized方法是粗粒度的并发控制，某一个时刻只能有一个线程执行该synchronized方法。
synchronized同步代码块是细粒度的并发控制，只会将块中的代码同步，代码块之外的代码可以被其他线程同时访问。

参考： [Java单例模式的不同写法（懒汉式、饿汉式、双检锁、静态内部类、枚举）](https://blog.csdn.net/fly910905/article/details/79286680)
关于设计模式，CSDN上也有一个总结: [Android源码设计模式分析一期发布](https://blog.csdn.net/bboyfeiyu/article/details/44563871)