binder跨进程通讯，
本地进程 Local-side process
远端进程 remote-side process

LSP 通过asInterface拿到RSP实例，通过RSP实例调用transact方法传递数据（数据要实现Parcelable接口）
RSP通过重写onTransact方法，拿到数据，作进一步处理


![Activity View层级](/images/blogimages/2020/activiy_window_layers.webp)
一个Activity对应一个PhoneWindow，一个PhoneWindow对应一个DecorView，DecorView其实就是一个FrameLayout控件，这个DecorView才是一个我们看到的界面的根View，一个FrameLayout类型的ViewGroup. Activity的setContentView添加到的是FrameLayout中id为content的view里

PopupWindow，调用的是WindowManagerImpl的addview方法，view被添加到跟DecorView平级，传入PhoneWindow中去
Dialog，是新建一个PhoneWindow，跟Acitity结构一样，有一个DecorView，还有一个id为content的view，然后添加到content中去

### 线程独占区

#### 虚拟机栈
虚拟机栈属于线程独占区，具体指虚拟机栈中的局部变量表。虚拟机栈描述的是Java方法执行的动态内存模型。每个方法执行都会创建一个栈帧，伴随着方法从创建到执行完成。用于存储局部变量表，操作数栈，动态链接，方法出口等。

局部变量表的内存空间，在编译期完成分配，当进入一个方法时，这个方法需要帧中分配多少内存是固定的，在方法运行期间不会改变局部变量表大小

局部变量表保存的是对象的引用，对象是分配在堆内存中

虚拟机栈已满，但仍有方法需要进栈，则会抛出 StackOverflowError，栈未满，但内存不够了，则会抛出OutOfMemory异常

#### 本地方法栈
本地方法栈为虚拟机执行native方法服务

#### 程序计数器

### 线程共享区

#### Java堆

- 新生代
    - Eden
    - Survivor 存活区
    - Tenured Gen
- 老年代

存放对象实例
垃圾收集器管理的主要区域
分新生代，老年代，Eden空间
OutOfMemory

#### 方法区

存储运行时常量池，已被虚拟机加载类信息，常量，静态变量，即时编译器编译后的代码等数据

类信息：
- 类的版本
- 字段
- 方法
- 接口

方法区和永久代：有的虚拟机（HotSpot），使用永久代实现的方法区，但两者并不等价

垃圾回收在方法区中的行为：常量池的回收，类型的卸载

异常：OutOfMemory

#### 运行时常量池

**字符串对象的创建，s1 = new String("a")内容存放在堆内存，而s1 = "a"则存放在方法区的常量池中**


~~~ Java
String s1 = "abc";
String s2 = "abc"; // s1 == s2 true，地址相同， 字节码常量

String s3 = new String("abc"); // s3在堆内存，s1 == s3 false 地址不同

System.out.println(s1 == s3.intern()); // true，s3.intern()则会将内容搬到方法区的常量池中去，地址相同
// s3.intern()叫运行时常量
~~~

#### 直接内存
堆内存之外分配的内存，NIO可以直接分配堆外内存




### 对象的创建

指针碰撞
空闲列表

#### 线程安全问题

- 线程同步  本地线程分配缓冲

#### 对象的结构
Header 对象头
    - 自身运行时数据 (Mark Word)： 哈希值 GC分代年龄 锁状态标志 线程持有的锁 偏向线程ID 偏向时间戳  64bit
    - 类型指针 对象指向它类的元数据的指针

InstanceData

Padding
    填充内存，保证8bit的整数倍

#### 对象的访问定位

- 使用句柄 指向堆中，句柄池，通过句柄池找到对象内存地址
- 直接指针 指向内存区域

### GCRoot对象
- 执行上下文
    - 虚拟机栈(栈帧中的本地变量表)
- 全局性的引用
    - 方法区类属性所引用的对象
    - 方法区中常量所引用的对象
    - 本地方法栈中引用的对象

### 内存分配策略
-verbose:gc 控制台打印gc日志
-XX:+PrintGCDetails
-XX:+UseSerialGC 启动serialgc回收器，默认使用parallel回收器
-Xms20M -Xmx20M 堆内存大小20M，不可扩展
-Xmn10M 指定新生代内存为10M
-XX:SurvivorRatio =8 Eden 区域为80% ??? 

- 对象优先在Eden上分配

