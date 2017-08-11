---
layout: post
title:  ThreadLocal一个使用——SimpleDateFormat
category: accumulation
tags: ThreadLocal
keywords: ThreadLocal
banner: http://obxk8w81b.bkt.clouddn.com/A%20Wind-Beaten%20Tree.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/A%20Wind-Beaten%20Tree.jpg
toc: true
---

### 背景

之前写过一篇文章：[《ThreadLocal、HandlerThread、Lopper区别》](https://agehua.github.io/2016/09/05/ThreadLocal-HandlerThread-Lopper/)，其中简单提及了ThreadLocal的解释。本篇文章主要结合具体应用场景——SimpleDateFormat，和大家来一起学习ThreadLocal的原理，并对ThreadLocal进行一个详细的介绍。

下面是本文的参考资料：

java学习记录–ThreadLocal使用案例——SimpleDateFormat：
http://blog.csdn.net/u012706811/article/details/53231598

SimpleDateFormat的线程安全问题与解决方案：
http://www.cnblogs.com/zemliu/archive/2013/08/29/3290585.html


### 前言
Thread这个类有一个变量：ThreadLocal.ThreadLocalMap threadLocals。

这是一个map的数据结构，里面的元素的key就是ThreadLocal，value就是我们自定义的一些目标类。

我们可以在自己的多线程类中定义好几个ThreadLocal，然后每一个ThreadLocal put一个特定的目标类，然后以后可以用ThreadLocal get到目标类（ThreadLocal用自己作为Thread里map的key），因为每个Thread有自己独自的map，所以这样可以实现每个线程有自己的LocalThread，并且一个Thread里可以有多个LocalThread。

ThreadLocal为变量在每个线程中都创建了一个副本，所以每个线程可以访问自己内部的副本变量，不同线程之间不会互相干扰。

本篇文章结合具体应用场景，来分析ThreadLocal为什么可以实现不同线程之间不会互相干扰。

<!--more-->

### 应用场景
那么，在什么场景下比较适合使用ThreadLocal呢？

stackoverflow上有人给出了还不错的回答：

> [When and how should I use a ThreadLocal variable?](http://stackoverflow.com/questions/817856/when-and-how-should-i-use-a-threadlocal-variable)
One possible (and common) use is when you have some object that is not thread-safe, but you want to avoid synchronizing access to that object (I'm looking at you, SimpleDateFormat). Instead, give each thread its own instance of the object.

#### SimpleDateFormat为什么线程不安全
SimpleDateFormat类内部持有一个Calendar对象引用，

如果你的工具类里，SimpleDateFormat是个static的，那么多个thread之间就会共享这个SimpleDateFormat，同时也会共享这个Calendar引用。

查看下源码中的SimpleDateFormat.parse()方法，你会发现有如下的调用:

~~~ Java
protected Calendar calendar;

Date parse() {

  calendar.clear(); // 清理calendar

  ... // 执行一些操作, 设置 calendar 的日期什么的

  calendar.getTime(); // 获取calendar的时间
}
~~~

> 这里calendar.clear()方法是线程不安全的，不同thread调用parse()方法，会导致结果不可预期

问题重现可以看：[这篇博客](http://www.cnblogs.com/zemliu/archive/2013/08/29/3290585.html)


#### 解决方案

最简单的解决方案我们可以把static去掉，这样每个新的线程都会有一个自己的SimpleDateFormat实例，从而避免线程安全的问题

但是，使用这种方法，在高并发的情况下会大量的new SimpleDateFormat以及销毁SimpleDateFormat，这样是非常耗费资源的

下面是一个从网上找的使用ThreadLocal解决SimpleDateFormat线程不安全问题的例子：

~~~ Java
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;

public class DateUtil {

    /** 锁对象 */
    private static final Object lockObj = new Object();

    /** 存放不同的日期模板格式的SimpleDateFormat的Map */
    private static Map<String, ThreadLocal<SimpleDateFormat>> sdfMap = new HashMap<String, ThreadLocal<SimpleDateFormat>>();

    /**
     * 返回一个ThreadLocal的SimpleDateFormat,每个线程只会new一次SimpleDateFormat
     *
     * @param pattern
     * @return
     */
    private static SimpleDateFormat getSdf(final String pattern) {
        ThreadLocal<SimpleDateFormat> tl = sdfMap.get(pattern);

        // 此处的双重判断和同步是为了防止sdfMap这个单例被多次put重复的sdf
        if (tl == null) {
            synchronized (lockObj) {
                tl = sdfMap.get(pattern);
                if (tl == null) {
                    // 只有Map中还没有这个pattern的sdf才会生成新的sdf并放入map
                    System.out.println("put new sdf of pattern " + pattern + " to map");

                    // 这里是关键,使用ThreadLocal<SimpleDateFormat>替代原来直接new SimpleDateFormat
                    tl = new ThreadLocal<SimpleDateFormat>() {

                        @Override
                        protected SimpleDateFormat initialValue() {
                            System.out.println("thread: " + Thread.currentThread() + " init pattern: " + pattern);
                            return new SimpleDateFormat(pattern);
                        }
                    };
                    sdfMap.put(pattern, tl);
                }
            }
        }

        return tl.get();
    }

    /**
     * 是用ThreadLocal<SimpleDateFormat>来获取SimpleDateFormat,这样每个线程只会有一个SimpleDateFormat
     *
     * @param date
     * @param pattern
     * @return
     */
    public static String format(Date date, String pattern) {
        return getSdf(pattern).format(date);
    }

    public static Date parse(String dateStr, String pattern) throws ParseException {
        return getSdf(pattern).parse(dateStr);
    }

}
~~~

### 实现原理
那为什么ThreadLocal为变量在每个线程都保存了一个副本呢？

        下面部分内容来自博文，http://www.jianshu.com/p/33c5579ef44f (深入浅出ThreadLocal)

先看一下Thread和ThreadLocal两个类的具体结构，如下：

![ThreadLocal](http://oui2w5whj.bkt.clouddn.com/blogimages/2016/ThreadLocal.png)

从线程Thread的角度来看，每个线程内部都会持有一个对ThreadLocalMap实例的引用（也就是threadLocals变量），ThreadLocalMap是TThreadLocal的静态内部类。而Entry则是ThreadLocalMap的静态内部类。ThreadLocalMap实例相当于线程的局部变量空间，由Entry[] table存储着线程的私有数据。

#### Entry
Entry继承自WeakReference类，是存储线程私有变量的数据结构。ThreadLocal实例作为引用，意味着如果ThreadLocal实例为null，就可以从table中删除对应的Entry。

~~~ Java
class Entry extends WeakReference<ThreadLocal<?>> {
      Object value;
      Entry(ThreadLocal<?> k, Object v) {
           super(k);
           value = v;
      }
}
~~~
#### ThreadLocalMap
内部使用table数组存储Entry，默认大小INITIAL_CAPACITY(16)，先介绍几个参数：

- **size**：table中元素的数量。
- **threshold**：table大小的2/3，当size >= threshold时，遍历table并删除key为null的元素，如果删除后size >= threshold*3/4时，需要对table进行扩容。

#### ThreadLocal.set()实现

~~~ Java
/**
 * ThreadLocal.set()
 */
public void set(T value) {
    Thread t = Thread.currentThread();
    ThreadLocalMap map = getMap(t);
    if (map != null)
        map.set(this, value);
    else
        createMap(t, value);
}

ThreadLocalMap getMap(Thread t) {
    return t.threadLocals;
}
~~~
从上面代码中看出来：

- 1.从当前线程Thread中获取ThreadLocalMap实例。
- 2.ThreadLocal实例和value封装成Entry。


#### ThreadLocal.get() 实现
~~~ Java
public T get() {
    Thread t = Thread.currentThread();
    ThreadLocalMap map = getMap(t);
    if (map != null) {
        ThreadLocalMap.Entry e = map.getEntry(this);
        if (e != null) {
            @SuppressWarnings("unchecked")
            T result = (T)e.value;
            return result;
        }
    }
    return setInitialValue();
}

private T setInitialValue() {
        T value = initialValue();
        Thread t = Thread.currentThread();
        ThreadLocalMap map = getMap(t);
        if (map != null)
            map.set(this, value);
        else
            createMap(t, value);
        return value;
}

ThreadLocalMap getMap(Thread t) {
    return t.threadLocals;
}
~~~

从上面代码中可以看出，
> ThreadLocal.get()方法，如果是不同线程ThreadLocalMap为null，最终会调用**initialValue()方法**，这个方法也是我们给出的解决方案中重载的方法。
最后得到的结果是，不同线程ThreadLocal只有一个，而SimpleDateFormat则每个线程都有一个。


但当同一线程有了一个SimpleDateFormat了，会是什么情况呢？看下面代码：

~~~ Java
//同一线程中再次执行ThreadLocal.get()
public T get() {
    Thread t = Thread.currentThread();//得到当前子线程
    ThreadLocalMap map = getMap(t);
    //同一个线程，map不为空
    if (map != null) {
        ThreadLocalMap.Entry e = map.getEntry(this);
        if (e != null) {
            @SuppressWarnings("unchecked")
            T result = (T)e.value;
            return result;
        }
    }
    return setInitialValue();
}

ThreadLocalMap getMap(Thread t) {
    //同一线程Thread，变量threadLocals经过初始化了，则不为空
    return t.threadLocals;
}
~~~

ThreadLocalMap.getEntry()是如何实现的呢？先看下Entry是如何存入table数组的

#### Entry存入table数组如何实现的：

~~~ Java
/**
 * static class ThreadLocalMap.set()方法
 */
private void set(ThreadLocal<?> key, Object value) {
    Entry[] tab = table;
    int len = tab.length;
    int i = key.threadLocalHashCode & (len-1);

    for (Entry e = tab[i]; e != null; e = tab[i = nextIndex(i, len)]) {
        ThreadLocal<?> k = e.get();
        if (k == key) {
            e.value = value;
            return;
        }
        if (k == null) {
            replaceStaleEntry(key, value, i);
            return;
        }
    }

    tab[i] = new Entry(key, value);
    int sz = ++size;
    if (!cleanSomeSlots(i, sz) && sz >= threshold)
        rehash();
}
~~~
- 1.通过ThreadLocal的nextHashCode方法生成hash值。

threadLocalHashCode，由nextHashCode()方法得到
~~~ Java
/**
 * 对应上面代码的key.threadLocalHashCode
 * 在ThreadLocal 类中
 */
private final int threadLocalHashCode = nextHashCode();
~~~

- 2.通过 key.threadLocalHashCode & (len -1) 定位到table的位置i，假设table中i位置的元素为f。
- 3.如果f != null，假设f中的引用为k：
  - 如果k和当前ThreadLocal实例一致，则修改value值，返回。
  - 如果k为null，说明这个f已经是stale(陈旧的)的元素。调用replaceStaleEntry方法删除table中所有陈旧的元素（即entry的引用为null）并插入新元素，返回。
  - 否则通过nextIndex方法找到下一个元素f，继续进行步骤3。
- 4.如果f == null，则把Entry加入到table的i位置中。
- 5.通过cleanSomeSlots删除陈旧的元素，如果table中没有元素删除，需判断当前情况下是否要进行扩容。

~~~ Java
/**
 * ThreadLocal 类中
 */
private static AtomicInteger nextHashCode = new AtomicInteger();
private static int nextHashCode() {    
 return nextHashCode.getAndAdd(HASH_INCREMENT);
}
~~~
  从nextHashCode方法可以看出，ThreadLocal每实例化一次，其hash值就原子增加HASH_INCREMENT。

#### Entry从table数组中取出

在回看getEntry()方法，就简单多了：
~~~ Java
/**
 * ThreadLocalMap的getEntry()方法
 */
private Entry getEntry(ThreadLocal<?> key) {
    int i = key.threadLocalHashCode & (table.length - 1);
    Entry e = table[i];
    if (e != null && e.get() == key)
        return e;
    else
        return getEntryAfterMiss(key, i, e);
}
~~~
- 1.通过key.threadLocalHashCode & (len -1)定位到table的位置i，假设table中i位置的元素为g。
- 2.如果g不为null，且和当前ThreadLocal实例一致，则返回这个Entry。

**在看一遍ThreadLocal.get()的代码：**
~~~ Java
//同一线程中再次执行ThreadLocal.get()
public T get() {
    Thread t = Thread.currentThread();//得到当前子线程
    ThreadLocalMap map = getMap(t);
    //同一个线程，map不为空
    if (map != null) {
        //当前ThreadLocal实例一致，则Entry不为null
        ThreadLocalMap.Entry e = map.getEntry(this);
        if (e != null) {
            @SuppressWarnings("unchecked")
            T result = (T)e.value;
            return result;
        }
    }
    return setInitialValue();
}
~~~

> 所以，同一个线程，ThreadLocalMap不为空，Entry不为null，则返回table中保存的result。不会执行到setInitialValue()。所以，而SimpleDateFormat则会每一个线程只有一个。

由此，就可以得出结论了，确实可以使用ThreadLocal解决SimpleDateFormat的线程安全问题。


    p.s. 感兴趣的可以结合ThreadLocal源码继续研究下面的内容：





#### 获取当前的线程的threadLocals。

- 如果threadLocals不为null，则通过ThreadLocalMap.getEntry方法找到对应的entry，如果其引用和当前key一致，则直接返回，否则在table剩下的元素中继续匹配。
- 如果threadLocals为null，则通过setInitialValue方法初始化，并返回。
~~~ Java
private Entry getEntryAfterMiss(ThreadLocal<?> key, int i, Entry e) {
 Entry[] tab = table;
 int len = tab.length;
 while (e != null) {
     ThreadLocal<?> k = e.get();
     if (k == key)
         return e;
     if (k == null)
         expungeStaleEntry(i);
     else
         i = nextIndex(i, len);
     e = tab[i];
 }
 return null;
}
~~~

#### table扩容
如果table中的元素数量达到阈值threshold的3/4，会进行扩容操作，过程很简单：

~~~ Java
/**
 * 在ThreadLocalMap类中
 */
private void resize() {
    Entry[] oldTab = table;
    int oldLen = oldTab.length;
    int newLen = oldLen * 2;
    Entry[] newTab = new Entry[newLen];
    int count = 0;

    for (int j = 0; j < oldLen; ++j) {
        Entry e = oldTab[j];
        if (e != null) {
            ThreadLocal<?> k = e.get();
            if (k == null) {
                e.value = null; // Help the GC
            } else {
                int h = k.threadLocalHashCode & (newLen - 1);
                while (newTab[h] != null)
                    h = nextIndex(h, newLen);
                newTab[h] = e;
                count++;
            }
        }
    }

    setThreshold(newLen);
    size = count;
    table = newTab;
}
~~~
- 新建新的数组newTab，大小为原来的2倍。
- 复制table的元素到newTab，忽略陈旧的元素，假设table中的元素e需要复制到newTab的i位置，如果i位置存在元素，则找下一个空位置进行插入。

      文章到这里就结束了，没有更多了:）
       文章内容部分来自互联网，部分为自己整理，最后感谢耐心看到这里的人，
