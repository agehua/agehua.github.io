---
layout: post
title:  注解在Android中的应用
category: accumulation
tags: Annotation
keywords: Annotation, apt, annotationProcessor
banner: http://obxk8w81b.bkt.clouddn.com/Basket%20of%20Potatoes.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Basket%20of%20Potatoes.jpg
toc: true
---

## 注解在Android中的应用
注解是Java语言的特性之一，它是在源代码中插入标签，这些标签在后面的编译或者运行过程中起到某种作用，每个注解都必须通过注解接口 @Interface 进行声明，接口的方法对应着注解的元素。

在上一篇文章[Injection(CDI)和assertion(断言)](https://agehua.github.io/2017/03/14/Android-CDI/)中介绍了Java中存在的注解，这篇文章主要介绍注解在Android中的应用。

先看看Android上著名的View注入框架Butterknife的Bind注解的源码：

~~~ Java
@Retention (RetentionPolicy.Class)
@Target (ElementType.FIELD)
public @interface Bind {
   /** View ID to which the field will be found. **/
   int[] value();
}
~~~
@interface 声明会创建一个实际的Java接口，与其他任何接口一样，注解也会编译成.class文件。@Retention 和@Target 下面会介绍到。

### Java注解的分类

<!--more-->
Java API中默认定义的注解叫做标准注解。它们定义在java.lang、java.lang.annotation和javax.annotation包中。按照使用场景不同，可以分为如下三类：

#### 编译相关注解
编译相关的注解是给编译器使用的，有以下几种：
- @Override：编译器检查被注解的方法是否真的重载了一个来自父类的方法，如果没有，编译器会给出错误提示。
- @Deprecated：可以用来修饰任何不再鼓励使用或已被弃用的属性、方法等。
- @SuppressWarnings：可用于除了包之外的其他声明项中，用来抑制某种类型的警告。
- @SafeVarargs：用于方法和构造函数，用来断言不定长参数可以安全使用
- @Generated：一般是给代码生成工具使用，用来表示这段代码不是开发者手动编写的，而是工具生成的。被@Generated修饰的代码一般不建议手动修改它。
- @FunctionalInterface：用来修饰接口，表示对应得接口是带单个方法的函数式接口

#### 资源相关注解
一共有四个，一帮用在JavaEE领域，Android开发中应该不会用到，就不在详细介绍了。
分别是：
- @PostConstruct
- @PreDestroy
- @Resource
- @Resources

#### 元注解
Butterknife的Bind注解用到的就是元注解。

元注解，顾名思义，就是用来定义和实现注解的注解，总共有如下五种：
- @Retention, 用来指明注解的访问范围，也就是在什么级别保留注解，有三种选择：
  - 源码级注解：使用@Retention(RetentionPolicy.SOURCE)修饰的注解，该类型修饰的注解信息只会保留在 .java源码里，源码经过编译后，注解信息会被丢弃，不会保留在编译好的 .class文件中。
  - 编译时注解：使用@Retention(RetentionPolicy.CLASS)修饰的注解，该类型的注解信息会保留在 .java源码里和 .class文件里，在执行的时候会被Java虚拟机丢弃，不会加载到虚拟机中。
  - 运行时注解：使用@Retention(RetentionPolicy.RUNTIME)修饰的注解，Java虚拟机在运行期间也保留注解信息，可以通过反射机制读取注解的信息
  未指定类型时，默认是CLASS类型。
- @Target, 这个注解的取值是一个ElementType类型的数组，用来指定注解所使用的对象范围，共有十种不同的类型，如下表所示，同时支持多种类型共存，可以进行灵活的组合。

| 元素类型 |  适用于|
| :-------- | :--------|
| ANNOTATION_TYPE  | 注解类型声明 |  
| CONSTRUCTOR    |  构造函数 |  
| FIELD     |   实例变量 |
| LOCAL_VARIABLE     |   局部变量 |
| METHOD     |   方法 |
| PACKAGE     |   包 |
| PARAMETER     |   方法参数或者构造函数的参数 |
| TYPE     |   类（包含enum）和接口（包含注解类型） |
| TYPE_PARAMETER     |   类型参数 |
| TYPE_USE     |   类型的用途 |

  > 如果一个注解的定义没有使用@Target修饰，那么它可以用在除了TYPE_USE和TYPE_PARAMETER之外的其他类型声明中

- @Inherited, 表示该注解可以被子类继承的。
- @Documented, 表示被修饰的注解应该被包含在被注解项的文档中（例如用JavaDoc生成的文档）
- @Repeatable, 表示这个注解可以在同一个项上面应用多次。不过这个注解是在Java 8中才引入的，前面四个元注解都是在Java 5中就已经引入。

### 运行时注解
前面说过，要定义运行时注解只需要在声明注解时指定 @Retention(RetentionPolicy.RUNTIME)即可，运行时注解一般和反射机制配合使用。相比编译时注解性能比较低，但灵活性好，实现起来比较简单。

> Butterknife在较低版本依然是通过运行时反射实现View的注入，性能较低下，不过在8.0.0版本以后使用编译时注解来提升性能。

#### 运行时注解的简单使用

下面展示一个Demo。其功能是通过注解实现布局文件的设置。

之前我们是这样设置布局文件的：

~~~ Java
@Override
protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_home);
}
~~~
如果使用注解，我们就可以这样设置布局了