- 大对象直接进入老年代
-XX:PretenureSizeThreshold : 指定老年代阈值，超过这个阈值大小的对象会直接放入老年代
- 长期存活对象将进入到老年代
-XX:MaxTenuringThreshold: 指定老年代任期阈值
- 空间分配担保（Eden区域不够用，已分配对象会被移到老年代）
-XX:+HandlerPromotionFailure: 是否禁用空间分配担保（+开启，默认，-禁用）
- 动态对象年龄判断

JVM 参数样式：
- `-XX:+<option>`, 表示开启option选项
- `-XX:-<option>`, 表示关闭option选项
- `-XX:<option>=<value>`, 表示将option选项的值设置为value


#### 逃逸分析与栈上分配
逃逸分析：分析对象的作用域，
栈上分配：方法体内没有逃逸的对象，会分配在栈上 

Jconsole
死锁检测


Davilk 字节码指令，基于寄存器架构
Java 虚拟机的指令集，基于栈架构

### 类的初始化
1.遇到new，getstatic, putstatic或invokestatic这4条字节码指令时，如果类没有进行过初始化，则需要先触发其初始化
常见的场景有：new关键字实例化对象，读取或设置一个类的静态字段（没有被final修饰），调用一个类的静态方法时。
> 被final修饰的叫静态字段，会被放入常量池（方法区）中。所以引用类中的常量字段，是不会触发类的初始化的

2.当初始化一个类的时候，如果发现其父类还没有进行初始化，则需要先触发其父类的初始化
3.使用java.lang.reflect包的方法对类进行反射调用的时候，如果类没有初始化，则需要先触发其初始化

#### 不会初始化类的场景
通过子类引用父类的静态字段，子类不会被初始化
通过数组定义来引用类，类不会被初始化，实例化的是数组类型
~~~ Java
A[] a = new A[10];
// A对象不会初始化
~~~
调用类的常量（final），类不会初始化

助记符：
- anewarray: 表示创建一个引用类型（如类，接口，数组）的数组，并将其压入栈顶
- newarray: 表示创建一个指定的原始类型（如int、float、char等）的数组，并将其压入栈顶

#### 加载
通过一个类的全限定类名来获取定义此类的**二进制流**
- 可以从文件加载
    - Class文件
    - Jar 包
- 网络
    - applet
- 计算生成一个二进制流
    - $Proxy(反射包中)
- 由其他文件生成
    - JSP
- 数据库


#### 验证
验证是连接的第一步，这一阶段目的是为了确保Class文件的字节流中包含的信息符合当前虚拟机的要求，并且不会危害虚拟机自身的安全

#### 准备
准备阶段正式为**类变量**分配内存，并设置**变量的初始值**，这些变量使用的内存都将在方法区中进行分配。
这里的初始值并非我们指定的值，而是默认值。但是如果被final修饰，初始化值为常量值

#### 解析
解析阶段是虚拟机将常量池中的符号引用替换为直接引用的过程
> 关于符号引用和直接引用，可以看这篇文章: https://www.zhihu.com/question/30300585

对符号引用进行解析：

类或接口解析
字段解析
类方法解析  
> 上面三个都会对扶摇引用进行权限验证，不具备访问权限，抛出java.lang.illegalAccessError异常
接口方法解析 接口所有方法都是public，所以不存在权限验证
 
### 初始化

初始化时类加载的最后一步
初始化是执行`<clinit>()`方法的过程

~~~ Java
public class Demo {
    static {
        i = 0;
        System.out.println(i); // i可以赋值，但不能访问
    }

    static int i = 1;
}
~~~
- `<clinit>()`方法是由编译器自动收集类中所有**类变量的赋值动作**和**静态语句块中的语句**合并产生的，编译器收集的顺序是由语句在源文件中出现的顺序决定的，静态语句块中只能访问定义在静态语句块之前的变量，定义在它之后的变量，在前面的语句块中可以赋值，但是不能访问。

~~~ Java
public class Parent {
    public static int A =1;
    static {
        A = 2;
    }

    static class Sub extends Parent {
        public static int B = A;
    }

    public static void main(String[] args) {
        System.out.println(Sub.B);
    }
}
~~~
- 子类的`<clinit>()`在执行之前，虚拟机保证父类的先执行完毕，因此在赋值前父类static已经执行，因此结果为2.

- 接口中也有变量要赋值，也会生成`<clinit>()`，但不需要先执行父类的`<clinit>()`方法。只有父接口中定义的变量使用时才会初始化。

- 如果多个线程同时初始化一个类，只有一个线程会执行这个类的`<clinit>()`，其他线程等待执行完毕。如果方法执行时间过长，则会造成多个线程阻塞。

