---
layout: post
title: Kotlin 与 Java 语言比较
category: accumulation
tags:
    - Kotlin
keywords: Kotlin, Java
banner: http://cdn.conorlee.top/Flowering%20Garden%20with%20Path.jpg
thumbnail: http://cdn.conorlee.top/Flowering%20Garden%20with%20Path.jpg
toc: true
---

本文整理了一些 Kotlin 与 Java 语言的区别，方便从Java转型学习Kotlin的同学。

### Kotlin 解决了一些 Java 中的问题
Kotlin 通过以下措施修复了 Java 中一系列长期困扰我们的问题：
<!--more-->
+代表理解难度

- 空引用由类型系统控制（可空类型与非空类型）(+)
- 无原始类型(+)
- Kotlin 中数组是不型变的(++)
- 相对于 Java 的 SAM(Single Abstract Method)-转换，Kotlin 有更合适的函数类型(++)
- 没有通配符的使用处型变(+++)
- Kotlin 没有受检异常(+)
### Java 有而 Kotlin 没有的东西
- 受检异常
- 原始类型 —— 字节码会尽可能试用原生类型，但不是显式可用的。
- 静态成员 —— 以 伴生对象、 顶层函数、 扩展函数 或者 @JvmStatic 取代。
- 通配符类型 —— 以 声明处协变 与 类型投影 取代。
- 三目操作符 `a ? b : c` —— 以 `if` 表达式取代
### Kotlin 有而 Java 没有的东西
- Lambda 表达式 + 内联函数 = 高性能自定义控制结构(++)
- 扩展函数
- 空安全
- 智能类型转换
- 字符串模板
- 类的属性
- 主构造函数
- 委托
- 变量与属性类型的类型推断
- 单例
- 声明处型变 & 类型投影
- 区间表达式 `in a...b step 2`
- 操作符重载 `operator`关键字
- 伴生对象 `companion`
- 数据类 `data class A`
- 密封类 `sealed class A`
- 分离用于只读与可变集合的接口 `MutableList, MutableSet, MutableMap`
- 协程

### 空安全
~~~ java
var a: String = "abc" // 默认情况下，常规初始化意味着非空
a = null // 编译错误

var b: String? = "abc" // 可以设置为空
b = null // ok
print(b)

val l = b.length // 错误：变量“b”可能为空
~~~

但是我们还是需要访问该属性，对吧？有几种方式可以做到

#### 在条件中检测 null
首先，可以显式检测 b 是否为 null，并分别处理两种可能：
~~~ java
val l = if (b != null) b.length else -1
~~~
#### 安全的调用
第二个选择是安全调用操作符，写作 `?.`
~~~ java
val a = "Kotlin"
val b: String? = null
println(b?.length)
println(a?.length) // 无需安全调用
~~~
如果 b 非空，就返回 b.length，否则返回 null，这个表达式的类型是 `Int?`。

对集合，如果要只对非空值执行某个操作，安全调用操作符可以与 let 一起使用：
~~~ java
val listWithNulls: List<String?> = listOf("Kotlin", null)
for (item in listWithNulls) {
    item?.let { println(it) } // 输出 Kotlin 并忽略 null
}
~~~

安全调用也可以出现在赋值的左侧。这样，如果调用链中的任何一个接收者为空都会跳过赋值，而右侧的表达式根本不会求值：
~~~ java
// 如果 `person` 或者 `person.department` 其中之一为空，都不会调用该函数：
person?.department?.head = managersPool.getManager()
~~~
#### Elvis 操作符
当我们有一个可空的引用 b 时，我们可以说“如果 b 非空，我使用它；否则使用某个非空的值”：
~~~ java
val l: Int = if (b != null) b.length else -1
~~~
除了完整的 if-表达式，这还可以通过 Elvis 操作符表达，写作 `?:`：
~~~ java
val l = b?.length ?: -1
~~~
如果 ?: 左侧表达式非空，elvis 操作符就返回其左侧表达式，否则返回右侧表达式。 请注意，当且仅当左侧为空时，才会对右侧表达式求值。
#### !! 操作符
第三种选择是为 NPE 爱好者准备的：非空断言运算符（`!!`）将任何值转换为非空类型，若该值为空则抛出异常。我们可以写 b!! ，这会返回一个非空的 b 值 （例如：在我们例子中的 String）或者如果 b 为空，就会抛出一个 NPE 异常：
~~~ java
val l = b!!.length
~~~
#### 安全的类型转换
如果对象不是目标类型，那么常规类型转换可能会导致 ClassCastException。 另一个选择是使用安全的类型转换，如果尝试转换不成功则返回 null：
~~~ java
val aInt: Int? = a as? Int
~~~
#### 可空类型的集合
如果你有一个可空类型元素的集合，并且想要过滤非空元素，你可以使用 filterNotNull 来实现：
~~~ java
val nullableList: List<Int?> = listOf(1, 2, null, 4)
val intList: List<Int> = nullableList.filterNotNull()
~~~

### kotlin无原始类型
在 Kotlin 中，所有东西都是对象，在这个意义上讲我们可以在任何变量上调用成员函数与属性。 一些类型可以有特殊的内部表示——例如，数字、字符以及布尔值可以在运行时表示为原生类型值，但是对于用户来说，它们看起来就像普通的类
~~~ java
val b: Byte = 1
val c:Double = b.toDouble()
~~~

### 型变
Java 类型系统中最棘手的部分之一是通配符类型（T，E，K，V，？）。 而 Kotlin 中没有。 相反，它有两个其他的东西：型变（variance）与类型投影（type projections），型变包括声明处型变和使用处型变。

