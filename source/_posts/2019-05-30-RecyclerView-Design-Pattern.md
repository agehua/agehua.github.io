---
layout: post
title: RecyclerView 中的设计模式
category: accumulation
tags:
    - RecyclerView
keywords: RecyclerView, Design Pattern
banner: http://cdn.conorlee.top/Farmhouse%20in%20Provence.jpg
thumbnail: http://cdn.conorlee.top/Farmhouse%20in%20Provence.jpg
toc: true
---

`本篇文章是在掘金上偶然看到，介绍的比较全面，收藏到本博客`

Recyclerview 在 Android 应用开发中使用非常频繁，但一般人只知道onCreateViewHolder、onBindViewHolder等简单使用。了解这篇文章无论是对项目中实际开发还是Android源码设计，都会有借鉴和帮助

### Recyclerview 源码中用到的设计模式
本文通过桥接,组合,适配器,观察者模式四种设计模式来解读RecyclerView。
<!--more-->
- 1. 通过**桥接模式**，使RecyclerView 将布局方式独立成LayoutManager，实现对布局的定制化。
- 2. 通过**组合模式**，使RecycleView通过dispatchLayout对Item View进行布局绘制的。
- 3. 通过**适配器模式**，ViewHolder将RecycleView与ItemView联系起来，使得RecycleView方便操作ItemView。
- 4. 通过**观察者模式**，给ViewHolder注册观察者，当调用notifyDataSetChanged时，就能重新绘制。

![RecyclerView设计模式草图](/images/blogimages/2019/recyclerview_design_pattern.png)

### 设计思路
RecyclerView官网给出的定义是: A flexible view for providing a limited window into a large data set. ，也就是在限定的视图内展示大量的数据，来一张通俗明了的图：

![](/images/blogimages/2019/recyclerview_sketch_1.png)

RecyclerView的职责就是将Datas中的数据以一定的规则展示在它的上面，但说破天RecyclerView只是一个ViewGroup，它只认识View，不清楚Data数据的具体结构，所以两个陌生人之间想构建通话，我们很容易想到适配器模式,因此，RecyclerView需要一个**Adapter(适配器模式)**来与Datas进行交流：

![](/images/blogimages/2019/recyclerview_sketch_2.png)

如上如所示，RecyclerView表示只会和ViewHolder进行接触，而Adapter的工作就是将Data转换为RecyclerView认识的ViewHolder，因此RecyclerView就间接地认识了Datas。

事情虽然进展愉快，但RecyclerView是个很懒惰的人，尽管Adapter已经将Datas转换为RecyclerView所熟知的View，但RecyclerView并不想自己管理些子View，因此，它雇佣了一个叫做**LayoutManager**的大祭司来帮其完成布局(桥接模式)，现在，图示变成下面这样：

![](/images/blogimages/2019/recyclerview_sketch_3.png)

如上图所示，LayoutManager协助RecyclerView来完成布局。但LayoutManager这个大祭司也有弱点，就是它只知道如何将一个一个的View布局在RecyclerView上，但它并不懂得如何管理这些View，如果大祭司肆无忌惮的玩弄View的话肯定会出事情，所以，必须有个管理View的护法，它就是**Recycler**，LayoutManager在需要View的时候回向护法进行索取，当LayoutManager不需要View(试图滑出)的时候，就直接将废弃的View丢给Recycler，图示如下：

![](/images/blogimages/2019/recyclerview_sketch_4.png)

到了这里，有负责翻译数据的Adapter，有负责布局的LayoutManager，有负责管理View的Recycler，一切都很完美，但RecyclerView乃何等神也，它下令说当子View变动的时候姿态要优雅(动画)，所以用雇佣了一个舞者**ItemAnimator(观察者模式)**，因此，舞者也进入了这个图示:

![](/images/blogimages/2019/recyclerview_sketch_5.png)

如上，我们就是从宏观层面来对RecylerView有个大致的了解，可以看到，RecyclerView作为一个View，它只负责接受用户的各种讯息，然后将信息各司其职的分发出去。接下来我们将深入源码，详细讲解用到的设计模式，看看RecyclerView都是怎么来操作各个组件工作的。

### 桥接模式详解

#### 模式的定义
将抽象部分与实现部分分离，使它们都可以独立的变化。

