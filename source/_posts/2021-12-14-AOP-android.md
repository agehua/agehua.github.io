---
layout: post
title: android场景实现AOP编程
category: accumulation
tags:
    - ASM
    - AGP
    - Transform 
    - AOP
keywords: ASM, AGP, Transform
banner: https://cdn.conorlee.top/Girl%20in%20White%20in%20the%20Woods.jpg
thumbnail: https://cdn.conorlee.top/Girl%20in%20White%20in%20the%20Woods.jpg
toc: true
---
我在很早之前就了解过面向切面（AOP）的编程思想，比如[AspectJ](http://www.eclipse.org/aspectj/index.php) 实现简单的切面编程，不过都快忘了 `:(`。本文是想探究AOP在Android环境中的使用，补全Android开发者的技能树。

常见的AOP编程框架有AspectJ、 Cglib，Hibernate 和 Spring 等等，而这些目前流行的AOP框架绝大多数底层实现都是直接或间接地通过 ASM 来实现字节码操作，本文重点就来介绍一下ASM。
<!--more-->
### ASM简介

ASM是一个通用的Java字节码操作和分析框架，它可以用来修改现有的类或直接以二进制形式动态生成类。官网地址是：https://asm.ow2.io/


#### java字节码简单介绍
由于 ASM 是直接对class文件的字节码进行操作，因此，要修改class文件内容时，也要注入相应的java字节码。

所以，在注入字节码之前，我们还需要了解下class文件的结构，JVM指令等知识。

1.class文件结构
Java源文件经过javac编译器编译之后，将会生成对应的二进制.class文件

Java类文件是 8 位字节的二进制流。数据项按顺序存储在class文件中，相邻的项之间没有间隔，这使得class文件变得紧凑，减少存储空间。在Java类文件中包含了许多大小不同的项，由于每一项的结构都有严格规定，这使得 class 文件能够从头到尾被顺利地解析。

![class文件结构图](/images/blogimages/2021/class-file-structure.png)

- 类结构体中所有的修饰符、字符常量和其他常量都被存储在class文件开始的一个常量堆栈(Constant Stack)中，其他结构体通过索引引用。
- 每个类必须包含headers（包括：class name, super class, interface, etc.）和常量堆栈（Constant Stack）其他元素，例如：字段（fields）、方法（methods）和全部属性（attributes）可以选择显示或者不显示。
- 每个字段块（Field section）包括名称、修饰符（public, private, etc.）、描述符号(descriptor)和字段属性。
- 每个方法区域（Method section）里面的信息与header部分的信息类似，信息关于最大堆栈（max stack）和最大本地变量数量（max local variable numbers）被用于修改字节码。对于非abstract和非native的方法有一个方法指令表，exceptions表和代码属性表。除此之外，还可以有其他方法属性。
- 每个类、字段、方法和方法代码的属性有属于自己的名称记录在类文件格式的JVM规范的部分，这些属性展示了字节码多方面的信息，例如源文件名、内部类、签名、代码行数、本地变量表和注释。JVM规范允许定义自定义属性，这些属性会被标准的VM（虚拟机）忽略，但是可以包含附件信息。
- 方法代码表包含一系列对java虚拟机的指令。有些指令在代码中使用偏移量，当指令从方法代码被插入或者移除时，全部偏移量的值可能需要调整。

2.Java类型与class文件内部类型对应关系
Java类型分为基本类型和引用类型，在 JVM 中对每一种类型都有与之相对应的类型描述，如下表：

| *Java type*| *JVM Type descriptor* |
|:--------:|:-------:|
|boolean|Z|
|char|C|
|byte	|B|
|short	|S|
|int	|I|
|float	|F|
|long	|J|
|double	|D|
|Object	| Ljava/lang/Object;|
|int[]	|[I|
|Object[][]	|[[Ljava/lang/Object;|

在 ASM 中要获得一个类的 JVM 内部描述，可以使用org.objectweb.asm.Type类中的getDescriptor(final Class c)方法，如下：
~~~ java
public class TypeDescriptors {    
    public static void main(String[] args) {    
        System.out.println(Type.getDescriptor(TypeDescriptors.class));    
        System.out.println(Type.getDescriptor(String.class));    
    }        
}
~~~
运行结果：
~~~ java
Lorg/victorzhzh/core/structure/TypeDescriptors;    
Ljava/lang/String;    
~~~

3.Java方法声明与class文件内部声明的对应关系
在`Java`的二进制文件中，方法的方法名和方法的描述都是存储在Constant pool 中的，且在两个不同的单元里。因此，方法描述中不含有方法名，只含有参数类型和返回类型。
格式：(参数描述符)返回值描述符

| *Method declaration in source file* | *Method descriptor* |
|:--------:|:-------:|
|void m(int i, float f)	 | (IF)V |
|int m(Object o)|	(Ljava/lang/Object;)I|
|int[] m(int i, String s)|	(ILjava/lang/String;)[I|
|Object m(int[] i)	|([I]Ljava/lang/Object;|
|String m()	|()Ljava/lang/String;|

上面3点，如果不熟悉字节码操作集合的话，写起来会很费劲，所以ASM为主流的IDE专门提供了开发插件BytecodeOutline：

- IDEA：ASM Bytecode Outline
- Eclipse：BytecodeOutline

以IDEA为例，只需要对应的类中右击->Show Bytecode outline即可

### ASM 框架执行流程
ASM对字节码的操作和分析都是基于访问者模式来实现，而访问者模式有两个核心类分别是：独立的访问者、接收访问者事件产生器
对应的ASM里面就是两个核心类：ClassVisitor和ClassReader，下面分别进行介绍；

> 关于访问者模式的介绍，可以看这篇文章：[访问者模式](https://refactoringguru.cn/design-patterns/visitor)

#### ClassVisitor

ASM 内部采用 访问者模式 将 .class 类文件的内容从头到尾扫描一遍，每次扫描到类文件相应的内容时，都会调用ClassVisitor内部相应的方法。
比如：

- 扫描到类文件时，会回调ClassVisitor的visit()方法；
- 扫描到类注解时，会回调ClassVisitor的visitAnnotation()方法；
- 扫描到类成员时，会回调ClassVisitor的visitField()方法；
- 扫描到类方法时，会回调ClassVisitor的visitMethod()方法；
······
扫描到相应结构内容时，会回调相应方法，该方法会返回一个对应的字节码操作对象（比如，visitMethod()返回MethodVisitor实例），通过修改这个对象，就可以修改class文件相应结构部分内容，最后将这个ClassVisitor字节码内容覆盖原来.class文件就实现了类文件的代码切入。

具体关系如下：

| *树形关系*| *使用的接口* |
|:--------:|:-------:|
|Class|	ClassVisitor|
|Field|	FieldVisitor|
|Method	|MethodVisitor|
|Annotation	|AnnotationVisitor|

整个具体的执行时序如下图所示：
![gradle的架构](/images/blogimages/2021/asm-visitor-sequence.png)

根据时序，上面所有方法都会被事件产生器ClassReader调用，所有方法中的参数都是ClassReader提供的

#### ClassReader
此类主要功能就是读取字节码文件，然后把读取的数据通知ClassVisitor，字节码文件可以多种方式传入：
~~~ java
public ClassReader(final InputStream inputStream)：字节流的方式；
public ClassReader(final String className)：文件全路径；
public ClassReader(final byte[] classFile)：二进制文件；
~~~
常见使用方式如下所示：
~~~ java
ClassReader classReader = new ClassReader("com/zh/asm/TestService");
ClassWriter classVisitor = new ClassWriter(ClassWriter.COMPUTE_MAXS);
classReader.accept(classVisitor, 0);
~~~
ClassReader的accept方法处理接收一个访问者，还包括另外一个parsingOptions参数，选项包括：

- **SKIP_CODE**：跳过已编译代码的访问（如果您只需要类结构，这可能很有用）；
- **SKIP_DEBUG**：不访问调试信息，也不为其创建人工标签；
- **SKIP_FRAMES**：跳过堆栈映射帧；
- **EXPAND_FRAMES**：解压缩这些帧；

#### ClassWriter
以上实例中使用了ClassWriter，其继承于ClassVisitor，主要用来生成类，可以单独使用，如下所示：
~~~ java
ClassWriter cw = new ClassWriter(0);
cw.visit(V1_5, ACC_PUBLIC + ACC_ABSTRACT + ACC_INTERFACE,"pkg/Comparable", null, "java/lang/Object",new String[]{"pkg/Mesurable"});
cw.visitField(ACC_PUBLIC + ACC_FINAL + ACC_STATIC, "LESS","I", null, new Integer(-1)).visitEnd();
cw.visitField(ACC_PUBLIC + ACC_FINAL + ACC_STATIC, "EQUAL","I", null, new Integer(0)).visitEnd();
cw.visitField(ACC_PUBLIC + ACC_FINAL + ACC_STATIC, "GREATER","I", null, new Integer(1)).visitEnd();
cw.visitMethod(ACC_PUBLIC + ACC_ABSTRACT, "compareTo","(Ljava/lang/Object;)I", null, null).visitEnd();
cw.visitEnd();
byte[] b = cw.toByteArray();

//输出
FileOutputStream fileOutputStream = new FileOutputStream(new File("F:/asm/Comparable.class"));
fileOutputStream.write(b);
fileOutputStream.close();
~~~
以上通过ClassWriter生成一个字节码文件，然后转换成字节数组，最后通过FileOutputStream输出到文件中，反编译结果如下：
~~~ java
package pkg;

public interface Comparable extends Mesurable {
    int LESS = -1;
    int EQUAL = 0;
    int GREATER = 1;

    int compareTo(Object var1);
}
~~~
在实例化ClassWriter需要提供一个参数flags，选项包括：

- **COMPUTE_MAXS**：将为你计算局部变量与操作数栈部分的大小；还是必须调用 `visitMaxs`，但可以使用任何参数：它们将被忽略并重新计算；使用这一选项时，仍然必须自行计算这些帧；
- **COMPUTE_FRAMES**：一切都是自动计算；不再需要调用 `visitFrame`，但仍然必须调用 visitMaxs（参数将被忽略并重新计算）；
- **0**：不会自动计算任何东西；必须自行计算帧、局部变量与操作数栈的大小；

以上只是对ClassWriter的单独使用，但更有意义的其实是把以上三个核心类整合起来使用，下面重点看看转换操作；
转换操作
在类读取器和类写入器之间引入一个 ClassVisitor，把三者整合起来，大致代码结构如下所示：
~~~ java
ClassReader classReader = new ClassReader("com/zh/asm/TestService");
ClassWriter classWriter = new ClassWriter(ClassWriter.COMPUTE_MAXS);
//处理
ClassVisitor classVisitor = new AddFieldAdapter(classWriter...);
classReader.accept(classVisitor, 0);
~~~

这里提供了一个添加属性的适配器，可以重写`visitEnd`方法，然后写入新的属性，代码如下：
~~~ java
public class AddFieldAdapter extends ClassVisitor {
    private int fAcc;
    private String fName;
    private String fDesc;
    //是否已经有相同名称的属性
    private boolean isFieldPresent;

    public AddFieldAdapter(ClassVisitor cv, int fAcc, String fName,
                           String fDesc) {
        super(ASM4, cv);
        this.fAcc = fAcc;
        this.fName = fName;
        this.fDesc = fDesc;
    }

    @Override
    public FieldVisitor visitField(int access, String name, String desc,
                                   String signature, Object value) {
        //判断是否有相同名称的字段，不存在才会在visitEnd中添加
        if (name.equals(fName)) {
            isFieldPresent = true;
        }
        return cv.visitField(access, name, desc, signature, value);
    }

    @Override
    public void visitEnd() {
        if (!isFieldPresent) {
            FieldVisitor fv = cv.visitField(fAcc, fName, fDesc, null, null);
            if (fv != null) {
                fv.visitEnd();
            }
        }
        cv.visitEnd();
    }
}
~~~
根据ClassVisitor的每个方法被调用的顺序，如果类中有多个属性，那么`visitField`会被调用多次，每次都会检查要添加的字段是否已经有了，然后保存在isFieldPresent标识中，这样在访问最后的visitEnd中判断是否需要添加新属性；
~~~ java
ClassVisitor classVisitor = new AddFieldAdapter(classWriter,ACC_PUBLIC + ACC_FINAL + ACC_STATIC,"id","I");
~~~
这里添加了一个public static final int id；可以把字节数组写入class类文件中，然后反编译查看：
~~~ java
public class TestService {
    public static final int id;
    ......
}
~~~
visitField()方法返回了一个 FieldVisitor，它是一个抽象类，所以可以在 visitField() 方法中返回我们自己定义的 `FieldVisitor` 的实现类，去处理对应的逻辑；同理还有 `MethodVisitor`

#### 几个常见的工具类介绍
- **Type**
Type对象表示一种 Java类型，既可以由类型描述符构造，也可以由Class对象构建；Type类还包含表示基元类型的静态变量；
- **TraceClassVisitor**
扩展了ClassVisitor类，并构建了所访问类的文本表示；使用TraceClassVisitor以便获得实际生成内容的可读跟踪；
- **CheckClassAdapter**
ClassWriter 类并不会核实对其方法的调用顺序是否恰当，以及参数是否有效；因此有可能会生成一些被 Java 虚拟机验证器拒绝的无效类。为了尽可能提前检测出部分此类错误，可以使用CheckClassAdapter类 ；
- **ASMifier**
这个类为TraceClassVisitor工具提供了一个可选的后端（默认情况下，它使用一个Textifier后端，产生上面显示的输出类型）。这个后端使TraceClassVisitor类的每个方法打印用于调用它的Java代码
- **LocalVariablesSorter**
继承自MethodVisitor，将一个方法中使用的局部变量按照它们在这个方法中的出现顺序重新进行编号，同时可以使用 newLocal 方法创建一个新的局部变量；
- **AdviceAdapter**
继承自LocalVariablesSorter，此适配器是一个抽象类，可用于在方法的开头以及任何RETURN或ATHROW指令之前插入代码；其主要优点是它也适用于构造函数，其中代码不能仅插入构造函数的开头，而是在调用超级构造函数之后插入。

处理字节码的工具介绍完了，那android的插件如何将操作后的字节码打包的apk中呢？
官方提供了一份在打包 dex 文件之前的编译过程中操作 .class 文件的api，那就是 **`AGP`**

### AGP(Android Gradle Plugin)

> 很早之前郭霖大神就写过一篇文章：[Android Gradle高级用法，动态编译技术:Plugin Transform Javassist操作Class文件](https://blog.csdn.net/c10WTiybQ1Ye3/article/details/78098450)

文章中提到：gradle 从1.5开始，gradle 插件包含了一个叫 Transform 的 API，这个 API 允许第三方插件在 class 文件转为为 dex 文件前操作编译好的 class 文件，这个 API 的目标是简化自定义类操作，而不必处理 Task，并且在操作上提供更大的灵活性。

他文章里的transform依赖是这样
~~~ java
compile 'com.android.tools.build:transform-api:1.5.0'
~~~
但是现在是这样
~~~ java
//从2.0.0版本开始就是在gradle-api中了
implementation 'com.android.tools.build:gradle-api:3.1.4'
~~~
关于Gradle版本和Gradle Plugin的版本，参考官方的介绍：[gradle plugin版本说明](https://developer.android.com/studio/releases/gradle-plugin#versioning-update)，这里简单列出常见的几个版本：

| *Gradle版本* |*Gradle Plugin版本*|
|:--------:|:-------:|
| 4.1+|3.0.0+|
|4.4+|3.1.0+|
|5.4.1+|3.5.0 - 3.5.4|
|6.5+| 4.1.0+|
|7.0+|7.0|

AGP 4.1 中移除了构建缓存，本文使用的AGP版本为 3.5.4


#### Gradle的架构

![gradle的架构](/images/blogimages/2021/gradle-architure.webp)

- 在最下层的是底层Gradle框架，它主要提供一些基础服务，如task的依赖，有向无环图的构建等
- 上面的则是Google编译工具团队的Android Gradle plugin框架，它主要是在Gradle框架的基础上，创建了很多与Android项目打包有关的task及artifacts
- 最上面的则是开发者自定义的Plugin，一般是在Android Gradle plugin提供的task的基础上，插入一些自定义的task，或者是增加Transform进行编译时代码注入

#### android plugin层
语言选择：Google编译工具组从3.2.0开始，新增的插件全部都是用Kotlin编写的。

怎么查看官方的plugin都有哪些呢，在android studio Project视图 > External libraries > com.android.tools.build:gradle:x.x.x里，如下图：

![官方gradle插件](/images/blogimages/2021/gradle-official-plugins.png)

如何创建Plugin我们后面再说，可以提前了解一点的是：官方和我们自己实现的plugin都是基于 `org.gradle.api.Plugin` 实现的，都需要提供一个类似 `META-INF.gradle-plugins/xxx.properties` 这样的配置文件，文件内容是需要指定 “`implementation-class`”的实现类是什么。

打开上图里的任意一个`.properties`文件，里面就是对应plugin实现类的地址

#### 自定义Gradle Plugin
Gradle支持通过插件的方式来扩展功能，已保证开发者可以按照自己的需求来完成自动化构建。而且Gradle插件允许已任何可以被编译成字节码的语言编写
Gradle提供如下几个插件编写方式：
- Build Script  
我们可以在build脚本中直接完成插件代码的编写，这种方式缺点是只能在一个脚本中使用，优点是开发编译都很方便
- buildSrc 工程
除了应用程序工程，Gradle脚本同样也可以作为工程进行管理。默认情况下，Gradle会自动处理如下路径的插件源码，并添加到编译流程中：
~~~ java
[Project_root]/buildSrc/src/main/groovy
~~~
这种方式整个app所有build脚本中都可以使用，但也仅限于本应用程序范畴
- Standalone Project
以独立工程实现，在这种情况下，插件JAR包存在，并支持在一个包中包含多个插件

根据Gradle规定，用户自定义的插件需要实现Plugin接口。当我们在build脚本中使用插件时，可以通过apply plugin:[PLUG_ID]来指明。然后Gradle会调用该插件中的apply方法：
~~~ java
apply plugin: GreetingPlugin

class GreetingPlugin implements Plugin<Project> {
    void apply(Project project) {
        project.task('hello') << {
            println "Hello from the GreetingPlugin"
        }
    }
}
~~~
这段代码用于添加一个名为”hello“的task，它的代码虽然简单，但是apply方法中传递了一个重要的类 `org.gradle.api.Project`，其他复杂的插件都是通过Project类来操作编译过程，达到对应的效果。
下面是 Project 类的一些重要方法：
~~~ java
// 根据name创建一个task，就是上面例子里的写法，还有好多同名方法，这里不一一列举了
Task task(String name) throws InvalidUserDataException;
// 根据一个工程路径，获得一个File对象，省略同名方法
File file(Object path);
// Adds a closure to be called immediately after this project has been evaluated.
afterEvaluate​(Closure closure)
//Returns the properties of this project.
getProperties()
// ...
~~~
其他方法的介绍可以看gradle 文档：https://docs.gradle.org/current/javadoc/org/gradle/api/Project.html

如果我们不单只是创建一个Task去执行，还想在android编译过程中进行处理，则需要使用到`Transform Api`，像下面这种方式：
~~~ java
AppExtension appExtension = (AppExtension)project.getProperties().get("android");
// 或者
// AppExtension appExtension = project.extensions.getByType(AppExtension)

//将自定义Transform添加到编译流程中
appExtension.registerTransform(new MethodTimeTransform(project));
~~~

#### Transform Api
Transform 是专门处理构建过程中的中间产物，Transform 可以被看作是 Gradle 在编译项目时的一个 task，在 .class 文件转换成 .dex 的流程中会执行这些 task，对所有的 .class 文件（可包括第三方库的 .class）进行转换，转换的逻辑定义在 Transform 的 transform 方法中。实际上平时我们在 build.gradle 中常用的功能都是通过 Transform 实现的，比如混淆（proguard）、dexBuilder等。

篇幅有限，本文关于当前android上实现AOP编程就介绍到这里。关于Transform api的语法和具体例子可以看这篇文章：[手把手教大家用Transform API和ASM实现一个防快速点击案例](https://juejin.cn/post/6864349303843307534)




### Ref
[字节码增强技术探索](https://tech.meituan.com/2019/09/05/java-bytecode-enhancement.html)

ASM教程：https://github.com/dengshiwei/asm-module

ASM入门篇：https://segmentfault.com/a/1190000040160637

参考：https://juejin.cn/post/6844903616487096333