首先，让我们思考为什么 Java 需要那些神秘的通配符。
在 《Effective Java》第三版 解释了该问题——第 31 条：利用有限制通配符来提升 API 的灵活性。
首先，Java 中的泛型是不型变的，这意味着 `List<String>` 并不是 `List<Object> `的子类型。 
为什么这样？ 如果 List 不是**不型变的**，它就没比 Java 的数组好到哪去，因为如下代码会通过编译然后导致运行时异常：
~~~ java
// Java
List<String> strs = new ArrayList<String>();
List<Object> objs = strs; // ！！！此处的编译器错误让我们避免了之后的运行时异常
objs.add(1); // 这里我们把一个整数放入一个字符串列表
String s = strs.get(0); // ！！！ ClassCastException：无法将整数转换为字符串
~~~
因此，Java 禁止这样的事情以保证运行时的安全。但这样会有一些影响。例如，考虑 Collection 接口中的 addAll() 方法。该方法的签名应该是什么？直觉上，我们会这样：
~~~ java
// Java
interface Collection<E> …… {
  void addAll(Collection<E> items);
}
~~~
但随后，我们就无法做到以下简单的事情（这是完全安全）
~~~ java
// Java
void copyAll(Collection<Object> to, Collection<String> from) {
  to.addAll(from);
  // ！！！对于这种简单声明的 addAll 将不能编译：
  // Collection<String> 不是 Collection<Object> 的子类型
}
~~~
（在 Java 中，我们艰难地学到了这个教训，参见《Effective Java》第三版，第 28 条：列表优先于数组）

这就是为什么 addAll() 的实际签名是以下这样：
~~~ java
// Java
interface Collection<E> …… {
  void addAll(Collection<? extends E> items);
}
~~~

**通配符类型参数 `? extends E`** 表示此方法接受 E 或者 E 的 一些子类型对象的集合，而不只是 E 自身。 这意味着我们可以安全地从其中（该集合中的元素是 E 的子类的实例）读取 E，但不能写入， 因为我们不知道什么对象符合那个未知的 E 的子类型。 反过来，该限制可以让`Collection<String>`表示为`Collection<? extends Object>`的子类型。 简而言之，带 **extends** 限定（上界）的通配符类型使得类型是协变的（covariant）。

理解为什么这个技巧能够工作的关键相当简单：如果只能从集合中获取元素，那么使用 String 的集合， 并且从其中读取 Object 也没问题 。反过来，如果只能向集合中 放入 元素，就可以用 Object 集合并向其中放入 String：在 Java 中有 `List<? super String>` 是 `List<Object>` 的一个超类。

后者称为** 逆变性（contravariance）**，并且对于 `List <? super String>` 你只能调用接受 String 作为参数的方法 （例如，你可以调用 add(String) 或者 set(int, String)），当然如果调用函数返回 `List<T>` 中的 T，你得到的并非一个 String 而是一个 Object。

看一个关于java通配符集合的经典图：

![java wildcards collection](/images/blogimages/2020/java_collections_wildcards.png)

Joshua Bloch 称那些你只能从中读取的对象为生产者，并称那些你只能写入的对象为消费者。他建议：“为了灵活性最大化，在表示生产者或消费者的输入参数上使用通配符类型”，并提出了以下助记符：

> PECS 代表生产者-Extends、消费者-Super（Producer-Extends, Consumer-Super）。

注意：如果你使用一个生产者对象，如 List<? extends Foo>，在该对象上不允许调用 add() 或 set()。但这并不意味着该对象是不可变的：例如，没有什么阻止你调用 clear()从列表中删除所有元素，因为 clear() 根本无需任何参数。通配符（或其他类型的型变）保证的唯一的事情是类型安全。不可变性完全是另一回事。

#### Kotlin 中的 Java 泛型
Kotlin 的泛型与 Java 有点不同。当将 Java 类型导入 Kotlin 时，我们会执行一些转换：

> 消费者 in, 生产者 out! :-)

- Java 的通配符转换成**类型投影**
    - Foo<? extends Bar> 转换成 `Foo<out Bar!>!`
    - Foo<? super Bar> 转换成 `Foo<in Bar!>!`
- Java的原始类型转换成**星投影**
    - List 转换成 List<*>!，即 `List<out Any?>!`

如果泛型类型具有多个类型参数，则每个类型参数都可以单独投影。 例如，如果类型被声明为 `interface Function <in T, out U>`，我们可以想象以下星投影：

- `Function<*, String>` 表示 `Function<in Nothing, String>`；
- `Function<Int, *>` 表示 `Function<Int, out Any?>`；
- `Function<*, *>` 表示 `Function<in Nothing, out Any?>`。

注意：星投影非常像 Java 的原始类型，但是安全。

和 Java 一样，Kotlin 在运行时不保留泛型，即对象不携带传递到他们构造器中的那些类型参数的实际类型。 即 `ArrayList<Integer>()` 和 `ArrayList<Character>()` 是不能区分的。 这使得执行 is-检测不可能照顾到泛型。 Kotlin 只允许 is-检测星投影的泛型类型：
~~~ java
if (a is List<Int>) // 错误：无法检测它是否真的是一个 Int 列表
// but
if (a is List<*>) // OK：不保证列表的内容
~~~

#### 类型投影的使用处型变
~~~ java
fun copy(from: Array<out Any>, to: Array<Any>) { …… }
~~~
简单介绍下，这里 out 修饰的数组只能调用 get()，对应于 Java 的 Array<? extends Object>。
~~~ java
fun fill(dest: Array<in String>, value: String) { …… }
~~~
`Array<in String>` 对应于 Java 的 Array<? super String>，也就是说，你可以传递一个 CharSequence 数组或一个 Object 数组给 fill() 函数。