#### 模式的使用场景
如果一个系统需要在构件的抽象化角色和具体化角色之间添加更多的灵活性，避免在两个层次之间建立静态的联系。设计要求实现化角色的任何改变不应当影响客户端，或者实现化角色的改变对客户端是完全透明的。需要跨越多个平台的图形和窗口系统上。一个类存在两个独立变化的维度，且两个维度都需要进行扩展。

#### UML类图
![桥接模式UML类图](/images/blogimages/2019/design_pattern_bridge_uml.png)

#### 角色介绍
抽象化(Abstraction)角色：抽象化给出的定义，并保存一个对实现化对象的引用。 修正抽象化(Refined Abstraction)角色：扩展抽象化角色，改变和修正父类对抽象化的定义。实现化(Implementor)角色：这个角色给出实现化角色的接口，但不给出具体的实现。必须指出的是，这个接 口不一定和抽象化角色的接口定义相同，实际上，这两个接口可以非常不一样。实现化角色应当只给出底层操作，而抽象化角色应当只给出基于底层操作的更高一层的操作。具体实现化(ConcreteImplementor)角色：这个角色给出实现化角色接口的具体实现。

#### 模式的简单实现

其实Java的虚拟机就是一个很好的例子，在不同平台平台上，用不同的虚拟机进行实现，这样只需把Java程序编译成符合虚拟机规范的文件，且只用编译一次，便在不同平台上都能工作。 但是这样说比较抽象，用一个简单的例子来实现bridge模式。

编写一个程序，使用两个绘图的程序的其中一个来绘制矩形或者原型，同时，在实例化矩形的时候，它要知道使用绘图程序1（DP1）还是绘图程序2（DP2）。

(ps:假设dp1和dp2的绘制方式不一样，它们是用不同方式进行绘制，示例代码，不讨论过多细节)

##### 实现源码
首先是两个绘图程序dp1,dp2
~~~ Java
//具体的绘图程序类dp1
public class DP1 {
    public void draw_1_Rantanle(){
        System.out.println("使用DP1的程序画矩形");
    }
    public void draw_1_Circle(){
        System.out.println("使用DP1的程序画圆形");
    }
}
//具体的绘图程序类dp2
public class DP2 {
    public void drawRantanle(){
        System.out.println("使用DP2的程序画矩形");
    }
    public void drawCircle(){
        System.out.println("使用DP2的程序画圆形");
    }
}
~~~

接着​抽象的形状Shape和两个派生类：矩形Rantanle和圆形Circle
~~~ Java
//抽象化角色Abstraction
abstract class Shape {
    //持有实现的角色Implementor(作图类)
    protected Drawing myDrawing;
    public Shape(Drawing drawing) {
        this.myDrawing = drawing;
    }
    abstract public void draw();
    //保护方法drawRectangle
    protected void drawRectangle(){
       //this.impl.implmentation()
        myDrawing.drawRantangle();
    }
    //保护方法drawCircle
    protected void drawCircle(){
        //this.impl.implmentation()
        myDrawing.drawCircle();
    }
}
//修正抽象化角色Refined Abstraction(矩形)
public class Rantangle extends Shape{
    public Rantangle(Drawing drawing) {
        super(drawing);
    }
    @Override
    public void draw() {
        drawRectangle();
    }
}
//修正抽象化角色Refined Abstraction(圆形)
public class Circle extends Shape {
    public Circle(Drawing drawing) {
        super(drawing);
    }
    @Override
    public void draw() {
        drawCircle();
    }
}
~~~
最后，我们的实现绘图的Drawing和分别实现dp1的V1Drawing和dp2的V2Drawing
~~~ Java
//实现化角色Implementor
//implmentation两个方法，画圆和画矩形
public interface Drawing {
    public void drawRantangle();
    public void drawCircle();
}
//具体实现化逻辑ConcreteImplementor
//实现了接口方法，使用DP1进行绘图
public class V1Drawing implements Drawing{
    DP1 dp1;
    public V1Drawing() {
        dp1 = new DP1();
    }
    @Override
    public void drawRantangle() {
        dp1.draw_1_Rantanle();
    }
    @Override
    public void drawCircle() {
        dp1.draw_1_Circle();
    }           
}
//具体实现化逻辑ConcreteImplementor
//实现了接口方法，使用DP2进行绘图
public class V2Drawing implements Drawing{
    DP2 dp2;
    public V2Drawing() {
        dp2 = new DP2();
    }
    @Override
    public void drawRantangle() {
        dp2.drawRantanle();
    }
    @Override
    public void drawCircle() {
        dp2.drawCircle();
    }
}
~~~