~~~ Java
@ContentView(R.layout.activity_home)
public class HomeActivity extends BaseActivity {
    ...
}
~~~
我们先不讲这两种方式哪个好哪个坏，我们只谈技术不谈需求。

那么这样的注解是怎么实现的呢？很简单，往下看。

- 创建一个注解
~~~ Java
@Retention(RetentionPolicy.RUNTIME)
@Target({ElementType.TYPE})
public @interface ContentView {
    int value();
}
~~~

前面已经讲过元注解，这不不再介绍。

- 对于：public @interface ContentView

这里的interface并不是说ContentView是一个接口。就像申明类用关键字class。申明枚举用enum。申明注解用的就是@interface。

（值得注意的是：在ElementType的分类中，class、interface、Annotation、enum同属一类为ElementType.Type，并且从官方注解来看，interface是包含@interface的）

~~~ Java
/** Class, interface (including annotation type), or enum declaration */
TYPE,
~~~

- 对于：int value();

返回值表示这个注解里可以存放什么类型值。比如我们是这样使用的
~~~ Java
@ContentView(R.layout.activity_home)
~~~

R.layout.activity_home 实质是一个int型id，如果这样用就会报错：

~~~ Java
@ContentView(“string”)
~~~
关于注解的具体语法，可以看这篇文章[Android编译时注解框架-语法讲解](https://lizhaoxuan.github.io/2016/07/17/apt-Grammar-explanation/)

#### 注解解析

注解申明好了，但具体是怎么识别这个注解并使用的呢？
~~~ Java
@ContentView(R.layout.activity_home)
public class HomeActivity extends BaseActivity {
    ...
}
~~~
注解的解析就在BaseActivity中。我们看一下BaseActivity代码
~~~ Java
public class BaseActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    //注解解析
    for (Class c = this.getClass(); c != Context.class; c = c.getSuperclass()) {
        ContentView annotation = (ContentView) c.getAnnotation(ContentView.class);
        if (annotation != null) {
            try {
                this.setContentView(annotation.value());
            } catch (RuntimeException e) {
                e.printStackTrace();
            }
            return;
        }
    }
}
~~~
解释下上面的代码：

- 第一步：遍历所有的子类
- 第二步：找到修饰了注解ContentView的类
- 第三步：获取ContentView的属性值。
- 第四步：为Activity设置布局。

>  总结：要定义运行时注解，只需要在声明注解时指定@Retention(RetentionPolicy.RUNTIME)即可，运行时注解一般和反射机制配合使用，相比编译时注解性能比较低，但实现比较简单，会提高一定的开发效率。

### 编译时注解

编译时注解能够自动处理Java源文件并生成更多的源码、配置文件、脚本或其他可能想要生成的东西。这些操作是通过**注解处理器**完成的。Java通过在编译期间调用 javac -processor命令可以调起注解处理器，它能够实现编译时注解的功能，从而提高函数库的性能。