#### Kotlin 中数组是不型变的

Kotlin 中数组是不型变的（invariant）。这意味着 Kotlin 不让我们把 `Array<String>`赋值给 `Array<Any>`，以防止可能的运行时失败。

> 但是你可以使用 `Array<out Any>`

#### 创建数组
- arrayOf() 函数
我们可以使用库函数 arrayOf() 来创建一个数组并传递元素值给它，这样 arrayOf(1, 2, 3) 创建了 array [1, 2, 3]。
~~~ java
val a = arrayOf("Hello", "world")
// 这里，arrayOf() 函数创建了一个 Array<String> 对象

val arr = arrayOf("aaa", "bbb", 123, true) // Array<Any>
println(arr[2])
~~~
- emptyArray() 函数
~~~ java
// emptyArray() 函数是创建一个长度为 0 的 null 数组，然后将它强制转换为指定类型的数组。
// 所以我们可以用 arrayOfNulls<T>(0) 替代 emptyArray() 函数
// 因为 emptyArray() 函数需要转换为特定的数组类型，所以定义变量需要指定数组类型，否则出现编译错误
val emptyArray1: Array<String?> = emptyArray()
// 也可以写成 val emptyArray2: Array<String?> = arrayOfNulls(0)，这里是类型推断
val emptyArray2 = arrayOfNulls<String?>(0)
println(Arrays.equals(emptyArray1, emptyArray2)) // 输出 true
~~~
- arrayOfNulls() 函数
~~~ java
val fixedSizeArray = arrayOfNulls<Int>(5)
// 直接指定长度，返回一个长度指定、所有值都是 null 的数组。
~~~
- 不使用 Array 类, 使用装箱操作比如, 对于 Int 类型数组, 使用 IntArray 类替代 Array<Int> 类：
~~~ java
val arr = arrayOf(1, 2, 3)
val intArr = intArrayOf(1, 2, 3)    //同理还有 booleanArrayOf() 等
~~~
#### 声明处型变
假设有一个泛型接口 `Source<T>`，该接口中不存在任何以 T 作为参数的方法，只是方法返回 T 类型值：
~~~ java
// Java
interface Source<T> {
  T nextT();
}
~~~
那么，在 `Source <Object>` 类型的变量中存储 `Source <String> `实例的引用是极为安全的——没有消费者-方法可以调用。但是 Java 并不知道这一点，并且仍然禁止这样操作：
~~~ java
// Java
void demo(Source<String> strs) {
  Source<Object> objects = strs; // ！！！在 Java 中不允许
  // ……
}
~~~
为了修正这一点，我们必须声明对象的类型为 Source<? extends Object>，这是毫无意义的，因为我们可以像以前一样在该对象上调用所有相同的方法，所以更复杂的类型并没有带来价值。但编译器并不知道。

在 Kotlin 中，有一种方法向编译器解释这种情况。这称为**声明处型变**：我们可以标注 Source 的类型参数 T 来确保它仅从 `Source<T>` 成员中返回（生产），并从不被消费。 为此，我们提供 out 修饰符：
~~~ java
interface Source<out T> {
    fun nextT(): T
}
​
fun demo(strs: Source<String>) {
    val objects: Source<Any> = strs // 这个没问题，因为 T 是一个 out-参数
    // ……
}
~~~
一般原则是：当一个类 C 的类型参数 T 被声明为 out 时，它就只能出现在 C 的成员的输出-位置，但回报是 `C<Base>` 可以安全地作为 `C<Derived>`的超类。

简而言之，他们说类 C 是在参数 T 上是协变的，或者说 T 是一个协变的类型参数。 你可以认为 C 是 T 的生产者，而不是 T 的消费者。

这里，out 由于它在类型参数声明处提供，所以我们称之为声明处型变.
### Kotlin与java8的 SAM 转换对比
只有一个抽象方法的接口称为函数式接口或 SAM（Single Abstract Method 单一抽象方法）接口。函数式接口可以有多个非抽象成员，但只能有一个抽象成员。
SAM 实际上这是java8中提出的概念，你就把他理解为是只有一个抽象方法的接口的就可以了

可以用 fun 修饰符在 Kotlin 中声明一个函数式接口。
~~~ java
fun interface KRunnable {
   fun invoke()
}
~~~

#### SAM 转换
对于函数式接口，可以通过 lambda 表达式实现 SAM 转换，从而使代码更简洁、更有可读性。

使用 lambda 表达式可以替代手动创建实现函数式接口的类。 通过 SAM 转换， Kotlin 可以将其签名与接口的单个抽象方法的签名匹配的任何 lambda 表达式转换为实现该接口的类的实例。
例如，有这样一个 Kotlin 函数式接口：
~~~ java
fun interface IntPredicate {
   fun accept(i: Int): Boolean
}
~~~
如果不使用 SAM 转换，那么你需要像这样编写代码：
~~~ java
// 创建一个类的实例
val isEven = object : IntPredicate {
   override fun accept(i: Int): Boolean {
       return i % 2 == 0
   }
}
~~~
通过利用 Kotlin 的 SAM 转换，可以改为以下等效代码：
~~~ java
// 通过 lambda 表达式创建一个实例
val isEven = IntPredicate { it % 2 == 0 }
~~~
可以通过更短的 lambda 表达式替换所有不必要的代码。

~~~ java
fun interface IntPredicate {
   fun accept(i: Int): Boolean
}
​
val isEven = IntPredicate { it % 2 == 0 }
​
fun main() {
   println("Is 7 even? - ${isEven.accept(7)}")
}
~~~