在这个示例中，图形Shape类有两种类型，圆形和矩形，为了使用不同的绘图程序绘制图形，把实现的部分进行了分离，构成了Drawing类层次结构，包括V1Drawing和V2Drawing。在具体实现类中，V1Drawing控制着DP1程序进行绘图，V2Drawing控制着DP2程序进行绘图，以及保护的方法drawRantangle,drawCircle(Shape类中) 。

### 组合模式详解

#### 模式的定义
组合模式(Composite Pattern)又叫作部分-整体模式，它使我们树型结构的问题中，模糊了简单元素和复杂元素的概念，客户程序可以像处理简单元素一样来处理复杂元素,从而使得客户程序与复杂元素的内部结构解耦。 GoF在《设计模式》一书中这样定义组合模式：将对象组合成树形结构以表示“部分-整体”的层次结构。使得用户对单个对象和组合对象的使用具有一致性。

#### 模式的使用场景
表示对象的部分-整体层次结构。从一个整体中能够独立出部分模块或功能的场景。

#### UML类图

![桥接模式UML类图](/images/blogimages/2019/design_pattern_composite.png)

#### 角色分析
Component抽象构件角色 ：定义参加组合对象的共有方法和属性，可以定义一些默认的行为或属性。Leaf叶子构件 : 叶子对象，其下再也没有其他的分支。Composite树枝构件 ：树枝对象，它的作用是组合树枝节点和叶子节点形成一个树形结构。

#### 该模式的实现实例

抽象构件 Component.java：
~~~ Java
public abstract class Component {
   //个体和整体都具有的共享
  public void doSomething(){
   //业务逻辑
  }
}
~~~
树枝构件 Composite.java
~~~ Java
public class Composite extends Component {
   //构件容器
   private ArrayList componentArrayList = new ArrayList();
   //增加一个叶子构件或树枝构件
   public void add(Component component){
      this.componentArrayList.add(component);
   }
   //删除一个叶子构件或树枝构件
   public void remove(Component component){
      this.componentArrayList.remove(component);
   }
   //获得分支下的所有叶子构件和树枝构件
   public ArrayList getChildren(){
     return this.componentArrayList;
   }
 }
~~~
树叶构件 Leaf.java
~~~ Java
public class Leaf extends Component {
   //可以覆写父类方法
   public void doSomething(){    
   }
}
~~~

场景类 Client.java
~~~ Java
public class Client {
    public static void main(String[] args) {
       //创建一个根节点
      Composite root = new Composite();
      root.doSomething();
      //创建一个树枝构件
      Composite branch = new Composite();
      //创建一个叶子节点
      Leaf leaf = new Leaf();
      //建立整体
      root.add(branch);
      branch.add(leaf);
   }
   //通过递归遍历树
   public static void display(Composite root){
      for(Component c:root.getChildren()){
         if(c instanceof Leaf){ //叶子节点
             c.doSomething();
         }else{ //树枝节点
             display((Composite)c);
            }
        }
   }
}
~~~

### 适配器模式详解
7种结构型模式：适配器模式、装饰模式、代理模式、外观模式、桥接模式、组合模式、享元模式。其中对象的适配器模式是各种模式的起源，我们看下面的图：

![适配器模式](/images/blogimages/2019/design_pattern_adapter.PNG)

适配器模式将某个类的接口转换成客户端期望的另一个接口表示，目的是消除由于接口不匹配所造成的类的兼容性问题。主要分为三类：类的适配器模式、对象的适配器模式、接口的适配器模式。首先，我们来看看类的适配器模式，先看类图：

#### 类的适配器模式
![类的适配器模式](/images/blogimages/2019/design_pattern_adapter_class.PNG)

