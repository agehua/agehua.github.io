---
layout: post
title:  Injection(CDI)和assertion(断言)
category: accumulation
tags: Injection
keywords: Injection, assert
banner: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Acacia%20Branches.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Acacia%20Branches.jpg
toc: true
---

### @Inject注解和CDI（上下文依赖注入）
怎么让代码显得高大上呢，当然是多使用注解，那[Android Support Annotation](https://developer.android.com/reference/android/support/annotation/package-summary.html)使用完了，有没有更高端的呢？

这篇文章就简单介绍下Java中存在的注解。

CDI（Contexts and Dependency Injection 上下文依赖注入）
[Java 依赖注入标准（JSR-330）](http://blog.csdn.net/DL88250/article/details/4838803)规范，在javax.inject包中提供了一个接口Provider，和几个注解类型@Scope、@Inject、@Named、@Qualifier

#### @Inject
@Inject是javax.inject中提供的一个注解，可以不使用new关键字直接创建对象，怎么样，是不是很高端 :)

##### 在字段注解：
用@Inject注解，字段不能是final的。可以拥有一个合法的名称

<!--more-->

doc文档原文：

~~~ Java
Injectable fields:

  - are annotated with @Inject.
  - are not final.
  - may have any otherwise valid name.

@Inject FieldModifiers(opt) Type VariableDeclarators;
~~~

     (opt)这里表示可选

##### 在方法上注解：
用@Inject注解，不能是抽象方法，不能声明自身参数类型。可以有返回结果。拥有一个合法的名称。可以有0个或多个参数

(doc文档原文)[http://docs.oracle.com/javaee/6/api/javax/inject/Inject.html]：

~~~ Java
Injectable methods:
- are annotated with @Inject.
- are not abstract.
- do not declare type parameters of their own.
- may return a result
- may have any otherwise valid name.
- accept zero or more dependencies as arguments.


@Inject MethodModifiers(opt) ResultType Identifier(FormalParameterList(opt)) Throws(opt) MethodBody
~~~

     (opt)这里表示可选

代码示例：

~~~ Java
@Inject
WeatherDao weatherDao;
~~~

~~~ Java
@Inject
WeatherDao(Context context) {
    this.context = context;
}
~~~
@Inject支持构造函数、方法和字段注解，也可能使用于静态实例成员。
可注解成员可以是任意修饰符（private,package-private,protected,public）。
注入顺序：构造函数、字段，然后是方法。父类的字段和方法注入优先于子类的字段和方法，同一类中的字段和方法是没有顺序的。

**@Inject注解的构造函数可以是无参或多个参数的构造函数。@Inject每个类中最多注解一个构造函数。**

除了@Inject注解，还有@Named、@Qualifier和@Provider，下面简单介绍一下。

#### @Named
@Named，一般和@Inject一起使用，如果没有值生成的Bean名称默认和类名相同。

如果指定名称，那么就生成一个指定名称的Bean。
~~~ Java
public class Car {
     @Inject @Named("driver") Seat driverSeat;
     @Inject @Named("passenger") Seat passengerSeat;
     ...
}
~~~

#### @Qualifier
自定义一个新的修饰语（注解），一个qualifier注解应该满足如下条件：
- 定义的注解类有@Qualifier，@Retention(RUNTIME)和@Documented。
- 可以有属性
- 可以是公共API的一部分
- 可以用@Target注解限定使用范围

[doc文档原文](http://docs.oracle.com/javaee/6/api/javax/inject/Qualifier.html)是：
~~~ Java
Identifies qualifier annotations. Anyone can define a new qualifier. A qualifier annotation:

- is annotated with @Qualifier, @Retention(RUNTIME), and typically @Documented.
- can have attributes.
- may be part of the public API, much like the dependency type, but unlike implementation types which needn't be part of the public API.
- may have restricted usage if annotated with @Target. While this specification covers applying qualifiers to fields and parameters only, some injector configurations might use qualifier annotations in other places (on methods or classes for example).
~~~

下面是[java doc文档@Named注解](http://docs.oracle.com/javaee/6/api/javax/inject/Named.html)的生成代码
~~~ Java
@Qualifier
@Documented
@Retention(value=RUNTIME)
public @interface Named
~~~

~~~ Java
@Documented  
@Retention(RetentionPolicy.RUNTIME)  
@Qualifier  
@Target(value = {ElementType.FIELD, ElementType.PARAMETER, ElementType.TYPE})  
public @interface Genre {  
    User user() default User.STUDENT;  
    public enum User {STUDENT, TEACHER}  
}
~~~

~~~ Java
@Named  
@Genre(user = User.STUDENT)  
public class StudentDAO implements IUserDAO{  
    @Override  
    public int count() {  
        System.out.println("----StudentDAO----");  
        return 0;  
    }  
}  
~~~

#### @Provider
@Provider注解，可以实现任意类型的对象的注入。相对于直接注入T对象，@Provider提供了一个T.get()方法来获取注入的对象

[doc文档原文](http://docs.oracle.com/javaee/6/api/javax/inject/Provider.html):
~~~ Java
Provides instances of T. Typically implemented by an injector. For any type T that can be injected, you can also inject Provider<T>. Compared to injecting T directly, injecting Provider<T> enables:

- retrieving multiple instances.
- lazy or optional retrieval of an instance.
- breaking circular dependencies.
- abstracting scope so you can look up an instance in a smaller scope from an instance in a containing scope.
~~~

For example:
~~~ Java
class Car {
@Inject Car(Provider<Seat> seatProvider) {
   Seat driver = seatProvider.get();
   Seat passenger = seatProvider.get();
   ...
 }
}
~~~

get方法解释
~~~ Java
T	get()
    Provides a fully-constructed and injected instance of T.
~~~

#### @Singleton
使用该注解标记该类只创建一次，不能被继承。一般在类上用该注解。



[doc原文](http://docs.oracle.com/javaee/6/api/javax/inject/Singleton.html):
~~~ Java
@Scope
@Documented
@Retention(value=RUNTIME)
public @interface Singleton
Identifies a type that the injector only instantiates once. Not inherited.
~~~


#### @Scope
注解 @Scope 用于标识作用域注解。一个作用域注解是被标识在包含一个可注入构造器的类上的，用于控制该类型的实例如何被注入器重用。缺省情况下，如果没有标识作用域注解，注入器将为每一次注入都创建（通过注入类型的构造器）新实例，并不重用已有实例。如果多个线程都能够访问一个作用域内的实例，该实例实现应该是线程安全的。作用域实现由注入器完成。

在下面的例子中，作用域注解 @Singleton 确保我们始终只有一个 Log 实例：

~~~ Java
@Singleton
class Log {
   void log(String message) { ... }
}
~~~

当多于一个作用域注解或不被注入器支持的作用域注解被使用在同一个类上时，注入器将生成一个错误。

一个作用域注解：

- 被标注了 @Scope、@Retention(RUNTIME) 标注的，通常也被 @Documented 标注。
- 不应该含有属性。
- 不应该被 @Inherited 标注，因此作用域与继承实现（正交）无关。
- 如果标注了 @Target 可能会有一些用法限制。

使用 @Scope 来标识一个作用域注解有助于注入器探测程序员使用了作用域注解但却忘了去配置作用域的情况。一个保守的注入器应该生成一个错误而不是去适用该作用域。

[doc原文请点击](http://docs.oracle.com/javaee/6/api/javax/inject/Scope.html)

### assertion(断言)
在实现中，assertion就是在程序中的一条语句，它对一个boolean表达式进行检查，一个正确程序必须保证这个boolean表达式的值为true；如果该值为false，说明程序已经处于不正确的状态下，系统将给出警告并且退出。


在语法上，为了支持assertion，Java增加了一个关键字assert。它包括两种表达式，分别如下：

~~~ Java
1.assert expression1;

2.assert expression1: expression2;
~~~

expression1表示一个boolean表达式，expression2表示一个基本类型、表达式或者是一个Object，用于在失败时输出错误信息。

- 在运行时，如果关闭了assertion功能，这些语句将不起任何作用。
- 如果打开了assertion功能，那么expression1的值将被计算。
- 如果它的值为false，该语句强抛出一个AssertionError对象。
- 如果expression1值为true，expression2将不被计算。

如何关闭和开启assertion功能，[请看这篇文章](http://blog.csdn.net/ithomer/article/details/9185125)
