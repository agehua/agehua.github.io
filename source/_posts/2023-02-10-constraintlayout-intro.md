---
layout: post
title: ConstraintLayout 高级用法介绍
category: accumulation
tags:
  - ConstraintLayout
keywords: ConstraintLayout
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Harvest%20at%20La%20Crau%2C%20with%20Montmajour%20in%20the%20Background.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Harvest%20at%20La%20Crau%2C%20with%20Montmajour%20in%20the%20Background.jpg
toc: true
---

本文介绍了部分ConstraintLayout 2.0新增的api，同时介绍了一下ConstraintLayout中不太常用的方法，方便以后查阅。

<!--more-->

### 边距
边距与平常使用并无太大区别，但需要先确定view的位置，边距才会生效。如：

#### GONE Margin
ConstraintLayout提供了特殊的goneMargin属性，在目标View隐藏时，属性生效。有如下属性：

- layout_goneMarginStart
- layout_goneMarginEnd
- layout_goneMarginLeft
- layout_goneMarginTop
- layout_goneMarginRight
- layout_goneMarginBottom

#### 居中
在RelativeLayout居中，通常是使用以下三个属性：

- layout_centerInParent 中间居中
- layout_centerHorizontal 水平居中
- layout_centerVertical 垂直居中

而在ConstraintLayout居中则采用左右上下边来约束居中。

- 水平居中 layout_constraintLeft_toLeftOf & layout_constraintRight_toRightOf
- 垂直居中 layout_constraintTop_toTopOf & layout_constraintBottom_toBottomOf
- 中间居中 水平居中 & 垂直居中

#### 偏移
- layout_constraintHorizontal_bias 水平偏移
- layout_constraintVertical_bias 垂直偏移

两个属性的取值范围在0-1。在水平偏移中，0表示最左，1表示最右；在垂直偏移，0表示最上，1表示最下；0.5表示中间


#### 角度定位（圆形定位）
角度定位指的是可以用一个角度和一个距离来约束两个空间的中心。举个例子：
~~~ xml
    <TextView
        android:id="@+id/TextView1"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content" />

    <TextView
        android:id="@+id/TextView2"
        android:layout_width="wrap_content"
        android:layout_height="wrap_content"
        app:layout_constraintCircle="@+id/TextView1"
        app:layout_constraintCircleAngle="120"
        app:layout_constraintCircleRadius="150dp" />
~~~
上面例子中的TextView2用到了3个属性：
- `app:layout_constraintCircle="@+id/TextView1"`
- `app:layout_constraintCircleAngle="120"（角度）`
- `app:layout_constraintCircleRadius="150dp"（距离）`
    指的是TextView2的中心在TextView1的中心的120度，距离为150dp

有ConstraintLayout实践经验的朋友应该知道margin设置负值在ConstraintLayout是没有效果的.可以通过Space来间接实现view重叠效果。

~~~ java
/**
 * Space is a lightweight View subclass that may be used to create gaps between components
 * in general purpose layouts.
 */
