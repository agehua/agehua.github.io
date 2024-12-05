---
layout: post
title: Kotlin DSL 语法糖
category: accumulation
tags:
    - Kotlin
keywords: Kotlin, DSL
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Fish-Drying%20Barn%2C%20Seen%20From%20a%20Height.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Fish-Drying%20Barn%2C%20Seen%20From%20a%20Height.jpg
toc: true
---

### Kotlin 中使用 DSL
最近发现一个使用Kotlin DSL技术来代替XML生成UI布局的工程，类似 Anko，觉得很有意思，特意复制过来，研究一下实现原理，源码在这里 [layout_dsl](https://github.com/agehua/Layout_DSL)

所谓DSL领域专用语言(Domain Specified Language)，其基本思想是“求专不求全”：为专门解决某一特定问题的计算机语言，比如大家耳熟能详的 SQL 和正则表达式。

`Kotlin DSL 所体现的代码结构有如下特点：链式调用，大括号嵌套，并且可以近似于英语句子。`

<!--more-->
### 实现原理
看了那么多 Kotlin DSL 的风格和使用场景，相较于刻板的、传统的 Java 而言，更加神奇和富有想象力。要理解 Kotlin DSL 这场魔术盛宴，就必须了解其背后用到的魔术道具——**扩展函数**、**lambda**、**中缀调用**和 **invoke 约定**。

### 扩展函数（扩展属性）
对于同样作为静态语言的 Kotlin 来说，扩展函数（扩展属性）是让他拥有类似于动态语言能力的法宝，即我们可以为任意对象动态的增加函数或属性。

比如，为 String 扩展一个函数： lastChar():
~~~ Java
package strings

fun String.lastChar(): Char = this.get(this.length - 1)
~~~

与 JavaScript 这类动态语言不一样，Kotlin 实现原理是： 提供静态工具类，将接收对象(此例为 String )做为参数传递进来,以下为该扩展函数编译成 Java 的代码

~~~ Java
/* Java */
char c = StringUtilKt.lastChar("Java");
~~~

### lambda介绍
lambda 为 Java 8 提供的新特性，于2014年3月18日发布。在2018年的今天我们依然无法使用或者要花很大的代价才能在 Android 编程中使用，而 Kotlin 则帮助我们解决了这一瓶颈，这也是我们拥抱 Kotlin 的原因之一。

lambda 是构建整洁代码的一大利器。

#### lambda 表达式

下图是 lambda 表达式，他总是用一对大括号包装起来，可以作为值传递给下节要提到的高阶函数。

![kotlin_lambda](/images/blogimages/2020/kotlin_lamda.jpg)

lambda 表达式总是括在花括号中， 完整语法形式的参数声明放在花括号内，并有可选的类型标注， 函数体跟在一个 `->` 符号之后。如果推断出的该 lambda 的返回类型不是 Unit，那么该 lambda 主体中的最后一个（或可能是单个） 表达式会视为返回值。

### 高阶函数

关于高阶函数的定义，参考《Kotlin 实战》：
> 高阶函数就是以另一个函数作为参数或返回值的函数

如果用 lamba 来作为高价函数的参数（此时为形参），就必须先了解如何声明一个函数的形参类型，如下：

![kotlin_higher_functions](/images/blogimages/2020/kotlin_higher_functions.jpg)

~~~ Java
// printSum 为高阶函数，定义了 lambda 形参
fun printSum(sum:(Int,Int)->Int){
        // sum作为传入的函数的形参，然后可以通过sum 调用方法
        val result = sum(1, 2)
        println(result)
}

// 以下 lambda 为实参，传递给高阶函数 printSum
val sum = {x:Int,y:Int->x+y}
printSum(sum)
~~~

有了高阶函数，我们可以很轻易地做到**一个 lambda 嵌套另一个 lambda 的代码结构**。

#### 大括号放在最后

Kotlin 的 lambda 有个规约：如果 lambda 表达式是函数的最后一个实参，则可以放在括号外面。如果该 lambda 表达式是调用时唯一的参数，那么圆括号可以完全省略：
~~~ Kotlin
person.maxBy({ p:Person -> p.age })

// 可以写成，放在括号外面
person.maxBy(){
    p:Person -> p.age
}

// 更简洁的风格，可以省略括号：
person.maxBy{
    p:Person -> p.age
}
~~~

再举一个例子：

~~~ Kotlin
// { acc, e -> acc * e } 作为函数最后一个实参，放在了括号外面
val product = items.fold(1) { acc, e -> acc * e }
~~~

~~~ Kotlin
// 省略圆括号
run { println("...") }
~~~

> 这种语法也称为拖尾 lambda 表达式。

这个规约是 Kotlin DSL 实现嵌套结构的本质原因，比如很多博客提到的 anko Layout：

~~~ Kotlin
// 省略圆括号
verticalLayout {
    val name = editText()
    button("Say Hello") {
        onClick { toast("Hello, ${name.text}!") }
    }
}
~~~

这里 verticalLayout 中 嵌套了 button，想必该库定义了如下函数：
~~~ Kotlin
fun verticalLayout( ()->Unit ){
    
}

fun button( text:String,()->Unit ){
    
}
~~~
verticalLayout 和 button 均是高阶函数，结合大括号放在最后的规约，就形成了 lambda 嵌套的语法结构。

`注意，允许将函数留在圆括号外的简写语法仅适用于 lambda 表达式。`

#### 带接收者的 lambda
函数类型可以有一个额外的`接收者类型`，它在表示法中的点之前指定： 类型 `A.(B) -> C` 表示可以在 A 的接收者对象上以一个 B 类型参数来调用并返回一个 C 类型值的函数。 带有接收者的函数字面值通常与这些类型一起使用。
如下图：

![kotlin_lambda_with_receiver](/images/blogimages/2020/kotlin_lamba_with_receiver.jpg)

Kotlin 提供了使用指定的**接收者对象**调用函数字面值的功能。 在函数字面值的函数体中，可以调用该接收者对象上的方法而无需任何额外的限定符。

> 函数字面值，有的也叫函数字面量，（英文：function literal）。函数字面值（量）即一段函数文本，说白了就是一段代码，可以当作参数来传递。

~~~ Kotlin
val sum: Int.(Int) -> Int = { other -> plus(other) }
// 相当于: 
val sum: Int.(Int) -> Int = { other -> this.plus(other) } // plus是Int的一个方法
~~~

在使用的时候，需要指定接收者。

~~~ Kotlin
//类似扩展函数的用法，用实例对象来调用
 println( 1.sum(2) )
//输出 3
~~~
1即这里的接收者，定义的函数字面值里的this就是指向了它

匿名函数语法允许你直接指定函数字面值的接收者类型。 如果你需要使用带接收者的函数类型声明一个变量，并在之后使用它，这将非常有用。

~~~ Kotlin
val sum = fun Int.(other: Int): Int = this + other
// 下面写法是错误的，不能既用 = ，又用{}
val sum = fun Int.(other: Int): Int = {this + other}
~~~

此外，带有接收者类型的函数的非字面值可以作为参数进行传递，前提是所需要接收函数的地方应该有一个接收者类型的参数，反之依然，比如说：`String.(Int) -> Boolean 与（String, Int）-> Boolean`是等价的，下面用代码进行说明：

~~~ Kotlin
val myEquals: String.(Int) -> Boolean = {param -> this.toIntOrNull() == param}

println("11".myEquals(11))
println("11".myEquals(12))
// 结果 true false
~~~

> 为了说明"String.(Int) -> Boolean与（String, Int）-> Boolean是等价的"，下面用代码来论证下：

~~~ Kotlin
fun myTest(op:(String, Int) -> Boolean, a: String, b:Int, c:Boolean) = println(op(a, b) == c)
// 这里可以传入 myEquals 函数，并且能成功执行，说明两者是等价的
myTest(myEquals, "200", 200, true)
~~~

带接收者的 lambda 丰富了函数声明的信息，当传递该 lambda值时，将携带该接收者，比如：

~~~ Kotlin
// 声明接收者
fun kotlinDSL(block: StringBuilder.()->Unit){
    block(StringBuilder("Kotlin"))
}

// 调用高阶函数
kotlinDSL {
    // 这个 lambda 的接收者类型为StringBuilder
    append(" DSL")
    println(this)
}

>>> 输出 Kotlin DSL
~~~

简单介绍一下上面代码的意思：kotlinDSL是一个函数，接收一个名为block的参数，该参数本身就是一个函数。
block函数的类型是 StringBuilder.()->Unit，它是一个带接受者的函数类型。这意味着我们需要向block函数传递一个StringBuilder类型的实例（接收者），并且我们可以在kotlinDSL 函数内部调用该实例的成员。该接收者可以通过this关键字访问

可以再看下面这个例子：
~~~ Kotlin
class HTML {
    fun body() { println("HTML BODY") }
}

fun html(init: HTML.() -> Unit): HTML {
    val html = HTML()  // 创建接收者对象
    html.init()        // 将该接收者对象传给该 lambda
    return html
}

html {       // 带接收者的 lambda 由此开始
    body()   // 调用该接收者对象的一个方法
}
~~~

总而言之，lambda 在 Kotlin 和 Kotlin DSL 中扮演着很重要的角色，是实现整洁代码的必备语法糖。

#### 中缀调用
Kotlin 中有种特殊的函数可以使用中缀调用，代码风格如下：
~~~ Kotlin
"key" to "value"
// 等价于
"key.to("value")
~~~
而 to() 的实现源码如下：
~~~ Kotlin
infix fun Any.to(that:Any) = Pair(this,that)
~~~
这段源码理解起来不难，**infix 修饰符**代表该函数支持中缀调用，然后为任意对象提供扩展函数 to，接受任意对象作为参数，最终返回键值对。

中缀调用是实现类似英语句子结构 DSL 的核心。

#### invoke 约定

Kotlin 提供了 invoke 约定，可以让对象向函数一样直接调用，比如：
~~~ Kotlin
class Person(val name:String){
    operator fun invoke(){
        println("my name is $name")
    }
}

>>>val person = Person("geniusmart")
>>> person()
my name is geniusmart
~~~

看下网上提到的 Gradle Kotlin DSL：
~~~ Kotlin
dependencies {
    compile("com.android.support:appcompat-v7:27.0.1")
    compile("com.android.support.constraint:constraint-layout:1.0.2")
}
~~~
// 等价于：
~~~ Kotlin
dependencies.compile("com.android.support:appcompat-v7:27.0.1")
dependencies.compile("com.android.support.constraint:constraint-layout:1.0.2")
~~~
这里，dependencies 是一个实例，既可以调用成员函数 compile，同时也可以直接传递 lambda 参数，后者便是采用了 invoke 约定，实现原理简化如下：
~~~ Kotlin
class Dependencies{

    fun compile(coordinate:String){
        println("add $coordinate")
    }

    operator fun invoke(block:Dependencies.()->Unit){
        block()
    }
}

>>>val dependencies = Dependencies()
>>>// 以两种方式分别调用 compile()
~~~
invoke 约定让对象调用函数的语法结构更加简洁。

### 总结
Kotlin 本身语法就非常整洁，使用DSL则是对 Kotlin 所有语法糖的一个融合，相信以后Kotlin DSL技术应用的地方会越来越多。

参考：
[《Kotlin 之美—DSL篇》](https://juejin.im/entry/6844903569372479501)，部分内容有增加
[带接收者的函数字面值与解构声明详解](https://www.cnblogs.com/webor2006/p/11519460.html)