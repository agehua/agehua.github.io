---
layout: post
title:  Android架构师积累
category: accumulation
tags:
  - Android Framework
  - EIT
  - Object-oriented
keywords: EIT, Object-oriented, Framework
banner: http://cdn.conorlee.top/Beach%20at%20Scheveningen%20in%20Calm%20Weather.jpg
thumbnail: http://cdn.conorlee.top/Beach%20at%20Scheveningen%20in%20Calm%20Weather.jpg
toc: true
---

## Android架构师积累 ——By 高焕堂
“-Oriented”的涵意
◎ ”-Oriented”意味着一种信仰 。
◎ ”Object-oriented”相信任何软件都是
由对象所构成的，而且Nothing else 。


”-Driven”的涵意; 例如:Model-driven、Use Case-driven
-- 其实”-driven”是”引导”，而不是大家常说的”驱动”。
-- 就向北极星引导我们，指出方向而已。也像汽车司机(Driver)只是引导汽车方向，并没有去驱动汽车;而是引擎才是驱动汽车。

“-Centered”的涵意; 例如:Architecture-centered、 Architecture-centeric
--一切软件开发的活动都围绕着架构，就像盛诞节的糖果和礼物都挂在圣诞树上一样。


### 卡榫函数
• 所谓「卡榫(Hook)」，就是用来接合两个东西的接口。如果两个东西于不同时间出现，则一方会预留虚空，给予另一边于未来时刻能以实体来填补该空间，两者虚实相依，就密合起来了。设计优良的卡榫，可以让实体易于新陈代谢、抽换自如(Plug and Play, 俗称PnP)。
<!--more-->

• 变与不变的分离(Separate code that changes from the code that doesn’t)是设计卡榫(Hook)函数及应用框架之基本原则和手艺。
• 分离出变(Variant)与不变(Invariant)部份之后，就可以将不变部份写在父类别(Super- class)里，而变的部份就写在子类别 (Subclass)里。

在Java里，使用抽象(abstract)函数或可覆写(overridable)函数来实现卡榫函数。

### IoC机制与Default函数

#### 卡榫函数实现IoC机制
• 控制反转(IoC: Inversion of Control)
• IoC机制源自于OO语言(如C++等)的类别 继承体系，例如C++语言中，基类的函数可以主动调用子类的函数，这就是典型的IoC机制
• 基类与子类之间，主控权是在基类手上， 透过Hook函数来调用子类
• 通常基类是撰写在先，而子类则撰写在后， 这种前辈拥有主导权，进而「控制」后辈 之情形，就通称为「控制反转」。

#### 默认(Default)行为
• 基类的重要功能:提供默认(预设)行为
• 基类可事先定义许多「默认」(Default)函数。这些默认函数可让子类来继承(或调用)之。

~~~ Java
interface IShape {
  void template_paint(Graphics gr);
} // 一般接口
~~~

~~~ Java
// Shape.java
import java.awt.*;
public abstract class Shape implements IShape {
  public void template_paint(Graphics gr){ //默认行为
    invariant_paint(gr); //画背景
    hook_paint(gr); //画前景
  }
  private void invariant_paint(Graphics gr){
    gr.setColor(Color.black); gr.fillRect(10,30, 200,100);
  }

  abstract protected void hook_paint(Graphics gr);
}
~~~

~~~ Java
// Bird.java
import java.awt.*;
public class Bird extends Shape {
  private void hook_paint(Graphics gr){ //控制反转
    gr.setColor(Color.cyan);
    gr.drawArc(30,80,90,110,40,100);
    gr.drawArc(88,93,90,100,40,80);
    gr.setColor(Color.white);
    gr.drawArc(30,55,90,150,35,75);
    gr.drawArc(90,80,90,90,40,80);
  }
}  
~~~

在Android中，子类Activity继承父类Activity，需要重写onCreate()方法，onCreate()方法就是基类主动调用，这也是IOC机制

## 认识EIT造形