再看一个开发中用到的代码：
~~~ java
ExecutorService executorService= Executors.newScheduledThreadPool(3);

    executorService.execute(new Runnable() {
        @Override
        public void run() {
            System.out.println("hello world");
        }
    });
~~~

用下面的java8中的lambda 来写 也是可以的。
~~~ java
executorService.execute(()->System.out.println("hello world"));
~~~

kotlin中的lambda 这里可以这么写
~~~ java
val executorService: ExecutorService = Executors.newScheduledThreadPool(3)
//kotlin中的 匿名内部类的标准写法
executorService.submit(object :Runnable{ // object 作为关键字，这里代表对象表示
    override fun run() {
        System.out.println("hello world")
    }
})
// 使用lambda表达式的精简写法
executorService.submit { System.out.println("hello world") }
~~~
这个kotlin的lambda的类型就是 `()->Unit` 是一个没有参数也没有返回值的类型

kotlin编译器遇到上面的代码，实际上是帮我们生成了一个函数：
~~~ java
fun Runnable(block:() -> Unit):Runnable {
    return object: Runnable {
        override fun run() {
            block()
        }
    }
}
~~~
这个Runnable函数的意思是接收一个方法，别名是 block，返回一个Runnable对象，并在Runnable对象的run方法中，调用 block 方法。这里的 `()->Unit` 是一个kotlin函数类型

#### 函数类型
Kotlin 使用类似 `(Int) -> String` 的一系列函数类型来处理函数的声明.

这些类型具有与函数签名相对应的特殊表示法，即它们的参数和返回值：

- 所有函数类型都有一个圆括号括起来的参数类型列表以及一个返回类型：`(A, B) -> C`表示接受类型分别为 A 与 B 两个参数并返回一个 C 类型值的函数类型。 参数类型列表可以为空，如 () -> A。Unit 返回类型不可省略。

- 函数类型可以有一个额外的接收者类型，它在表示法中的点之前指定： 类型 `A.(B) -> C` 表示可以在 A 的接收者对象上以一个 B 类型参数来调用并返回一个 C 类型值的函数。 带有接收者的函数字面值通常与这些类型一起使用。 

- 挂起函数属于特殊种类的函数类型，它的表示法中有一个 suspend 修饰符 ，例如 `suspend () -> Unit `或者 `suspend A.(B) -> C`。

函数类型表示法可以选择性地包含函数的参数名：`(x: Int, y: Int) -> Point`。 这些名称可用于表明参数的含义。
- 如需将函数类型指定为可空，请使用圆括号：`((Int, Int) -> Int)?`。
- 函数类型可以使用圆括号进行接合：`(Int) -> ((Int) -> Unit)`
- 箭头表示法是右结合的，`(Int) -> (Int) -> Unit` 与前述示例等价，但不等于 `((Int) -> (Int)) -> Unit`。

还可以通过使用类型别名给函数类型起一个别称：
- typealias ClickHandler = (Button, ClickEvent) -> Unit

#### 函数类型实例化及调用

- 使用 lambda 表达式或匿名函数进行实例化
~~~ java
var onItemClick:(Int, String) -> Unit = { a:Int, b:String ->
    Log.e("AAA", "aaa$a$b") // 配合lambda表达式
}
onItemClick.invoke(1, "2")
onItemClick(1, "2") // 这两种方式都可以

val f:(Int, String) -> String = fun(a:Int, b:String):String {
    return "aaa$a$b"  // 使用匿名函数进行实例化
}

f.invoke(1, "2")
~~~
- 使用实现函数类型接口的自定义类的实例。下例中可直接使用 Test()(111) 
~~~ java
private class Test:(Int)->String{
    override fun invoke(p1: Int): String = "$p1 xxxx"
}
~~~
- 函数引用及属性引用
~~~ java
val intPlus: Int.(Int) -> Int = Int::plus
// 下面三种方式都可以使用
intPlus.invoke(1, 1)
intPlus(1, 2)
2.intPlus(3)
~~~

>  `::` 表示创建一个函数成员引用或者一个类引用

- 函数引用
~~~ java
fun isOdd(x: Int) = x % 2 != 0
~~~
我们可以很容易地直接调用它（isOdd(5)），但是我们也可以将其作为一个函数类型的值，例如将其传给另一个函数。为此，我们使用 :: 操作符：
~~~ java
val numbers = listOf(1, 2, 3)
println(numbers.filter(::isOdd))
~~~
这里 `::isOdd` 是函数类型 `(Int) -> Boolean` 的一个值。

等同于下面使用形参odd的方式
~~~ java
val odd:(x: Int) -> Boolean = {x:Int ->
    x % 2 != 0
}
val numbers = listOf(1, 2, 3)
println(numbers.filter(odd))
~~~
- 类引用
最基本的反射功能是获取 Kotlin 类的运行时引用。要获取对静态已知的 Kotlin 类的引用，可以使用 类字面值 语法：
~~~ java
val c = MyClass::class
~~~
该引用是 KClass 类型的值。

请注意，Kotlin 类引用与 Java 类引用不同。要获得 Java 类引用， 请在 KClass 实例上使用 `.java` 属性
~~~ java
val d = MyClass::class.java
~~~

### Kotlin中的异常
Kotlin 中所有异常类都是 Throwable 类的子孙类。 每个异常都有消息、堆栈回溯信息以及可选的原因

#### try 是一个表达式，即它可以有一个返回值：

~~~ java
val a: Int? = try { parseInt(input) } catch (e: NumberFormatException) { null }
~~~
try-表达式的返回值是 try 块中的最后一个表达式或者是（所有）catch 块中的最后一个表达式。 finally 块中的内容不会影响表达式的结果