核心思想就是：有一个Source类，拥有一个方法，待适配，目标接口时Targetable，通过Adapter类，将Source的功能扩展到Targetable里，看代码：
~~~ Java
public class Source {  
    public void method1() {  
        System.out.println("this is original method!");  
    }  
}  
public interface Targetable {  
    /* 与原类中的方法相同 */  
    public void method1();  
    /* 新类的方法 */  
    public void method2();  
}  
public class Adapter extends Source implements Targetable {  
    @Override  
    public void method2() {  
        System.out.println("this is the targetable method!");  
    }  
}  
~~~
Adapter类继承Source类，实现Targetable接口，下面是测试类：
~~~ Java
public class AdapterTest {  
    public static void main(String[] args) {  
        Targetable target = new Adapter();  
        target.method1();  
        target.method2();  
    }  
}
~~~
#### 对象的适配器模式
基本思路和类的适配器模式相同，只是将Adapter类作修改，这次不继承Source类，而是持有Source类的实例，以达到解决兼容性的问题。看图：

![对象的适配器模式](/images/blogimages/2019/design_pattern_adapter_object.PNG)

只需要修改Adapter类的源码即可：
~~~ Java
public class Wrapper implements Targetable {  
    private Source source;  
    public Wrapper(Source source){  
        super();  
        this.source = source;  
    }  
    @Override  
    public void method2() {  
        System.out.println("this is the targetable method!");  
    }  
    @Override  
    public void method1() {  
        source.method1();  
    }  
}
~~~

测试类：
~~~ Java
public class AdapterTest {  
    public static void main(String[] args) {  
        Source source = new Source();  
        Targetable target = new Wrapper(source);  
        target.method1();  
        target.method2();  
    }  
}
~~~
#### 接口的适配器模式
接口的适配器是这样的：有时我们写的一个接口中有多个抽象方法，当我们写该接口的实现类时，必须实现该接口的所有方法，这明显有时比较浪费，因为并不是所有的方法都是我们需要的，有时只需要某一些，此处为了解决这个问题，我们引入了接口的适配器模式，借助于一个抽象类，该抽象类实现了该接口，实现了所有的方法，而我们不和原始的接口打交道，只和该抽象类取得联系，所以我们写一个类，继承该抽象类，重写我们需要的方法就行。看一下类图：

![对象的适配器模式](/images/blogimages/2019/design_pattern_adapter_interface.PNG)

这个很好理解，在实际开发中，我们也常会遇到这种接口中定义了太多的方法，以致于有时我们在一些实现类中并不是都需要。看代码：
~~~ Java
public interface Sourceable {  
    public void method1();  
    public void method2();  
}  
~~~
抽象类Wrapper2：
~~~ Java
public abstract class Wrapper2 implements Sourceable{  
    public void method1(){}  
    public void method2(){}  
}  
public class SourceSub1 extends Wrapper2 {  
    public void method1(){  
        System.out.println("the sourceable interface's first Sub1!");  
    }  
}  
public class SourceSub2 extends Wrapper2 {  
    public void method2(){  
        System.out.println("the sourceable interface's second Sub2!");  
    }  
}  
public class WrapperTest {  
    public static void main(String[] args) {  
        Sourceable source1 = new SourceSub1();  
        Sourceable source2 = new SourceSub2();  
        source1.method1();  
        source1.method2();  
        source2.method1();  
        source2.method2();  
    }  
}
~~~

> 讲了这么多，总结一下三种适配器模式的应用场景：
- 类的适配器模式：当希望将一个类转换成满足另一个新接口的类时，可以使用类的适配器模式，创建一个新类，继承原有的类，实现新的接口即可。
- 对象的适配器模式：当希望将一个对象转换成满足另一个新接口的对象时，可以创建一个Wrapper类，持有原类的一个实例，在Wrapper类的方法中，调用实例的方法就行。
- 接口的适配器模式：当不希望实现一个接口中所有的方法时，可以创建一个抽象类Wrapper，实现所有方法，我们写别的类的时候，继承抽象类即可。

### 观察者模式详解
观察者模式（Observer）是类和类之间的关系，不涉及到继承，学的时候应该记得归纳。观察者模式很好理解，类似于邮件订阅和RSS订阅，当我们浏览一些博客或wiki时，经常会看到RSS图标，就这的意思是，当你订阅了该文章，如果后续有更新，会及时通知你。其实，简单来讲就一句话：当一个对象变化时，其它依赖该对象的对象都会收到通知，并且随着变化！对象之间是一种一对多的关系。先来看看关系图：

