---
layout: post
title:  JAVA泛型通配符T，E，K，V区别，T以及Class<T>，Class<?>的区别
category: accumulation
tags:
  - Java
  - genericity
keywords: Java, 泛型
banner: http://obxk8w81b.bkt.clouddn.com/Basket%20of%20Apples.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Basket%20of%20Apples.jpg
toc: true
---
> 文章转载自[《JAVA泛型通配符T，E，K，V区别，T以及Class<T>，Class<?>的区别》](http://www.jianshu.com/p/95f349258afb)

### 1. 先解释下泛型概念

泛型是Java SE 1.5的新特性，泛型的本质是参数化类型，也就是说所操作的数据类型被指定为一个参数。这种参数类型可以用在类、接口和方法的创建中，分别称为泛型类、泛型接口、泛型方法。Java语言引入泛型的好处是安全简单。

在Java SE 1.5之前，没有泛型的情况的下，通过对类型Object的引用来实现参数的“任意化”，“任意化”带来的缺点是要做显式的强制类型转换，而这种转换是要求开发者对实际参数类型可以预知的情况下进行的。对于强制类型转换错误的情况，编译器可能不提示错误，在运行的时候才出现异常，这是一个安全隐患。

<!--more-->

泛型的好处是在编译的时候检查类型安全，并且所有的强制转换都是自动和隐式的，以提高代码的重用率。

      以上内容摘自百度百科

举个栗子:
Box类定义为一个泛型类
~~~ Java
public class Box<T> {
    private T object;

    public void set(T object) { this.object = object; }
    public T get() { return object; }
}
~~~
创建一个Box对象，不带泛型参数，发现获取对象的时候需要强制转换

~~~ Java
Box box2 = new Box();
box2.set(new Apple());
Apple apple = (Apple) box2.get();
~~~
创建一个Box对象，带泛型参数，获取对象的时候就不需要强制转换

~~~ Java
Box<Apple> box = new Box<Apple>();
box.set(new Apple());
Apple apple = box.get();
~~~
总结下泛型的好处就是：

**省去了强制转换，可以在编译时候检查类型安全，可以用在类，方法，接口上**

但是我们定义泛型类，泛型方法，泛型接口的时候经常会碰见很多不同的通配符T，E，K，V等等，这些通配符又都是什么意思呢？继续往下看

### 2. 下来说说泛型通配符T，E，K，V区别

这些全都属于java泛型的通配符，刚开始我看到这么多通配符，一下晕了，这几个其实没什么区别，只不过是一个约定好的代码，也就是说
使用大写字母A,B,C,D......X,Y,Z定义的，就都是泛型，把T换成A也一样，这里T只是名字上的意义而已

- ？ 表示不确定的java类型
- T (type) 表示具体的一个java类型
- K V (key value) 分别代表java键值中的Key Value
- E (element) 代表Element

举个栗子：
~~~ Java
public class Test<T> {    
  public List<T> list = new ArrayList<T>();   
  public static void main(String[] args) {
        Test<String> test = new Test<String>();
        test.list.add("hello");
        System.out.println(test.list);
}}
~~~
和
~~~ Java
public class Test<A> {    
  public List<A> list = new ArrayList<A>();   
  public static void main(String[] args) {
        Test<String> test = new Test<String>();
        test.list.add("hello");
        System.out.println(test.list);
}}
~~~
将T换成了A，在执行效果上是没有任何区别的，只不过我们约定好了T代表type，所以还是按照约定规范来比较好，增加了代码的可读性。

如果要定义多个泛型参数，比如说两个泛型参数，很典型的一个栗子是Map的key,value泛型，我们也可以定义一个这样的
~~~ Java
public interface Mymap<K, V> {
    public K getKey();
    public V getValue();
}

public class MymapImpl<K, V> implements Mymap<K, V> {

    private K key;
    private V value;

    public MymapImpl(K key, V value) {
    this.key = key;
    this.value = value;
    }

    public K getKey()    { return key; }
    public V getValue() { return value; }
}
~~~
下来就可以传入任意类型，创建实例了，不用转化类型
~~~ Java
Mymap<String, Integer> mp1= new MymapImpl<String, Integer>("Even", 8);
Mymap<String, String>  mp2= new MymapImpl<String, String>("hello", "world");
Mymap<Integer, Integer> mp3= new MymapImpl<Integer, Integer>(888, 888);
~~~
如果要定义超过两个，三个或三个以上的泛型参数可以使用T1, T2, ..., Tn，像这样子
~~~ Java
public class Test<T1,T2,T3> {
   public void print(T1 t1,T2 t2,T3 t3){
        System.out.println(t1.getClass());
        System.out.println(t2.getClass());
        System.out.println(t3.getClass());
    }
}
~~~
### 3. 下来说说T，Class<T>，Class<?>区别

T是一种具体的类，例如String,List,Map......等等，这些都是属于具体的类，这个比较好理解

Class是什么呢，Class也是一个类，但Class是存放上面String,List,Map......类信息的一个类，有点抽象，我们一步一步来看 。

如何获取到Class类呢，有三种方式：

- 1. 调用Object类的getClass()方法来得到Class对象，这也是最常见的产生Class对象的方法。例如：
~~~ Java
List list = null;
Class clazz = list.getClass();
~~~

- 2. 使用Class类的中静态forName()方法获得与字符串对应的Class对象。例如：

~~~ Java
Class clazz = Class.forName("com.lyang.demo.fanxing.People");
~~~

- 3.获取Class类型对象的第三个方法非常简单。如果T是一个Java类型，那么T.class就代表了匹配的类对象。
~~~ Java
Class clazz = List.class;
~~~
> 那么问题来了？Class类是创建出来了，但是Class<T>和Class<?>适用于什么时候呢？？？

使用Class<T>和Class<?>多发生在**反射**场景下，先看看如果我们不使用泛型，反射创建一个类是什么样的。
~~~ Java
People people = (People) Class.forName("com.lyang.demo.fanxing.People").newInstance();
~~~
看到了么，需要强转，如果反射的类型不是People类，就会报java.lang.ClassCastException错误。

使用Class<T>泛型后，不用强转了
~~~ Java
public class Test {
    public static <T> T createInstance(Class<T> clazz) throws IllegalAccessException, InstantiationException {
        return clazz.newInstance();
    }

    public static void main(String[] args)  throws IllegalAccessException, InstantiationException  {
            Fruit fruit= createInstance(Fruit .class);
            People people= createInstance(People.class);
    }
}
~~~
> 那Class<T>和Class<?>有什么区别呢？

- Class<T>在实例化的时候，T要替换成具体类
- Class<?>它是个通配泛型，?可以代表任何类型，主要用于声明时的限制情况

例如可以声明一个
~~~ Java
public Class<?> clazz;
~~~
但是你不能声明一个
~~~ Java
public Class<T> clazz;
~~~
因为T需要指定类型

所以当不知道定声明什么类型的Class的时候可以定义一个Class<?>,Class<?>可以用于参数类型定义，方法返回值定义等。