public final class Space extends View {
~~~

### ConstraintLayout中的控件尺寸约束
在ConstraintLayout中控件可以三种方式来设置其尺寸约束。

- 指定具体的值。如123dp
- 使用值 WRAP_CONTENT,内容自适配。
- 设为 0dp ,即 `MATCH_CONSTRAINT`，扩充可用空间。
    这种情况会根据约束情况重新计算控件大小，一般会测量2次

> 在ConstraintLayout中，不推荐使用MATCH_PARENT，而是推荐使用MATCH_CONSTRAINT(0dp),它们的行为是类似的

控件设为MATCH_CONSTRAINT时，控件的大小会扩展所有可用空间，可以通过以下属性改变控件的行为。

- layout_constraintWidth_min  最小宽度
- layout_constraintHeight_min 最小高度
- layout_constraintWidth_max    最大宽度
- layout_constraintHeight_max   最大高度
- layout_constraintWidth_percent  宽度占parent的百分比
- layout_constraintHeight_percent 高度占parent的百分比

#### 比例约束
控件的宽高比，要求是宽或高至少一个设为0dp，然后设置属性layout_constraintDimensionRatio即可。
~~~ xml
<TextView
    android:text="Hello"
    app:layout_constraintDimensionRatio="3:1"
    android:layout_width="0dp"
    android:layout_height="100dp"
    />
~~~
这里设置宽高比为3:1,高度为100dp,那么宽度将为300dp。

也可以在比例前加W,H表示是宽高比还是高宽比，比如：
~~~ xml
  app:layout_constraintDimensionRatio="H,16:9" // 高宽比16:9
~~~

### 链
链在水平或者垂直方向提供一组类似行为，多个元素在水平或垂直方向想相互约束，并且最终与parent约束，才能形成一条链。

横向链最左边第一个控件，垂直链最顶边第一个控件称为链头，可以通过下面两个属性链头统一定制链的样式。
~~~ java
layout_constraintHorizontal_chainStyle //水平方向链式
layout_constraintVertical_chainStyle //垂直方向链式
~~~
它两的值可以是

- `CHAIN_SPREAD` **展开样式（默认）**
- `Weighted chain` 在CHAIN_SPREAD样式，部分控件设置了MATCH_CONSTRAINT，那他们将扩展可用空间。
    配合 layout_constraintHorizontal_weight="1" 使用
- `CHAIN_SPREAD_INSIDE` **展开样式**，但两端不展开
- `CHAIN_PACKED` **抱团(打包)样式**，控件抱团一起。通过偏移bias，可以改变packed元素的位置。

![Chain type](/images/blogimages/2023/ConstraintLayout_chain-intro.webp)

### 工具类

#### Guideline（参考线）

参考线实际上不会在界面进行显示，只是方便在ConstraintLayout布局view时候做一个参考。
通过设置Guideline的属性orientation来表示是水平方向还是垂直方向的参考线，对应值为vertical和horizontal。可以通过三种方式来定位Guideline位置。

- `layout_constraintGuide_begin` 从左边或顶部指定具体的距离
- `layout_constraintGuide_end`   从右边或底部指定具体的距离
- `layout_constraintGuide_percent` 从宽度或高度的百分比来指定具体距离


#### Barrier（栅栏）

Barrier有点类似Guideline，但Barrier会根据所有引用的控件尺寸的变化重新定位。例如经典的登录界面，右边的EditText总是希望与左右所有TextView的最长边缘靠齐。
如果两个TextView其中一个变得更长，EditText的位置都会跟这变化，这比使用RelativeLayout灵活很多。

~~~ java
<android.support.constraint.ConstraintLayout
        xmlns:android="http://schemas.android.com/apk/res/android"
        xmlns:app="http://schemas.android.com/apk/res-auto"
        android:layout_width="match_parent"
        android:layout_height="match_parent">

    <android.support.constraint.Barrier
            android:layout_width="wrap_content"
            android:layout_height="wrap_content"
            app:barrierDirection="right"
            android:id="@+id/barrier"
            app:constraint_referenced_ids="tvPhone,tvPassword"
            />

    <TextView android:layout_width="wrap_content"
              android:text="手机号码"
              android:id="@+id/tvPhone"
              android:gravity="center_vertical|left"
              android:padding="10dp"
              android:layout_height="50dp"/>

    <TextView android:layout_width="wrap_content"
              android:text="密码"
              android:padding="10dp"
              android:gravity="center_vertical|left"
              android:id="@+id/tvPassword"
              app:layout_constraintTop_toBottomOf="@id/tvPhone"
              android:layout_height="wrap_content"/>

    <EditText android:layout_width="wrap_content"
              android:hint="输入手机号码"
              android:id="@+id/etPassword"
              app:layout_constraintLeft_toLeftOf="@id/barrier"
              android:layout_height="wrap_content"/>