#### 定义注解处理器
自定义编译时注解后，需要编写Processor类实现注解处理器，处理自定义注解。Processor继承自AbstractProcessor类并实现process方法，同时需要指定注解处理器能够处理的注解类型以及支持的Java版本，语句如下：

~~~ Java
public class JsonAnnotationProcessor extends AbstractProcessor {

  @Override
  public synchronized void init(ProcessingEnvironment env){
     super.init(env);
     //初始化方法，会被注解处理工具调用，并传入ProcessingEnvironment类型参数，
     //这个参数包含了很多工具类，如Elements、Types、Filer等
     elementUtils = env.getElementUtils();
     typeUtils = env.getTypeUtils();
     filer = env.getFiler();
  }

  @Override
  public Set<String> getSupportedAnnotationTypes() {
    //指定这个注解处理器能够处理的注解类型，返回一个支持的类型字符串合集
    return super.getSupportedAnnotationTypes();
  }

  @Override
  public SourceVersion getSupportedSourceVersion() {
    //指定注解处理器使用的Java版本
    return SourceVersion.latestSupported();
  }

  @Override
  public boolean process(Set<? extends TypeElement> annotations, RoundEnvironment roundEnv) {
      //在这个方法中实现注解处理器的具体业务逻辑，根据输入参数roundEnv可以得到包含特定注解的被注解元素
      //下面代码是Butterknife中这个方法的源码
      Map<TypeElement, BindingClass> targetClassMap = findAndParseTargets(env);

      for (Map.Entry<TypeElement, BindingClass> entry : targetClassMap.entrySet()) {
        TypeElement typeElement = entry.getKey();
        BindingClass bindingClass = entry.getValue();

        try {
          JavaFileObject jfo = filer.createSourceFile(bindingClass.getFqcn(), typeElement);
          Writer writer = jfo.openWriter();
          writer.write(bindingClass.brewJava());
          writer.flush();
          writer.close();
        } catch (IOException e) {
          error(typeElement, "Unable to write view binder for type %s: %s", typeElement,
              e.getMessage());
        }
      }

      //返回值 表示这组 annotations 是否被这个 Processor 接受，
      //如果接受（true）后续子的 pocessor 不会再对这个 Annotations 进行处理
      return true;
  }

  //Butterknife源码：RoundEnvironment表示当前或是之前的运行环境，可以通过该对象查找找到相应的注解。
  private Map<TypeElement, BindingClass> findAndParseTargets(RoundEnvironment env) {
    Map<TypeElement, BindingClass> targetClassMap = new LinkedHashMap<TypeElement, BindingClass>();
    Set<String> erasedTargetNames = new LinkedHashSet<String>();

    // Process each @Bind element.
    for (Element element : env.getElementsAnnotatedWith(Bind.class)) {
      //所有被使用的@Bind注解
      try {
        parseBind(element, targetClassMap, erasedTargetNames);
      } catch (Exception e) {
        logParsingError(element, Bind.class, e);
      }
    }

    ......
  }  
}
~~~

> 一个注解处理器，只能产生新的源文件，它不能够修改一个已经存在的源文件。当没有属于该Process处理的注解被使用时，process不会执行。

从Java7 开始，我们也可以使用注解来代替上面的getSupportedAnnotationTypes()和getSupportedSourceVersion()方法，代码如下：
~~~ Java
@SupportedAnnotationTypes({
  //该注解处理器支持的所有注解全名
})
@SupportedSourceVersion(SourceVersion.RELEASE_7)
public class ContentViewProcessor extends AbstractProcessor {
    ...
}
~~~

#### Element类型
所有通过注解取得元素都将以Element类型等待处理，也可以理解为Element的子类类型与自定义注解时用到的@Target是有对应关系的。

> Element的官方注释：Represents a program element such as a package, class, or method.
Each element represents a static, language-level construct (and not, for example, a runtime construct of the virtual machine).

表示一个程序元素，比如包、类或者方法。

Element的子类有：

- ExecutableElement
表示某个类或接口的方法、构造方法或初始化程序（静态或实例），包括注释类型元素。对应@Target(ElementType.METHOD) @Target(ElementType.CONSTRUCTOR)

- PackageElement
表示一个包程序元素。提供对有关包极其成员的信息访问。对应@Target(ElementType.PACKAGE)