#### 受检的异常
受检查异常要用 try-catch 捕获，要么抛出，否则会发生编译错误。而 kotlin 中没有受检查异常，所有异常都是运行时异常，即便是原本在 Java 中的受检查异常，在 kotlin 中也是运行时异常，例如：IOException 在 Java 中是受检查异常，在 kotlin 中是运行时异常。

以下是 JDK 中 StringBuilder 类实现的一个示例接口：
~~~ java
public int readNumber(BufferedReader reader) throws IOException {
    int result = 0;
    String line = reader.readLine(); // 这里要么在方法声明上抛出，要么在内部捕获处理，否则无法编译通过
    result = Integer.parseInt(line);
    reader.close();
    return result;
}
~~~
这并不好，参见《Effective Java》第三版 第 77 条：不要忽略异常。
> 通过一些小程序测试得出的结论是异常规范会同时提高开发者的生产力与代码质量，但是大型软件项目的经验表明一个不同的结论——生产力降低、代码质量很少或没有提高。

### kotlin实现静态成员的方式
在 kotin 语言中其实没有 java static 的这个概念，基本都是用一个静态对象来模拟 class 的静态属性和方法，目前有4种实现方式：

#### 伴生对象——声明单例的方式
类内部的对象声明可以用 `companion` 关键字标记，这样它就与外部类关联在一起，我们就可以直接通过外部类访问到对象的内部元素。
即可以声明属性，也可以声明方法，kotlin 中的调用方式感觉和 java 的 static 一样
~~~ java
class BookKotlin {

    var name: String = "AA"

    fun speak() {}

    companion object instance {
        var nameStatic: String = "BB"
        fun speakStatic() {}
    }
}
~~~

调用方式为：
~~~ java
BookKotlin.nameStatic
BookKotlin.speakStatic()
~~~
将kotlin代码转为java代码后，调用方式如下：
~~~ java
BookKotlin.instance.getNameStatic()
BookKotlin.instance.setNameStatic(String s)
BookKotlin.instance.speakStatic()
~~~

> 注意：一个类里面只能声明一个内部关联对象，即关键字 companion 只能使用一次。


kotlin 中所有的成员变量，不管是不是 static 的都有 get/set 方法，另外 kotlin 并没有把我们声明的静态参数和方法声明成 static 的，而是通过 instance 这个静态对象中转使用的

#### @JvmField + @JvmStatic 注解
声明成员和方法使用 JVM 提供的特性

- @JvmField - 修饰静态变量
- @JvmStatic - 修饰静态方法
- @JvmField 和 @JvmStatic 只能写在 object 修饰的类或者 companion object 里，写法虽然有些别扭，但是效果是真的是按 static 来实现的

修改一下上面代码：
~~~ java
class BookKotlin {

    companion object {

        @JvmField
        var nameStatic: String = "BB"

        @JvmStatic
        fun speakStatic() {
        }
    }

    var name: String = "AA"
    fun speak() {}
}
~~~
#### object 单例
kotlin 自身提供了一种单例实现方式：object ，直接用来修饰 class ，用 object 修饰的类不能 new 对象，只能使用 object 提供的单例
~~~ java
object BookKotlin {

    var name: String = "AA"
    fun speak() {}
}
~~~
Kotlin中使用如下：
~~~ java
BookKotlin.name
BookKotlin.speak()
~~~

#### const
const 写在 class 外面，效果 = @JvmField，但是不能修饰方法，也不能和 @JvmField 混用，一般就是用来声明常用值的，用处不多，而且只能用 val ，var 不行，想要修饰方法的话，不写 const 就行
~~~ java
const val name: String = "AA"
fun adk(){}

class BookKotlin {

    fun speak() {}
}
~~~

### kotlin没有三目运算符
用表达式`if() ... else ...` 或 `if () ... else if () ... else ...` 代替

~~~ java
var number = if(n>0) "Positive" else if(n<0) "Negative" else "Zero"
// 这一行比三元运算符简单得多，可读性也更强
~~~

### Kotlin内联函数
Java 方法执行的内存模型是基于 Java 虚拟机栈的：每个方法被执行的时候都会创建一个栈帧（Stack Frame），用于存储局部变量表、操作数栈、动态链接、方法出口等信息。每一个方法被调用直至执行完成的过程，就对应着一个栈帧入栈、出栈的过程。
也就是说每调用一个方法，都会对应一个栈帧的入栈出栈过程，如果你有一个工具类方法，在某个循环里调用很多次，那就会对应很多次的栈帧入栈、出栈过程。这里首先要记住的一点是，栈帧的创建及入栈、出栈都是有性能损耗的。下面以一个例子来说明，看段代码片段：
~~~ java
fun test() {
    //多次调用 sum() 方法进行求和运算
    println(sum(1, 2, 3))
    println(sum(100, 200, 300))
    println(sum(12, 34))
    //....可能还有若干次
}

fun sum(vararg ints: Int): Int {
    var sum = 0
    for (i in ints) {
        sum += i
    }
    return sum
}
~~~
在测试方法 test() 里，我们多次调用了 sum() 方法。为了避免多次调用 sum() 方法带来的性能损耗，我们期望的代码类似这样子的：
~~~ java
fun test() {
    var sum = 0
    for (i in arrayOf(1, 2, 3)) {
        sum += i            
    }
    println(sum)
    
    sum = 0
    for (i in arrayOf(100, 200, 300)) {
        sum += i            
    }
    println(sum)
    
    sum = 0
    for (i in arrayOf(12, 34)) {
        sum += i            
    }
    println(sum)
}
~~~
3次数据求和操作，都是在 test() 方法里执行的，没有之前的 sum() 方法调用，最后的结果依然是一样的，但是由于减少了方法调用，虽然代码量增加了，但是性能确提升了。那么怎么实现这种情况呢，一般工具类有很多公共方法，我总不能在需要调用这些公共方法的地方，把代码复制一遍吧，内联就是为了解决这一问题。