类的初始化步骤：
- 假如这个类还没有被加载和连接，那就先进行加载和连接
- 假如类存在直接父类，并且这个父类还没有被初始化，那就先初始化直接父类
- 假如类中存在初始化语句，那就依次执行这些初始化语句

类的初始化时机：
- 主动使用
    - 创建类的实例
    - 访问某个类或接口的静态变量，或者对该静态变量赋值
    - 调用类的静态方法
    - 反射（如Class.forName("com.test.XXXX")）
    - 初始化一个类的子类
    - Java虚拟机启动时被标明为启动类的类（Java Test）
    - JDK1.7开始提供的动态语言支持：java.lang.invoke.MethodHandle实例的解析结果REF_getStatic, REF_putStatic, REF_invokeStatic句柄对应的类没有初始化则初始化
- 除了上述7中情形，其他使用Java类的方式都被看作是被动使用，不会导致类的初始化

当Java虚拟机初始化一个类时，要求它的所有父类都已经被初始化，但是这条规则不适用于接口
    - 在初始化一个类时，并不会先初始化它所实现的接口
    - 在初始化一个接口时，并不会初始化它的父接口
只有当程序访问的静态变量或静态方法确实在当前类或当前接口中定义时，才可以认为是对类或接口的主动使用

### 类的实例化
为新的对象分配内存，为实例变量赋初始值，为实例变量赋默认值。
java编译器为它编译的每一个类都至少生成一个实例初始化方法，在java的class文件中，这个实例初始化方法被称为`<init>`。针对源代码中每一个类的构造方法，java编译器都产生一个`<init>`方法。
`类变量是在准备阶段就已经分配内存`

### 类加载器
- Java虚拟机自带的加载器
    - 根类加载器（Bootstrap）
    - 扩展类加载器（Extension）
    - 系统（应用）类加载器（System）
- 用户自己定义的加载器
    - java.lang.ClassLoader的子类
    - 用户可以定制类的加载方式

类加载器并不需要等到某个类被”首次主动使用“时再加载它。

如果在预先加载的过程中遇到.class文件缺失或存在错误，类加载器必须在程序首次主动使用该类是才报告错误(LinkageError错误)，如果这个类一直没有被程序主动使用，那么类加载器就不会报告错误。


#### 双亲委派模型

- 启动类加载器 由C++实现，是虚拟机的一部分，用于加载javahome下的lib目录下的类(rt.jar， java.lang.Object)
- 扩展类加载器  加载javahome下的 /lib/ext/目录中的类
- 应用程序类加载器 加载用户类路径上所指定的类库
- 自定义类加载器 

~~~ Java
class MyParent {
    public static String str = "Hello world":
    static {
        System.out.println("Parent static block");
    }
}

class MyChild extends MyParent {
    public static String str2 = "Welcome":
    static {
        System.out.println("Child static block");
    }
}

class Test {
    public static void main(String[] args) {
        System.out.println(MyChild.str2);  // 打印 Parent static block Child static block Hello world
        // System.out.println(MyChild.str); // 打印 Parent static block Hello world
    }
}
~~~
对于打印str2，主动使用了MyChild类，MyChild会初始化，子类如果被初始化，所有父类必须先行初始化，所以 MyParent会初始化
对于打印str，对于静态字段，只有直接定义了该字段的类才会被初始化，所以不会初始化MyChild类

> `-XX:+TraceClassLoading，用于追踪类的加载信息并打印出来。`
对于打印str，虽然没有打印 "Child static block"，说明没有初始化MyChild，但是也完成了MyChild类的加载

~~~ Java
class MyParent {
    public static final String str = "Hello world":
    static {
        System.out.println("Parent static block");
    }
}

class Test2 {
    public static void main(String[] args) {
        System.out.println(MyParent.str); // 只打印 ”Hello world“
    }
}
~~~
加上final，表示常量在编译阶段，常量会被存入调用这个常量的方法(main方法)所在的类(Test2)的常量池中。甚至，可以把编译后的MyParent类删除，程序可以正常执行。

本质上，调用类并没有直接引用到定义常量的类，因此不会触发定义常量的类的初始化

反编译Test2.class后查看类源码，发现一个助记符：ldc
> ldc表示将int，float或是String类型的常量值从常量池中推送至栈顶
- 助记符 bipush，表示是将单字节（-128 - 127）的常量值推送至栈顶
- 助记符 sipush，表示是将短整形（-32768 - 32767）的常量值推送至栈顶
- 助记符 iconst_1，表示将int类型1推送至栈顶（iconst_m1 - iconst_5）(-1 - 5)