- TypeElement
表示一个类或接口程序元素。提供对有关类型极其成员的信息访问。
对应@Target(ElementType.TYPE)

> 注意：枚举类型是一种类，而注解类型是一种接口。

- TypeParameterElement
表示一般类、接口、方法或构造方法元素的类型参数。
对应@Target(ElementType.PARAMETER)

- VariableElement
表示一个字段、enum常量、方法或构造方法参数、局部变量或异常参数。
对应@Target(ElementType.LOCAL_VARIABLE)

#### Processor输出日志
虽然是编译时执行Processor,但也是可以输入日志信息用于调试的。Processor日志输出的位置在编译器下方的Messages窗口中。Processor支持最基础的System.out方法。

同样Processor也有自己的Log输出工具: Messager。
~~~ Java
//同样是Butterknife源码
private void error(Element element, String message, Object... args) {
    if (args.length > 0) {
      message = String.format(message, args);
    }
    processingEnv.getMessager().printMessage(Diagnostic.Kind.ERROR, message, element);
}
~~~

### 注册注解处理器
为了让javac -processor能够对定义好的注解处理进行处理，我们需要将注解处理器打包到一个jar文件中，同时，需要在jar文件中增加一个名为**javax.annotation.processing.processor**的文件来指明jar文件中有哪些注解处理器，这个文件最终目录在jar文件根目录的META-INF/service目录中，jar文件解压后的目录结构如下图：