定义内联函数：
~~~ java
inline fun sum(vararg ints: Int): Int {
    var sum = 0
    for (i in ints) {
        sum += i
    }
    return sum
}
~~~
如上所示，用关键字 inline 标记函数，该函数就是一个内联函数。还是原来的 test() 方法，编译器在编译的时候，会自动把内联函数 sum() 方法体内的代码，替换到调用该方法的地方。查看编译后的字节码，会发现 test() 方法里已经没了对 sum() 方法的调用，凡是原来代码里出现 sum() 方法调用的地方，出现的都是 sum() 方法体内的字节码了。

> 注意，内联可能导致生成的代码增加；不过如果我们使用得当（即避免内联过大函数），性能上会有所提升，尤其是在循环中的“超多态（megamorphic）”调用处。

#### 禁用内联
如果一个内联函数的参数里包含 lambda表达式，也就是函数参数，那么该形参也是 inline 的，举个例子：
~~~ java
inline fun test(inlined: () -> Unit) {...}
~~~
这里有个问题需要注意，如果在内联函数的内部，函数参数被其他非内联函数调用，就会报错，如下所示：
~~~ java
//内联函数
inline fun test(inlined: () -> Unit) {
    //这里会报错
    otherNoinlineMethod(inlined)   
}

//非内联函数
fun otherNoinlineMethod(oninline: () -> Unit) {
    
}
~~~

要解决这个问题，必须为内联函数的参数加上 noinline 修饰，表示禁止内联，保留原有函数的特性，所以 test() 方法正确的写法应该是：
~~~ java
inline fun test(noinline inlined: () -> Unit) {
    otherNoinlineMethod(inlined)
}
~~~

> 注意，如果一个内联函数没有可内联的函数参数并且没有具体化的类型参数，编译器会产生一个警告，因为内联这样的函数很可能并无益处（如果你确认需要内联，则可以用 @Suppress("NOTHING_TO_INLINE") 注解关掉该警告

#### 非局部返回
在 Kotlin 中，我们只能对具名或匿名函数使用正常的、非限定的 return 来退出。 这意味着要退出一个 lambda 表达式，我们必须使用一个标签，并且在 lambda 表达式内部禁止使用裸 return，因为 lambda 表达式不能使包含它的函数返回：
~~~ java
fun test() {
    innerFun {      
        //return 会报错，非局部返回，直接退出 test() 函数。
        return@innerFun     //局部返回，只退出 innerFun() 函数
    }

    //以下代码依旧会执行
    println("test...")
}

fun innerFun(a: () -> Unit) {
    a()
}
~~~
非局部返回我的理解就是返回到顶层函数，如上面代码中所示，默认情况下是不能直接 return 的，但是内联函数确是可以的。所以改成下面这个样子
~~~ java
fun test() {
    innerFun {      
        return //非局部返回，直接退出 test() 函数。
    }
    
    //以下代码不会执行
    println("test...")
}

inline fun innerFun(a: () -> Unit) {
    a()
}
~~~

也就是说内联函数的函数参数在调用时，可以非局部返回，如上所示。那么 crossinline 修饰的 lambda 参数，可以禁止内联函数调用时非局部返回。
~~~ java
fun test() {
    innerFun {      
        return //这里这样会报错，只能 return@innerFun
    }
    
    //以下代码不会执行
    println("test...")
}

inline fun innerFun(crossinline a: () -> Unit) {
    a()
}
~~~

#### 具体化的类型参数
有时候我们需要访问一个作为参数传给我们的一个类型：
~~~ java
fun <T> TreeNode.findParentOfType(clazz: Class<T>): T? {
    var p = parent
    while (p != null && !clazz.isInstance(p)) {
        p = p.parent
    }
    @Suppress("UNCHECKED_CAST")
    return p as T?
}
~~~
在这里我们向上遍历一棵树并且检测每个节点是不是特定的类型。 这都没有问题，但是调用处不是很优雅：
~~~ java
treeNode.findParentOfType(MyTreeNode::class.java)
~~~
我们真正想要的只是传一个类型给该函数，即像这样调用它：
~~~ java
treeNode.findParentOfType<MyTreeNode>()
~~~
为能够这么做，内联函数支持具体化的类型参数，于是我们可以这样写：
~~~ java
inline fun <reified T> TreeNode.findParentOfType(): T? {
    var p = parent
    while (p != null && p !is T) {
        p = p.parent
    }
    return p as T?
}
~~~

我们使用 reified 修饰符来限定类型参数，现在可以在函数内部访问它了， 几乎就像是一个普通的类一样。由于函数是内联的，不需要反射，正常的操作符如 !is 和 as 现在都能用了。此外，我们还可以按照上面提到的方式调用它：`myTree.findParentOfType<MyTreeNodeType>()`。

再举个栗子：
~~~ java
inline fun <reified T: Activity> startActivity() {
    startActivity(Intent(this, T::class.java))
}
~~~
使用时直接传入泛型即可，代码简洁明了：
~~~ kotlin
startActivity<MainActivity>()
~~~
### 扩展函数
扩展函数可以在已有类中添加新的方法，不会对原类做修改，扩展函数定义形式：
~~~ java
fun receiverType.functionName(params){
    body
}
~~~
receiverType：表示函数的接收者，也就是函数扩展的对象
functionName：扩展函数的名称
params：扩展函数的参数，可以为NULL

以下实例扩展 User 类 ：
~~~ java
class User(var name:String)