> 有了架构设计造形的<简单性>，人们就很容易理解软件的复杂关系，进而提升了掌握软件系统复杂多变的能力，唯有熟谙此道，才能创造架构和产品的<未来性>。

高焕堂老师提出简单的EIT软件造形；则让人们能理解Android多层框架体系里的复杂关系

---

EIT造形是一种基本的结构(Structure)，一种概念(Concept)；我们称它为”EIT造形(Form)”。

![](http://blog.conorlee.top/blogimages/2017/EIT-form.png)


- 强龙做&lt;E&I&gt;，将&lt;T&gt;外包给地头蛇。
- 强龙掌控&lt;E&I&gt;，外包就不会失控。

所以EIT造形支持当今主流的外包模式。

**题目演练：**
由<E>提供一个值N，由&lt;T&gt;通过不同的算法，如1+2+3+ ... + N或1+2^2+ ... + N^2进行计算，最后将计算结果传递给&lt;E&gt;。

> 现在可以试试先想想接口&lt;I&gt;设计:
1.&lt;T&gt;必须有个抽象函数，被&lt;T&gt;反向调用(IoC)到&lt;T&gt;。在调用该函数时，顺便把&lt;E&gt;里的N值传递下去给&lt;T&gt;。
2.由&lt;T&gt;进行计算工作，然后将计算结果传回给&lt;E&gt;。
3.不同的算法就对应不同的&lt;T&gt;类


代码实现如下：
~~~ Java
//Counter.java，这就对应接口<I>
public abstract class Counter {
  public int run(){
      int N = getCount();
      return onCal(N);
  }
  public int getCount() { return 6; }
  protected abstract int onCal(int n);
}
~~~

~~~ Java
//myCounter.java，是<T>类的一种算法
public class myCounter extends Counter{

  @Override
  protected int onCal(int n) {
    int sum = 0;
    for(int i=1; i<=n; i++) {
      sum += i;
    }
    return sum;
  }
}
~~~

~~~ Java
//在<E>类中，调用方式如下
counter = new myCounter();
int sum = counter.run();
~~~

框架(或架构)设计的关键任务就是接口(Interface)设计，这项接口是框架&lt;E&gt;与插件&lt;T&gt;之间的接口，这就是所谓的：框架API。

## IPC

IPC(Inter-Process Communication)通信， 是跨越两个不同进程(Process)之通信

### IPC通信的效率
- 当我们启动某一支应用程序(App)时， Android系统里的Zygote服务孵化(Fork)一个新进程(Process)给它，然后将它(该App)加载到这个新诞生的进程里。
- 基于Linux的安全限制，以及进程的基本特性(例如，不同进程的地址空间是独立的)，如果两个类(或其对象)在同一个进程里执行时，两者沟通方便也快速 。
- 但是，当它们分别在不同的进程里执行时，两者沟通就属于IPC跨进程沟通了，不如前者方便，也慢些

- 一个进程是一个独立的执行空间，不会被正在其它进程里的程序所侵犯。这种保护方法是Android的重要安全机制。于是，得先认识进程的内涵，才能进一步了解跨进 程IPC机制。
- 在Android的进程里，有一个虚拟机(Virtual Machine，简称VM)的对象，可执行Java代码，也引导JNI本地程序的执行，实现Java与C/C++之间的沟通。如下图:

![每一进程有:一个VM对象、主线程、MQ和Looper](http://blog.conorlee.top/blogimages/2017/process-ipc-structure.png)

> 不同进程的地址空间是独立的

每一个进程在诞生时，都会诞生一个主线程(Main Thread)，以及诞生一个Looper类的对象和一个MQ(Message Queue)数据结构。每当主线程作完事情，就会去执行Looper类。

主线程最主要的工作就是处理UI画面的事件(Event)，每当UI事件发生时，Android框架会丢信息(Message)到MQ里。主线程看到MQ有新的信息时，就取出信息，然后依据信息内容而去执行特定的函数。执行完毕，就再继续执行Looper类，不断地观察MQ的动态。

### IPC的IBinder接口 -- 定义与实现
大家都知道，当两个类都在同一个进程里执行时，两者之间的沟通，只要采取一般的函数调用(Function Call)就行了，既快速又方便。一旦两个类分别在不同的进程里执行时，两者之间的沟通，就不能采取一般的函数调用途径了。只好采取IPC沟通途径。

Android框架的IPC沟通仰赖单一的IBinder接口。此时Client端调用IBinder接口的transact()函数，透过IPC机制而调用到远方(Remote)的onTransact()函数。

Java层的IBinder 接口是定义于IBinder.java代码文档里
~~~ Java
// IBinder.java
// .......
public interface IBinder {
// ........
public boolean transact(int code, Parcel data, Parcel reply, int flags)
  throws RemoteException; // ...........
}
~~~

IBinder接口定义了一些函数，可以让Client程序可以进行跨进程的调用(当然也能支持同进程的短程调用)。其中，最主要的一个函数就是: transact()函数

在Android的框架里，由Binder基类实现IBinder接口。

![java层的Binder基类定义](http://blog.conorlee.top/blogimages/2017/ibinder-binder.png)

Binder基类的很重要目的是支持跨进程调 用Service，也就是让远程的Client可以跨 进程调用某个Service。Binder基类定义于Binder.java文件里:
~~~ Java
// Binder.java
// .......
public class Binder implements IBinder {
  // ..........
  private int mObject;
  public Binder() {
    init();
  // ...........
  }
  //用来实现IBinder的transact()函数接口
  public final boolean transact(int code, Parcel data, Parcel reply, int flags)
    throws RemoteException {
    // ................
    boolean r = onTransact(code, data, reply, flags); return r;
  }

  //其角色与transact()函数是相同的，只是这是用来让C/C++本地程序来调用的。
  private boolean execTransact(int code, int dataObj, int replyObj, int flags) {
    Parcel data = Parcel.obtain(dataObj); Parcel reply = Parcel.obtain(replyObj);
    boolean res;
    res = onTransact(code, data, reply, flags);
    // ............
    return res;
  }

  //这是一个抽象函数，让应用子类来覆写(Override)的.
  //上述的transact()和 execTransact()两者都是调用onTransact()函数来实现反向调用(IoC, Inversion of Control)的。
  protected boolean onTransact(int code, Parcel data, Parcel reply, int flags)
    throws RemoteException {
  }

  //这是一个本地(Native)函数，让JNI模块来实现这个函数.
  //Binder()构造函数会调用这个init()本地函数
  private native final void init();
}
~~~

Binder就是EIT造形里的&lt;E&gt;

这个IBinder接口是Binder(即&lt;E&gt;)提供给Client的接口，简称为&lt;CI&gt;
![](http://blog.conorlee.top/blogimages/2017/binder-EIT.png)

onTransact()就是EIT造形里的&lt;I&gt;，是支持<基类/子类>之间IoC调用的接口
![](http://blog.conorlee.top/blogimages/2017/binder-EIT-onTransact.png)


示例：有一个Activity类别，它想跨进程去调用MediaPlayer播放引擎，以便播放MP3音乐.

类结构图如下：
![](http://blog.conorlee.top/blogimages/2017/binder-example-media.png)

在上图里，从myActivity到IBinder接口，画上了虚线箭头，表示那是抽象概念的。实际上，myActivity并没有直接调用Java层的IBinder接口，而是绕到底层C/C++和Binder驱动而间接调用到Binder基类的execTransact()函数，转而调用myBinder的onTransact()函数。如下图:

![](http://blog.conorlee.top/blogimages/2017/binder-example-media-detail.png)

### IPC通信的三步骤
还是用上面的MediaPlayer例子，其IPC通信的三个步骤是:
- **Step-1.** Activity使用startService()函數來启动Service。
- **Step-2.** Activity调用bindService()来绑定Service。亦即，Activity建立与Service之间的连结(Connection)。
- **Step-3.** Activity调用IBinder接口的transact() 函数，透过底层Binder Driver驱动而间接调用到Binder基类的execTransact()函数， 转而调用 myBinder的onTransact()函数。