![图片来自http://blog.csdn.net/lmj623565791/article/details/43452969](/images/blogimages/2017/processor_jar.png)

javax.annotation.processing.Processor文件的内容是注解处理器全路径名，如果存在多个注解处理器，以换行进行分隔，代码看图片

源文件的目录是，我们需要在src/main/java同级目录中新建一个名为resources的目录，将META-INF/services/javax.annotation.processing.Processor文件放进去就行

> 注意，注解处理器所在的Android Studio工程必须是Java Library类型，而不应该是Android Library类型。因为Android Library的JDK中不包含某些javax包里面的类。

手动实现上面注册过程很繁琐，因此Google开源了一个名为AutoService的函数库，使用这个库后，只需在自定义Processor时使用@AutoService注解标记即可完成上面注册步骤。

~~~ Java
@AutoService(Processor.class)
@SupportedAnnotationTypes({
  //该注解处理器支持的所有注解全名
})
@SupportedSourceVersion(SourceVersion.RELEASE_7)
public class ContentViewProcessor extends AbstractProcessor {
    ...
}
~~~

### android-apt插件
注解处理器所在的jar文件只能在编译期间起作用，到应用运行时不会用到，因此，在build.gradle中引入依赖时应该以provided方式，而不是compile方式引入。

当然，我们可以使用android-apt插件的方式。

APT(Annotation Processing Tool)是一种处理注释的工具,它对源代码文件进行检测找出其中的Annotation，使用Annotation进行额外的处理。
注解处理器在处理Annotation时可以根据源文件中的Annotation生成额外的源文件和其它的文件(文件具体内容由注解处理器的编写者决定),APT还会编译生成的源文件和原来的源文件，将它们一起生成class文件。

android-apt是在Android Studio中使用注解处理器的一个辅助插件，它的作用主要如下：

- 只在编译期间引入注解处理器所在的函数库作为依赖，不会打包到最终生成的APK中。
- 为注解处理器生成的源码设置好正确的路径，以便Android Studio能够正常找到，避免报错。

#### Project项目中使用apt
使用该插件，添加如下到你的构建脚本中：
~~~ Java
//配置在Project下的build.gradle中
buildscript {
    repositories {
      mavenCentral()
    }
    dependencies {
        ...
        //替换成最新android-apt版本
        classpath 'com.neenbedankt.gradle.plugins:android-apt:1.8'
    }
}

apply plugin: 'com.neenbedankt.android-apt'
~~~

接着以apt的方式引入注解处理器函数库作为依赖
~~~ Java
dependencies {
   apt'com.bluelinelabs:logansquare-compiler:1.3.6'
   compile 'com.bluelinelabs:logansquare:1.3.6'
}
~~~

[LoganSquare](https://github.com/bluelinelabs/LoganSquare)是一个实现了编译时注解以提高性能的JSON解析函数库。上面的compiler库就是LoganSquare的注解处理器。

#### 在Module中使用apt
在Module中build.gradle的配置

通常在使用的时候，使用apt声明注解用到的库文件。项目依赖可能分为多个部分。例如Dagger有两个组件Dagger-compiler和dagger。dagger-commpiler仅用于编译时，运行时必需使用dagger。
~~~ Java
//配置到Module下的build.gradle中
apply plugin: 'com.android.application'
apply plugin: 'com.neenbedankt.android-apt'

dependencies {
 apt 'com.squareup.dagger:dagger-compiler:1.1.0'
 compile 'com.squareup.dagger:dagger:1.1.0'
}
~~~

> provided vs apt使用注解处理器的不同？
  provided 将会导入注解处理器的classes和它的依赖到IDE的类路径下。这意味着你可以附带的引入并使用这些classes。例如，当注解处理器使用Guava，你可能错误的import其相关代码到你的Android 代码中。当运行时将导致crash。provided是间接的得到了依赖的Library，起到了避免依赖重复资源的作用。
  而使用apt，注解处理器的classes将不会添加到你当前的类路径下，仅仅用于注解处理过程。并且会把所有注解处理器生成的source放在IDE的类路径下，方便Android Studio引用。


越来越多第三方库使用apt技术，如DBflow、Dagger2、ButterKnife、ActivityRouter、AptPreferences。在编译时根据Annotation生成了相关的代码，非常高大上但是也非常简单的技术，可以给开发带来了很大的便利。


### APT处理annotation的流程
**注解处理器（AbstractProcess）+代码处理（javaPoet）+处理器注册（AutoService）+apt**

具体流程：
- 1.定义注解（如@inject）
- 2.定义注解处理器
- 3.在处理器里面完成处理方式，通常是生成Java代码。
- 4.注册处理器
- 5.利用APT完成如下图的工作内容。

![图片来自http://blog.csdn.net/xx326664162/article/details/68490059](/images/blogimages/2017/apt_processor.png)

### annotationProcessor介绍
annotationProcessor是APT工具中的一种，他是google开发的内置框架，不需要引入，可以直接在build.gradle文件中使用，
ButterKnife就是使用annotationProcessor处理注解，如下：

~~~ Java
dependencies {
     annotationProcessor project(':xx')
     annotationProcessor 'com.jakewharton:butterknife-compiler:8.5.1'
}
~~~

> apt vs annotationProcessor两者有何不同？
  android-apt是由一位开发者自己开发的apt框架，源代码托管在[这里](https://bitbucket.org/hvisser/android-apt)，随着Android Gradle 插件 2.2 版本的发布，Android Gradle 插件提供了名为 annotationProcessor 的功能来完全代替 android-apt ，自此android-apt 作者在官网发表声明最新的Android Gradle插件现在已经支持annotationProcessor，并警告和或阻止android-apt ，并推荐大家使用 Android 官方插件annotationProcessor。
  最近Android N的发布，android 迎来了Java 8，要想使用Java 8的话必须使用Jack编译，android-apt只支持javac编译而annotationProcessor既支持javac同时也支持jack编译。

想用annotationProcessor替代android-apt。删除和替换相应部分即可，具体可以参考[这篇文章](http://www.cnblogs.com/whoislcj/p/6148410.html)


文章参考：

[Android 打造编译时注解解析框架 这只是一个开始](http://blog.csdn.net/lmj623565791/article/details/43452969)
[Android APT（编译时代码生成）最佳实践](https://joyrun.github.io/2016/07/19/AptHelloWorld/)
[Android编译时注解框架系列1-什么是编译时注解](https://lizhaoxuan.github.io/2016/07/17/apt-wathapt/)
[你必须知道的APT、annotationProcessor、android-apt、Provided、自定义注解](http://blog.csdn.net/xx326664162/article/details/68490059)
[《Android高级进阶》一书——注解在Android中的应用](https://www.amazon.cn/%E5%9B%BE%E4%B9%A6/dp/B01MPY3VNG/ref=sr_1_1?ie=UTF8&qid=1491812168&sr=8-1)