/**扩展函数**/
fun User.Print(){
    print("用户名 $name")
}

fun main(arg:Array<String>){
    var user = User("Runoob")
    user.Print()
}
~~~

### 类的属性
Kotlin 类的属性可以声明为可变的（var）或者是只读的（val），声明一个属性的完整语法如下：
~~~ java
var <propertyName>[: <PropertyType>] [= <property_initializer>]
    [<getter>]
    [<setter>]
val <propertyName>[: <PropertyType>] [= <property_initializer>]
    [<getter>]
~~~
以一个简单的 Student 类为例：
~~~ java
class Student(_name: String, _age: Int) {
    val name: String = _name
    var age: Int = _age
    val isAdult: Boolean
        get() = this.age >= 18
}
~~~
这里声明 name 和 age 时确实是有 getter 和 setter 方法的，只是被省略了而已，我们以为的不省略写法如下（错误的）：
~~~ java
class Student(_name: String, _age: Int) {
    val name = _name // Initializer is not allowed here because this property has no backing field
        get() = this.name // Recursive call
    var age = _age // Initializer is not allowed here because this property has no backing field
        get() = this.age // Recursive call
        set(value) {
            this.age = value // Recursive call
        }
}
~~~
this.name 会造成 Recursive call（递归调用）。我们看一下是怎么回事：
- 第一步，调用 student.name；
- 第二步，调用 get() 方法；
- 第三步，在 get() 方法里，调用 this.name；
- 第一步，调用 student.name；
- 第二步，调用 get() 方法；
- ...

这样就形成了递归调用了 get() 方法，并且没有终止条件，所以是有问题的。

另一条错误信息提示没有`backing field`，也叫幕后字段

#### backing field
Kotlin 类中不能直接声明 Fields。然而，当一个属性需要一个 backing field 时，Kotlin 会自动地提供它。在访问器中使用 field 标识符就可以引用到 backing field

看下官方文档里的例子：
~~~ java
var counter = 0 // Note: the initializer assigns the backing field directly
    set(value) {
        if (value >= 0) field = value
    }
~~~
改写一下刚才的Student类：
~~~ java
class Student(_name: String, _age: Int) {
    val name = _name
        get() = field // redundant
    var age = _age
        get() = field // redundant
        set(value) { // redundant
            field = value
        }
    val isAdult: Boolean
            get() = this.age >= 18
}
~~~
#### Backing Properties
如果上面的方案都不符合你的需求，那么可以试试“后端属性”(backing property)的方法，它实际上也是隐含试的对属性值的初始化声明，避免了空指针。
~~~ java
class Student(_name: String, _age: Int) {
    val name = _name
    var age = _age
    private var _grades: Map<String, Int>? = null // 私有属性，后端属性
    val grades: Map<String, Int>
        get() {
            if (_grades == null) { // 只有在首次访问时才加载成绩信息，并只执行一次
                _grades = loadGrades()
            }
            return _grades!!
        }

    private fun loadGrades(): MutableMap<String, Int> {
        println("loadGrades() called")
        val result = mutableMapOf<String, Int>()
        result.put("Chinese", 100)
        result.put("Math", 99)
        result.put("English", 100)
        return result
    }
}
~~~

### Kotlin 独有的一些细节
#### 主构造函数
在Kotlin类中只有一个主构造函数(主构造器)，而辅助构造函数（次级构造器）可以是一个或者多个。
主构造函数用于初始化类，它在类标题中声明。标准写法：
~~~ java
class 类名 construction(参数1，参数2….){}
~~~
当constructor关键字没有注解和可见性修饰符作用于它时，constructor关键字可以省略

~~~ java
class Person {
    private val name:String
    private val age:Int
    constructor(name:String,age:Int){ // 次级构造函数
        this.name = name
        this.age = age
    }
}
~~~

#### 数据类
Kotlin 可以创建一个只包含数据的类，关键字为 data：
~~~ java
data class User(val name: String, val age: Int)
~~~

编译器会自动的从主构造函数中根据所有声明的属性提取以下函数：

- equals() / hashCode()
- toString() 格式如 "User(name=John, age=42)"
- componentN() functions 对应于属性，按声明顺序排列
- copy() 函数

为了保证生成代码的一致性以及有意义，数据类需要满足以下条件：
- 主构造函数至少包含一个参数
- 所有的主构造函数的参数必须标识为val 或者 var
- 数据类不可以声明为 abstract, open, sealed 或者 inner
- 数据类不能继承其他类 (但是可以实现接口)

**复制**
复制使用 copy() 函数，我们可以使用该函数复制对象并修改部分属性, 对于上文的 User 类，其实现会类似下面这样：
~~~ java
fun copy(name: String = this.name, age: Int = this.age) = User(name, age)
~~~
实例
使用 copy 类复制 User 数据类，并修改 age 属性:
~~~ java
data class User(val name: String, val age: Int)


fun main(args: Array<String>) {
    val jack = User(name = "Jack", age = 1)
    val olderJack = jack.copy(age = 2)
    println(jack)
    println(olderJack)
}

// 输出结果为：
// User(name=Jack, age=1)
// User(name=Jack, age=2)
~~~

#### 密封类
密封类用来表示受限的类继承结构：当一个值为有限几种的类型, 而不能有任何其他类型时。在某种意义上，他们是枚举类的扩展：枚举类型的值集合 也是受限的，但每个枚举常量只存在一个实例，而密封类 的一个子类可以有可包含状态的多个实例。

声明一个密封类，使用 sealed 修饰类，密封类可以有子类，但是所有的子类都必须要内嵌在密封类中。

sealed 不能修饰 interface ,abstract class(会报 warning,但是不会出现编译错误)

