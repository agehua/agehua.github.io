---
layout: post
title: 弄懂Android 源码中那些巧妙位运算
category: accumulation
tags:
    - bits operation
keywords: bits operation
banner: http://cdn.conorlee.top/Field%20with%20Poppies.jpg
thumbnail: http://cdn.conorlee.top/Field%20with%20Poppies.jpg
toc: true
---

### 位运算

要使用位运算之前，肯定是要了解下面的这些运算符号以及作用的。
<!--more-->

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
`>>`表示，`a >> b`表示将a右移b位，原本的末位进行右移后会被舍弃，若有需求会在高位进行补零。同样的，右移在十进制里面也可以近似为`a / (2^b)`的形式，不过要对结果取整，也不一定准确，只能够说意思大概如此。

### Android中的应用
- **`A | B` 添加标志B，可以添加多个（只要不冲突）**
例如：
~~~ Java
mViewFlags = SOUND_EFFECTS_ENABLED | HAPTIC_FEEDBACK_ENABLED | FOCUSABLE_AUTO;
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

### Ref
[谈谈位运算和在Android中的运用](https://juejin.im/post/5c51f308e51d45141a1f2f2c)

[弄懂Android 源码中那些巧妙位运算](https://www.itcodemonkey.com/article/11303.html)

[Android中巧妙的位运算](https://blog.csdn.net/zzp16/article/details/7956768)