再看下一例子
~~~ Java
class MyParent {
    public static final String str = UUID.randomUUID().toString():

    static {
        System.out.println("Parent static block");
    }
}

public class Test3 {
    public static void main(String[] args) {
        System.out.println(MyParent.str); // 会打印静态代码块
    }
}
~~~

当一个常量的值并非编译期间可以确定，那么其值就不会放到调用类的常量池中。这里str在编译期间不能确定，所以需要加载定义常量的类。

当一个接口初始化时，并不要求父接口都完成了初始化，只有在真正使用到父接口的时候（如引用接口中定义的常量时），才会初始化

类的加载的最终产品是位于内存中的Class对象，Class对象封装了类在方法区内的数据结构，并向Java程序员提供了访问方法区内的数据结构的接口

### 虚拟机字节码执行引擎

#### 运行时栈帧结构

- 局部变量表，编译期已经确定了大小。最小存储单元叫 slot。
slot复用，
全局变量，在加载中会被赋初始值，初始化阶段会被赋上用户定义的值。方法内的局部变量，没有机会赋值，因此使用局部变量必须赋值。
- 操作数栈，虚拟机指令执行方式
- 动态连接，符号引用
- 方法返回地址，附加信息 


#### 方法调用

javap -verbose xxx.class 查看class文件的汇编指令

- 静态分派 编译期确定静态类型，针对方法的重载
~~~ Java
public void sayHello(Parent p) {
    System.out.println("parent called");
}

public void sayHello(Child1 c) {
    System.out.println("child1 called");
}

Parent c = new Child1(); // Parent是静态类型
d.sayHello((Child2)c);// 静态类型改变
~~~
方法调用，多个匹配，会选择一个最比配的
方法翻译为invokevirtual
- 动态分派，针对方法的重写，根据实际类型
方法翻译为invokevirtual，是在invokevirtual里做了处理

#### 动态类型语言支持
静态类型语言在非运行阶段，变量的类型时可以确定的，也是变量是有类型的
动态类型在非运行阶段，变量类型时无法确定的，也就是变量是没有类型的，但值是有类型的，也就是运行期间可以确定变量的值的类型

#### 
Java的内存模型禁止把final字段的写，重排序到构造方法以外。
final 字段不能再静态代码块中赋值。必须在类初始化之后赋值，且只能初始化一次。

初始化顺序：静态代码块，main方法，构造器，代码块

### 面试题
1. i = i++和 i = ++i 
~~~ Java
public static void main(String[] args) {
    int i = 1;
    i = i++;
    int j = i++;
    int k = i + ++i * i++;
    System.out.println("i="+i);
    System.out.println("j="+j);
    System.out.println("k="+k);
}
~~~
i = i++, i 变量先进入操作数栈，然后在局部变量表中进行自增，然后在把栈中的数据返回给我们的变量 i
![Activity View层级](/images/blogimages/2020/jvm_iv_01.webp)

反过来，i = ++i， i 变量先在局部变量表中进行自增，然后再将 i 进栈，然后再把栈中的数据返回给我们的变量 i 

2.类变量准备阶段和初始化阶段差别
~~~ Java
public class Test {
    public static void main(String[] args) {
        Singleton singleton = Singleton.getInstance();
        System.out.println("counter1: "+ Singleton.counter1);
        System.out.println("counter2: "+ Singleton.counter2);
        // 输出接口 1 0
    }
}

class Singleton {
    public static int counter1;
  
    private static Singleton singleton = new Singleton();

    private Singleton() {
        counter1++;
        counter2++;
        // 这里时 counter1 和counter2 都是1
    }

    public static int counter2 = 0; // 这里对counter2重新赋值

    public static Singleton getInstance() {
        return singleton;
    }
}
~~~
Singleton类加载过程中，有两步都对counter2的值进行了修改：
- 在准备阶段，counter1 赋初值 0， singleton赋初值null， 构造方法没有执行，counter2 赋初值 0。
- 外部调用类的静态方法，类被调用，类从上到下进行初始化，singleton初始值为new Singleton()，构造方法被执行，counter2 值为2，然后对counter2初始化，值为0，所以最后打印出来counter2值为0。

把 `public static int counter1;` 改为 `public static int counter1 = 1`;，打印结果是`2，0`。

准备阶段赋初值，初始化阶段为类变量赋予正确默认值