密封类就是一种专门用来配合 when 语句使用的类，举个例子，假如在 Android 中我们有一个 view，我们现在想通过 when 语句设置针对 view 进行两种操作：显示和隐藏，那么就可以这样做：
~~~ java
sealed class UiOp {
    object Show: UiOp()
    object Hide: UiOp()
}

fun execute(view: View, op: UiOp) = when (op) {
    UiOp.Show -> view.visibility = View.VISIBLE
    UiOp.Hide -> view.visibility = View.GONE
}
~~~
以上功能其实完全可以用枚举实现，但是如果我们现在想加两个操作：水平平移和纵向平移，并且还要携带一些数据，比如平移了多少距离，平移过程的动画类型等数据，用枚举显然就不太好办了，这时密封类的优势就可以发挥了，例如：
~~~ java
sealed class UiOp {
    object Show: UiOp()
    object Hide: UiOp()
    class TranslateX(val px: Float): UiOp()
    class TranslateY(val px: Float): UiOp()
}

fun execute(view: View, op: UiOp) = when (op) {
    UiOp.Show -> view.visibility = View.VISIBLE
    UiOp.Hide -> view.visibility = View.GONE
    is UiOp.TranslateX -> view.translationX = op.px // 这个 when 语句分支不仅告诉 view 要水平移动，还告诉 view 需要移动多少距离，这是枚举等 Java 传统思想不容易实现的
    is UiOp.TranslateY -> view.translationY = op.px
    // 不再需要 `else` 子句，因为我们已经覆盖了所有的情况
}
~~~
以上代码中，TranslateX 是一个类，它可以携带多于一个的信息，比如除了告诉 view 需要水平平移之外，还可以告诉 view 平移多少像素，甚至还可以告诉 view 平移的动画类型等信息，我想这大概就是密封类出现的意义吧。

除此之外，如果 when 语句的分支不需要携带除“显示或隐藏view之外的其它信息”时（即只需要表明 when 语句分支，不需要携带额外数据时），用 object 关键字创建单例就可以了，并且此时 when 子句不需要使用 is 关键字。只有需要携带额外信息时才定义密封类的子类，而且使用了密封类就不需要使用 else 子句，每当我们多增加一个密封类的子类或单例，编译器就会在 when 语句中给出提示，可以在编译阶段就及时发现错误，这也是以往 switch-case 语句和枚举不具备的功能。

最后，我们甚至可以把这一组操作封装成一个函数，以便日后调用，如下：
~~~ java
// 先封装一个UI操作列表
class Ui(val uiOps: List = emptyList()) {
    operator fun plus(uiOp: UiOp) = Ui(uiOps + uiOp)
}

// 定义一组操作
val ui = Ui() +
        UiOp.Show +
        UiOp.TranslateX(20f) +
        UiOp.TranslateY(40f) +
        UiOp.Hide
// 定义调用的函数
fun run(view: View, ui: Ui) {
    ui.uiOps.forEach { execute(view, it) }
}

run(view, ui) // 最终调用
~~~

#### Break 和 Continue 标签
在 Kotlin 中任何表达式都可以用标签（label）来标记。 标签的格式为标识符后跟 @ 符号，例如：abc@、fooBar@都是有效的标签。 要为一个表达式加标签，我们只要在其前加标签即可。
~~~ java
loop@ for (i in 1..100) {
    for (j in 1..100) {
        if (……) break@loop
    }
}
~~~
标签限制的 break 跳转到刚好位于该标签指定的循环后面的执行点。 continue 继续标签指定的循环的下一次迭代。

#### 标签处返回(待补充)
Kotlin 有函数字面量、局部函数和对象表达式。因此 Kotlin 的函数可以被嵌套。 标签限制的 return 允许我们从外层函数返回。 最重要的一个用途就是从 lambda 表达式中返回。回想一下我们这么写的时候：
~~~ java
fun foo() {
    ints.forEach {
        if (it == 0) return
        print(it)
    }
}
~~~
这个 return 表达式从最直接包围它的函数即 foo 中返回。 （注意，这种非局部的返回只支持传给内联函数的 lambda 表达式。） 如果我们需要从 lambda 表达式中返回，我们必须给它加标签并用以限制 return。

~~~ java
fun foo() {
    ints.forEach lit@ {
        if (it == 0) return@lit
        print(it)
    }
}
~~~

现在，它只会从 lambda 表达式中返回。通常情况下使用隐式标签更方便。 该标签与接受该 lambda 的函数同名。

~~~ java
fun foo() {
    ints.forEach {
        if (it == 0) return@forEach
        print(it)
    }
}
~~~

#### object 关键字
`object` 是Kotlin中的一个重要的关键字，也是Java中没有的。`object` 主要有以下三种使用场景：

- 对象声明（Object Declaration），前面提到了，用于创建类的单例
- 伴生对象（Companion Object），类似java中 static 关键字
- 对象表达式（Object Expression），替换java的匿名内部类

#### 分离用于只读与可变集合的接口


#### 委托
参考 [一文彻底搞懂 Kotlin 中的委托](https://juejin.im/post/6844904038589267982)

#### 协程（coroutines）
参考谷歌官方文档：[Android 上的 Kotlin 协程](https://developer.android.com/kotlin/coroutines?hl=zh-cn)

### Ref
[Kotlin 语言中文站](https://www.kotlincn.net/docs/reference/)
[Kotlin内联函数:](https://juejin.im/entry/6844903879751008270)
[使用 Kotlin 进行 Android 开发](https://www.kotlincn.net/docs/reference/android-overview.html)
[Kotlin 的 Backing Fields 和 Backing Properties](https://blog.csdn.net/willway_wang/article/details/100184784)