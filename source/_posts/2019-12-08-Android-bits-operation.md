---
layout: post
title: 弄懂Android 源码中那些巧妙位运算
category: accumulation
tags:
    - bits operation
keywords: bits operation
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Field%20with%20Poppies.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Field%20with%20Poppies.jpg
toc: true
---

本文部分内容节选自[星星笑语](https://blog.csdn.net/u014068277)的文章
###  原码反码补码的概念
在介绍位运算符之前，我们先来看看关于反码补码等的相关知识。

- 正数的原码反码补码相同，都是将数字转换为二进制形式，然后将高位补0。比如说对于8位来说：
    - 10所对应的原码反码补码都是 0000 1010
    - 30所对应的原码反码补码都是 0001 1110
<!--more-->
- 而对于负数，负数的原码是它的绝对值对应的二进制，而最高位是1。所以：
    - -10所对应的原码是 1000 1010
    - -30所对应的原码是 1001 1110

- **负数的反码是它原码最高位除外的其余所有位取反**，所以：
    - -10所对应的反码是 1111 0101
    - -30所对应的反码是 1110 0001

- **而负数的补码是将其反码的数值+1**，所以：
    - -10所对应的补码是 1111 0110
    - -30所对应的补码是 1110 0010

是不是感觉负数的三种很杂乱？没关系，马上让你觉得不过如此～

我们对于刚才计算出来的负数的三种码，我们抛开符号为的概念，把三种码作为单存的二进制数来看。首先对于原码，原码是在最高位写上1得到的，而对于8位二进制，最高位的1是128，所以：
- -10所对应的原码是 1000 1010，而这个二进制数是128+10；
- -30所对应的原码是 1001 1110，而这个二进制数是128+30；

然后对于反码，在原码的基础上，最高位不变，其余各位取反，也就是说抛开最高位，其余各位变化前后的和是111 1111，所以：
- -10所对应的反码是 1111 0101，其值为127-10+128 
- -30所对应的反码是 1110 0001，其值为127-30+128
> 其余各位变化前后的和是111 1111，意思是 `原码 + 反码 = 127`

至于补码，就是反码加上1，所以：
- **-10所对应的补码是 1111 0110，其值为256-10**
- **-30所对应的补码是 1110 0010，其值为256-30**
> 所以一个负数就可以看成是一个正数 256-x

计算机为什么要引入补码的概念呢？主要是补码能够简化计算，例如对于8位的二进制数，从0000 0000一直到1111 1111，如果看作补码，所表示的数依次为0，1，…，127，-128，-127，…，-2，-1。对于两个正数的加法，正数的补码就是它本身，所以直接相加就能得到结果（注意，如果和超过了127，结果会错误）；
> -128 对应的补码是 1000 0000，对应原码是 1 1000 0000

对于两个负数的加法，我们仍然可以将他们的补码直接相加，而得到结果～比如对于两个负数-a和-b，他们的补码分别是256-a和256-b，相加后是512-（a+b），当a+b小于128时（a+b>128时这两个数相加超过了范围，得不到正确的结果），在计算机的表示是1 xxxx xxxx由于最高位的1超过了8位的范围所以舍去，所以相加结果是128-（a+b）而这就是-a-b的补码。

对于正数a和负数-b之间的加法，我们将其补码相加后得到256+a-b，而这就是a-b的补码（不管ab谁大都成立）。

由于减法可以对被减数取反，所以我们只用考虑加法运算。所以说如果使用补码，可以很好的将加减法运算转换成两个正数（负数的补码可以看成是一个正数256-x）的加法运算。如果计算机保存补码，在做数之间的加减法运算的时候能够直接将补码相加而得到正确的结果（没超过范围的情况下）。

我们也能够直接相加得到正确的结果。由于减法可以对被减数取反，所以我们只用考虑加法运算

实际上，考虑将所以有理数按256取模得到的数相等来分成256组，在做加减乘运算的时候，用同组的任意两个数运算得到的结果均是相同的，而负数和其补码是位于同一个组的，所以在做四则运算的时候，加减乘运算完全可以使用其的补码。

### 整型在 Java的内存形式
学习完补码之后，我们认识到如果使用补码，其在做运算时的方便之处。而Java的整形就是保存的各个数的补码，我们也可以在 Java上验证：

~~~ Java
System.out.println(Integer.toHexString(-1));
System.out.println(Integer.toHexString(-10));
System.out.println(Integer.toHexString(Integer.MAX_VALUE));
System.out.println(Integer.toHexString(Integer.MAX_VALUE));

System.out.println(Long.toHexString(-1));
System.out.println(Long.toHexString(Long.MAX_VALUE));

ffffffff
fffffff6
7fffffff
7fffffff
ffffffffffffffff
7fffffffffffffff
~~~

### Java中的位运算

要使用位运算之前，肯定是要了解下面的这些运算符号以及作用的。

- 与运算（and）
**`&`**表示，当两个相同位对应的数都是1的时候，该位获得的结果才是1，否则为0
例如说`6 & 11`，转换为二进制就是`0110 & 1011`，结果为`0010`
- 或运算（or）
`|`表示，当两个相同位对应的数只要有一个数1的时候，该位获得的结果为1，否则为0
还是用6和11这两个数做例子，就是`0110 | 1011 = 1111`
- 非运算（not）
**`~`**表示，这个运算只针对一个数字，将数字全部取反（原来是0结果就是1，原来是1结果就是0）。例如说：`~110 = 001`
注意，这里的取反是对**二进制数**进行取反，如果对非二进制数取反，可能有意想不到的结果：
~~~ Java
System.out.println("～1 = " + ~(1));
System.out.println("～0 = " + ~(0));
~~~
这个例子是对整数int进行取反，结果是：`~1 = -2  ~0 = -1`，详细回答可以看[这里](https://bbs.csdn.net/wap/topics/100095663)
- 异或运算（xor）
**`^`**表示，当两个相同位对应的数字不同的时候为1，否则为0
例如说：`0110 ^ 1011 = 1101`
- 左移（shl）
`<<`表示，`a << b`表示a左移b位，由于移位在末位多出来的未知数字补零。
在这里面可以等价为`a * 2^b`这个运算（针对十进制）。
- 右移（shr）
`>>`表示，`a >> b`表示将a右移b位，原本的末位进行右移后会被舍弃，左边的用原有标志位补充。同样的，右移在十进制里面也可以近似为`a / (2^b)`的形式，不过要对结果取整，也不一定准确，只能够说意思大概如此。
- 无符号右移
`>>>`表示，不管正负标志位为0还是1，将该数的二进制码整体右移，左边部分总是以0填充，右边部分舍弃。

> 1 << 1 = 2(对于int数的计算) 4 >> 1 = 2(对于int数的计算)  4 >>> 1 = 2(对于int数的计算)

~~~ Java
System.out.println(Integer.toHexString(0x7fffffff >> 4));
System.out.println(Integer.toHexString(0x8fffffff >> 4));
System.out.println(Integer.toHexString(0xffffffff >> 4));
System.out.println(Integer.toHexString(0x0fffffff >> 4));
System.out.println();
System.out.println(Integer.toHexString(0x7fffffff << 4));
System.out.println(Integer.toHexString(0x8fffffff << 4));
System.out.println(Integer.toHexString(0xffffffff << 4));
System.out.println(Integer.toHexString(0x0fffffff << 4));
System.out.println();
System.out.println(Integer.toHexString(0x7fffffff >>> 4));
System.out.println(Integer.toHexString(0x8fffffff >>> 4));
System.out.println(Integer.toHexString(0xffffffff >>> 4));
System.out.println(Integer.toHexString(0x0fffffff >>> 4));

        7ffffff
        f8ffffff
        ffffffff
        ffffff

        fffffff0
        fffffff0
        fffffff0
        fffffff0

        7ffffff
        8ffffff
        fffffff
        ffffff
~~~

总结来说：

- 右移：整体向右移，原来的最高位是0，就补0，原来的最高位是1，就补1；
- 左移：整体左移，在最低位补0；
- 无符号右移：最高位补0。

位运算符的使用又如下几点需要注意：

- 位运算符都是在整型保存在内存地址上的每一位进行运算的，所以有些会看起来超出我们的常识；
- 左移右移并不能简单的看作是在数的基础上乘以2或者除以2，而要结合它的内存形式来具体分析。

例如：
~~~ Java
System.out.println(~1);
结果是-2而不是0

这是因为1在内存上是000....001,~1就是111...1110,而这是-2的补码，所以结果是-1；

System.out.println(-1 >> 1);
结果还是-1而不是0

这是因为-1在内存上是111...1，它的符号位是1，所以右移的时候高位补1，结果不变；

System.out.println(Integer.MAX_VALUE << 1);
结果是-2，正数左移移成了负数。。。

System.out.println(0xBFFFFFFF);
System.out.println(0xBFFFFFFF << 1);
-1073741825
2147483646
负数左移成了正数。。。

System.out.println(-1 >>> 1);
2147483647
-1左移成了int的最大值。。。
~~~

### Android中的应用
- **`A | B` 添加标志B，可以添加多个（只要不冲突）**
例如：
~~~ Java
mViewFlags = SOUND_EFFECTS_ENABLED | HAPTIC_FEEDBACK_ENABLED | FOCUSABLE_AUTO;

// 或者
mPrivateFlags |= PFLAG_FORCE_LAYOUT;
mPrivateFlags |= PFLAG_INVALIDATED; 
~~~

- **`A & B` 判断A中是否有标志B**
原理就是与运算之中的1 & 1 = 1, 0 & 1 = 0，以此来判断flag对应位是否存在该flag
以(mViewFlags & FOCUSABLE_AUTO) != 0这个判断语句为例：
若为真，则与的结果不等于0，表示flag之中有该标志
~~~ Java
(mPrivateFlags & PFLAG_FORCE_LAYOUT) == PFLAG_FORCE_LAYOUT
~~~

- **`A & ~B` 去除标志B（上者的逆运算）**
例如：mViewFlags = (mViewFlags & ~FOCUSABLE) | newFocus用于去除原有的标志位并附上新的标志位（相当于更新）
~~~ Java
mPrivateFlags3 &= ~PFLAG3_MEASURE_NEEDED_BEFORE_LAYOUT;
~~~
- **`A ^ B` 取出A与B不同的部分，一般用于判断A是否发生改变**
~~~ Java
int changed = mViewFlags ^ old;
if (changed == 0) {
    return;
}
~~~

#### MeasureSpec
许最早在Android源码里面接触位运算的话就是在自定义View部分的时候，当书里面提及MeasureSpec这个变量的时候采用如下描述：
> MeasureSpec代表一个32位int值，高2位代表SpecMode，低30位代表SpecSize，SpecMode是指测量模式，而SpecSize是指在某种测量模式下的规格大小。

这里面就是典型的位运算的运用，不论是这个变量需要分别拆分获得SpecMode和SpecSize，还是其他的一些相关的操作，都需要位运算。
首先是SpecMode，在描述之中是高2位的部分，那么在处理之中一定会运用到左移或者右移来完成需求。

在这个类里面，一开始就声明了两个基本变量以及不同测量模式的值，已经用到了位运算中的左移
~~~ Java
// 声明位移量
private static final int MODE_SHIFT = 30;
// 后期截取SpecMode或SpecSize时使用的变量
// 3对应的二进制是11，左移30位后，int值的前2位就都是1，后30位为0，二进制是：11000000000000000000000000000000
private static final int MODE_MASK  = 0x3 << MODE_SHIFT;
// 三种测量模式对应的值
public static final int UNSPECIFIED = 0 << MODE_SHIFT;
public static final int EXACTLY     = 1 << MODE_SHIFT;
public static final int AT_MOST     = 2 << MODE_SHIFT;
~~~

先看看是如何通过位运算来获取SpecMode和SpecSize：
~~~ Java
@MeasureSpecMode
// 等价于@IntDef(value={...})。一种枚举类注释
public static int getMode(int measureSpec) {
    return (measureSpec & MODE_MASK);
    // 让低30位的值变为0，只保留高2位的值
}

public static int getSize(int measureSpec) {
    return (measureSpec & ~MODE_MASK);
    // 非运算直接让MASK值变成int值高2位为0，低30位为1
    // 进行与运算，直接将高2位的值变为0
}
~~~
这里就是典型的通过位运算来截取对应值，利用的是`x & 1 = x, x & 0 = 0`，其中x代表0与1两种值。

这种方式让一个变量能够存储多个内容方式实现，甚至也可以使用这样的方式将合成的值作为特定的key来做匹配或者相似需求。
接下来是如何获得MeasureSpec值：
~~~ Java
public static int makeMeasureSpec(
    @IntRange(from = 0, to = (1 << MeasureSpec.MODE_SHIFT) - 1) int size,
    // 要求传入的size值在指定范围内
    @MeasureSpecMode int mode) {
    if (sUseBrokenMakeMeasureSpec) {// 是否用原来的方法对MeasureSpec进行构建
        return size + mode;
    } else {
        return (size & ~MODE_MASK) | (mode & MODE_MASK);
    }
}
~~~
在API17及其以下的时候，是按照`size + mode`的方式进行构建，一个是只有int前2位有值，一个是只有int后30位有值，这么思考处理也情有可原。但假若两个值有溢出情况就会严重影响MeasureSpec的结果。故Google官方在API17之后就对该方法进行了修正，也是采用的位运算的形式：`(size & ~MODE_MASK) | (mode & MODE_MASK)`。相当于就是分别获得了SpecSize和SpecMode后通过或运算获得结果。

> 位运算应用口诀
清零取反要用与，某位置一可用或
若要取反和交换，轻轻松松用异或

Java中整形就是保存的各个数的补码。关于浮点型或原码、反码与补码的概念，请看这篇文章：[Java基础补完之数值与位运算符](https://blog.csdn.net/u014068277/article/details/102654292)

### 补充
1、判断两个数是否异号
~~~ Java
int x = -1, y = 2;
bool f = ((x ^ y) < 0); // true
int x = 3, y = 2;
bool f = ((x ^ y) < 0); // false
~~~
这个技巧还是很实用的，利用的是补码编码的符号位。如果不用位运算来判断是否异号，需要使用if else分支，还挺麻烦的。有人可能想到利用乘积或者商来判断，但是这种方式可能造成溢出，从而出现错误。

### Ref
[谈谈位运算和在Android中的运用](https://juejin.im/post/5c51f308e51d45141a1f2f2c)

[弄懂Android 源码中那些巧妙位运算](https://www.itcodemonkey.com/article/11303.html)

[Android中巧妙的位运算](https://blog.csdn.net/zzp16/article/details/7956768)