    <EditText android:layout_width="wrap_content"
              android:hint="输入密码"
              app:layout_constraintTop_toBottomOf="@id/etPassword"
              app:layout_constraintLeft_toLeftOf="@id/barrier"
              android:layout_height="wrap_content"/>


</android.support.constraint.ConstraintLayout>
~~~

app:barrierDirection所引用控件对齐的位置，可设置的值有：bottom、end、left、right、start、top.constraint_referenced_ids为所引用的控件，例如这里的tvPhone,tvPasswrod。

#### Group（组）
用来控制一组view的可见性，如果view被多个Group控制，则以最后的Group定义的可见性为主。

~~~ java
<androidx.constraintlayout.widget.Group
    android:layout_height="wrap_content"
    android:layout_width="wrap_content"
    android:visibility="visible"
    app:constraint_referenced_ids="textView3,textView2,textView1" />
~~~

#### Placeholder（占位符）
一个view占位的占位符，当指定Placeholder的content属性为另一个view的id时，该view会移动到Placeholder的位置。
~~~ xml
   <androidx.constraintlayout.widget.Placeholder
        android:id="@+id/place_holder"
        android:layout_height="60dp"
        android:layout_width="100dp"
        app:layout_constraintEnd_toEndOf="parent"
        app:layout_constraintTop_toTopOf="@+id/guideline2" 
        app:content="@id/textView3"/>
~~~
也可以通过代码调用的方式移动view到PlaceHolder，如下：
~~~ java
placeholder.setContentId(R.id.textView3);
~~~

#### Optimizer
当我们使用 MATCH_CONSTRAINT 时，ConstraintLayout 将对控件进行 2 次测量，ConstraintLayout在1.1中可以通过设置 layout_optimizationLevel 进行优化，可设置的值有：

- none：无优化
- standard：仅优化直接约束和屏障约束（默认）
- direct：优化直接约束
- barrier：优化屏障约束
- chain：优化链约束
- dimensions：优化尺寸测量

#### Layer
把一些控件组合在一起，当作一个图层，该图层自动计算边界，也是View的子类，但是功能不是完整的，Layer并不会增加 view 的层级。

支持的操作：
- 设置背景色
- 支持elevation属性
- 设置可见性
- 支持补间动画（alpha 是layer动，scale,rotation,transaction是其中的每个控件同时动）
- 多个图层同时包含同一个控件
- 图层本身支持事
- 支持padding、不支持margin、不支持大小


#### 自定义 Helper
为什么需要自定义?

- 保持 view 的层级不变，不像 ViewGroup 会增加 view 的层级
- 封装一些特定的行为，方便复用
- 一个 View 可以被多个 Helper引用，可以很方便组合出一些复杂的效果出来

如何自定义?

~~~ java
public abstract class ConstraintHelper extends View 

public abstract class VirtualLayout extends ConstraintHelper 

public class Flow extends VirtualLayout 
~~~

- Helper持有 view 的引用，所以可以获取 view (getViews)然后操作 view。
- 提供了 onLayout 前后的 callback（updatePreLayout/updatePreLayout）
- Helper 继承了 view，所以Helper本身也是 view


### Constraintlayout 2.0新增/少见的用法

#### Flow 流式布局

~~~ xml
<androidx.constraintlayout.helper.widget.Flow
        android:id="@+id/flow_test"
        android:layout_height="wrap_content"
        android:layout_width="wrap_content"
        app:constraint_referenced_ids="tvA,tvB,tvC,tvD,tvE,tvF,tvG"
        app:flow_firstHorizontalStyle="spread"
        app:flow_horizontalGap="30dp"
        app:flow_lastHorizontalStyle="packed"
        app:flow_maxElementsWrap="4"
        app:flow_verticalGap="30dp"
        app:flow_wrapMode="chain"
        app:layout_constraintEnd_toEndOf="parent"
        app:layout_constraintStart_toStartOf="parent"
       ...
~~~

flow_wrapMode属性一共有四种值：
- none: 所有引用的view形成一条链，水平居中，超出屏幕两侧的view不可见.
- chain: 所引用的View形成一条链，超出部分会自动换行，同行的View会平分宽度，中间排列。
- aligned: 所引用的View形成一条链，超出部分会自动换行，同行的View会优先左侧排列。
- chain2: 所引用的View形成一条链，超出部分会自动换行，同行的View会平分宽度。但会导致app:flow_maxElementsWrap 属性失效

其他属性：
- `app:flow_maxElementsWrap="4"` 一行几个元素
- `app:flow_verticalGap="10dp"` 竖直间距
- `app:flow_horizontalGap="10dp"` 横向间距
- `android:orientation="horizontal"`水平方向的流式还是竖直方向的流式
- `app:flow_verticalAlign ="top"` 值有top，bottom，center，baseline。每一行元素的对齐方式
- `app:flow_horizontalStyle = "spread | spread_inside | packed"` 当wrapMode为chain或ALIGNED时生效
- `app:flow_horizontalBias = "float"` low的bias偏移，只在style为packed时生效

#### ImageFilterView
ImageFilterView继承自ImageView，可以轻松实现圆角、图片色彩调节等
~~~ xml
    <androidx.constraintlayout.utils.widget.ImageFilterView
        android:id="@+id/imageFilterView"
        android:layout_height="50dp"
        android:layout_width="50dp"
        app:altSrc="@mipmap/ic_launcher"
        app:brightness="1"
        app:contrast="4"
        app:crossfade="0.5"
        app:round="20dp"
        app:roundPercent="1"
        app:srcCompat="@drawable/test_logo"
        app:warmth="1" />
~~~
roundPercent取值在0-1，由正方形向圆形过渡。

但是以下情况可能圆角会失效：
> layout_width或layout_height为wrapcontent时，圆角失效。或者设置了layout_constraintHorizontal_weight，且layout_width为0dp时，圆角失效

其他属性：
- `app:altSrc="@mipmap/ic_launcher"`
    altSrc和src属性是一样的概念，altSrc提供的资源将会和src提供的资源通过crossfade属性形成交叉淡化效果。默认情况下,crossfade=0，altSrc所引用的资源不可见,取值在0-1
- `warmth` 色温：1=neutral自然, 2=warm暖色, 0.5=cold冷色
- `brightness` 亮度：0 = black暗色, 1 = original原始, 2 = twice as bright两倍亮度；
- `saturation` 饱和度：0 = grayscale灰色, 1 = original原始, 2 = hyper saturated超饱和；
- `contrast` 对比度：1 = unchanged原始, 0 = gray暗淡, 2 = high contrast高对比;

#### ImageFilterButton
ImageFilterView与ImageFilterButton的属性一模一样，只是它两继承的父类不一样，一些操作也就不一样。ImageFilterButton继承自AppCompatImageButton, 也就是ImageButtion。而ImageFilterView继承自ImageView。

~~~ java
public class ImageFilterButton extends androidx.appcompat.widget.AppCompatImageButton {
    //...

public class ImageFilterView extends androidx.appcompat.widget.AppCompatImageView {
    //...
~~~

#### MockView
MockView能简单的帮助构建UI界面，通过对角线形成的矩形+标签。例如：
~~~ xml
 <androidx.constraintlayout.utils.widget.MockView
        android:id="@+id/first"
        android:layout_width="100dp"
        android:layout_height="100dp"
        app:layout_constraintLeft_toLeftOf="parent"
        app:layout_constraintTop_toTopOf="parent" />
~~~

### 关键帧动画

在 ConstraintLayout 中，可以使用 ConstraintSet 和 TransitionManager 为尺寸和位置元素的变化添加动画效果。
> 重要提示：ConstraintSet 动画仅为子元素的尺寸和位置添加动画效果。它们不会为其他属性（例如颜色）添加动画效果。


最后，ConstraintLayout 2.0还增加了ConstraintProperties类用于通过api(代码)更新ConstraintLayout子视图，ConstraintsChangedListener监听，具体可以参考Api。


### 参考
https://juejin.cn/post/6949186887609221133

https://juejin.cn/post/7071165641973530638