![对象的适配器模式](/images/blogimages/2019/design_pattern_observers.PNG)

我解释下这些类的作用：MySubject类就是我们的主对象，Observer1和Observer2是依赖于MySubject的对象，当MySubject变化时，Observer1和Observer2必然变化。AbstractSubject类中定义着需要监控的对象列表，可以对其进行修改：增加或删除被监控对象，且当MySubject变化时，负责通知在列表内存在的对象。我们看实现代码：

一个Observer接口：
~~~ Java
public interface Observer {  
    public void update();  
}  
~~~
两个实现类：
~~~ Java
public class Observer1 implements Observer {  
    @Override  
    public void update() {  
        System.out.println("observer1 has received!");  
    }  
}  
public class Observer2 implements Observer {  
    @Override  
    public void update() {  
        System.out.println("observer2 has received!");  
    }  
}  
~~~
Subject接口及实现类：
~~~ Java
public interface Subject {  
    /*增加观察者*/  
    public void add(Observer observer);  
    /*删除观察者*/  
    public void del(Observer observer);  
    /*通知所有的观察者*/  
    public void notifyObservers();      
    /*自身的操作*/  
    public void operation();  
}  
public abstract class AbstractSubject implements Subject {  
    private Vector vector = new Vector();  
    @Override  
    public void add(Observer observer) {  
        vector.add(observer);  
    }  
    @Override  
    public void del(Observer observer) {  
        vector.remove(observer);  
    }  
    @Override  
    public void notifyObservers() {  
        Enumeration enumo = vector.elements();  
        while(enumo.hasMoreElements()){  
            enumo.nextElement().update();  
        }  
    }  
}  
public class MySubject extends AbstractSubject {  
    @Override  
    public void operation() {  
        System.out.println("update self!");  
        notifyObservers();  
    }  
}
~~~

测试类：
~~~ Java
public class ObserverTest {  
    public static void main(String[] args) {  
        Subject sub = new MySubject();  
        sub.add(new Observer1());  
        sub.add(new Observer2());     
        sub.operation();  
    }  
}
~~~
这些东西，其实不难，只是有些抽象，不太容易整体理解，还有就是门面模式，适配器模式，桥接模式有些同学觉得都差不多，其实大不一样，但都是结构型模式。

- Facade（门面模式）出现较多是这样的情况,你有一个复杂的系统,对应了各种情况,客户看了说功能不错,但是使用太麻烦.你说没问题,我给你提供一个统一的门面.所以Facade使用的场合多是对系统的”优化”.

- Adapter出现是这样的情况,你有一个类提供接口A,但是你的客户需要一个实现接口B的类,这个时候你可以写一个Adapter让把A接口变成B接口,所以Adapter使用的场合是指鹿为马.就是你受夹板气的时候,一边告诉你我只能提供给你A(鹿),一边告诉你说我只要B(马),他长了四条腿,你没办法了,把鹿牵过去说,这是马,你看他有四条腿.(当然实现指鹿为马也有两种方法,一个方法是你只露出鹿的四条腿,说你看这是马,这种方式就是封装方式的Adapter实现,另一种方式是你把鹿牵过去,但是首先介绍给他说这是马,因为他长了四条腿这种是继承的方式.)

- Bridge在一般的开发中出现的情况并不多,AWT是一个,SWT也算部分是,如果你的客户要求你开发一个系统,这个系统在Windows下运行界面的样子是Windows的样子.在Linux下运行是Linux下的样子.在Macintosh下运行要是Mac Os的样子.怎么办? 定义一系列的控件Button,Text,radio,checkBox等等.供上层开发者使用,他们使用这些控件的方法,利用这些控件构造一个系统的GUI,然后你为这些控件写好Linux的实现,让它在Linux上调用Linux本地的对应控件,在Windows上调用Windows本地的对应控件,在Macintosh上调用Macintosh本地的对应控件ok,你的任务完成了.

篇幅有限本文只介绍了Recyclerview源码中应用到的几种设计模式，下一篇文章会继续介绍Recyclerview源码



### Ref
