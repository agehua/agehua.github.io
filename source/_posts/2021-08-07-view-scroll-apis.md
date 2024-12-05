---
layout: post
title: 移动view的几种方式总结
category: accumulation
tags:
    - view scroll 
keywords: scroll, view, layout, margin
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Garden%20in%20Auvers.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Garden%20in%20Auvers.jpg
toc: true
---

### 移动view的几种方式

#### 使用View.layout(l, t, r, b)方法
> Assign a size and position to a view and all of its descendants
这个方法是用来指定一个view和它所有的子view的尺寸和位置
<!--more-->
> This is the second phase of the layout mechanism. (The first is measuring). In this phase, each parent calls layout on all of its children to position them. This is typically done using the child measurements that were stored in the measure pass()
这个是布局的第二阶段，第一阶段是 measure。这个阶段由父view调用每个子view的layout方法来定位子view。
> Derived classes should not override this method. Derived classes with children should override onLayout. In that method, they should call layout on each of their children.
实现类不应该重写这个方法，而是应该重写 onLayout 方法，在onLayout方法中会调用实现类的各个子view的layout方法

这四个参数也都是相对于父视图来说的，layout的过程就是确定View在屏幕上显示的具体位置，在代码中就是设置其成员变量mLeft，mTop，mRight，mBottom的值，这几个值构成的矩形区域就是该View显示的位置，不过这里的具体位置都是**`相对于父视图`**的位置。

其中，mRight所指的是当前view的**`右边缘离父视图左边缘的距离`**（mLeft+自己的宽度），mBottom也是指当前view的**`下边缘离父视图的上边缘的距离`**

> 这种方式可以刷新页面，但是一旦调用requestLayout，会导致view回到最开始的位置，因为mTop、mLeft被重置了

#### 使用View.setX()
在 getX() 方法的注释上是这样说的：
> The visual x position of this view, in pixels. This is equivalent to the setTranslationX(float) translationX property plus the current getLeft() left property.

意思就是说获取到实际的View的x y 值，该值 **`getX = getLeft + getTraslationX`**，并且最后得出的值也是相对父视图来说的。

#### 使用setTranslationX(float translationX)

> Sets the horizontal location of this view relative to its {@link #getLeft() left} position. This effectively positions the object post-layout, in addition to wherever the object's layout placed it.

> param translationX: The horizontal position of this view relative to its left position, in pixels.

translateX是相对于**自身left属性**来进行的平移，比如 left=20，此时调用setTranslationX(30)后，就是相对于left（而不是它上一次的translationX后的位置）往右平移30px，若此时再调用调用setTranslationX(40)，之前的30px值就被覆盖，改为相对于left右移40px。

这里水平方向上的` X `也是相对于父容器的当前View的位置，就像前面的等式所说，x = left + translationX；所以每一次setTranslationX都会导致getX值的变化，而left的值并不会改变，requestLayout也不会让view发生任何改变


#### 使用View的offsetLeftAndRight(offset)
> Offset this view's horizontal location by the specified amount of pixels.

垂直方向上使用 offsetTopAndBottom()。RecyclerView移动子view就是使用这两个方法，具体原理可以看我的这篇文章[Recyclerview 滑动相关功能总结](https://conorlee.top/2020/12/04/RecyclerView_scroll_relatedapi/)。
本质上也是改变mTop、mLeft的值，但是要注意这个方法不能用在onTouch的MotionEvent.ACTION_MOVE事件上，会导致滑动距离加倍

#### 改变LayoutParams的margin
LayoutParams是相对于父布局而言，所以这种方式只能在父布局范围内移动view，如果父View内只有一个子view，`推荐使用这种方式移动子view`
`setmargins 相对父视图来实现移动，前提是不能跟其他子view发生关系`
需要手动调用 requestLayout 通知重绘
#### 使用 setPadding(l, t, r, b)
设置该**view的内部**缩进值(自己内部)，相对于Margin(相对于其他view或ViewGroup). 不推荐使用这种方式移动view，因为可能会造成View内部内容展示不全

#### 利用View的scrollTo(x, y), scrollBy(x, y)
> Set the scrolled position of your view. This will cause a call to {@link #onScrollChanged(int, int, int, int)} and the view will be invalidated.
这个方法修改的是View的两个成员变量mScrollX、mScrollY，然后触发了 invalidate，在View的onDraw()方法中会使用这两个成员变量来确定view的位置

`注意这个方法的滑动方向与正常方向不同 比如如果从左向右滑动100px 那么 是scrollBy(-100,0)`
这两个方法有个致命的缺陷：它们滑动的是`View里面的内容`而不是View自身 也就是说如果用于ViewGroup中内容的滑动是可以的 但是如果用于让自身控件移动是不可以的

### 总结
- 建议优先使用LayoutParams的方式，前提是父View中只有这一个子View，否则会因margin属性的改变，导致其他子view无法正常显示
- 其次如果onTouch事件中实现view的移动，可以使用view.layout(l, t, r, b)方法
- 不是在onTouch事件中，可以使用属性动画setTranslationX

### 拓展

#### 贴一个跟随手指移动view的实现
- 1.使用layoutparams.setMargin方式（Kotlin）
~~~ java
override fun onTouchEvent(event: MotionEvent): Boolean {
    when (event.action) {
        MotionEvent.ACTION_DOWN -> {
            if (null != mDragAnimSet && mDragAnimSet!!.isRunning) {
                mDragAnimSet!!.cancel()
            }
            isPressed = true
            isDrag = false
            inputStartX = event.rawX.toInt()
            inputStartY = event.rawY.toInt()
            viewStartLeft = mFloatBallParams.leftMargin
            viewStartBottom = mFloatBallParams.bottomMargin
        }
        MotionEvent.ACTION_MOVE -> run {
            val moveX: Int = event.rawX.toInt() - inputStartX
            val moveY: Int = event.rawY.toInt() - inputStartY
            if (mScreenWidth < 0 || mScreenHeight < 0) {
                isDrag = false
                return@run
            }
            if (abs(moveX) > slop && abs(moveY) > slop) {
                isDrag = true
                mFloatBallParams.leftMargin = viewStartLeft + moveX
                mFloatBallParams.bottomMargin = viewStartBottom - moveY
                layoutParams = mFloatBallParams
            } else {
                isDrag = false
            }
        }
        MotionEvent.ACTION_UP -> {
            if (isDrag) {
                //恢复按压效果
                isPressed = false
            }
            //吸附贴边计算和动画
            welt()
        }
        else -> {
        }
    }
    return isDrag || super.onTouchEvent(event)
}
~~~

- 2.使用view.layout()方式
~~~ java
@Override
public boolean onTouch(View view, MotionEvent event) {
    float  dx, dy;
    switch (event.getAction()) {
        case MotionEvent.ACTION_DOWN:
            startTouchX = event.getX();
            startTouchY = event.getY();

            circleImageLeft = circleImage.getLeft();
            circleImageRight = circleImage.getRight();
            break;

        case MotionEvent.ACTION_MOVE:
            dx = event.getX() - startTouchX;
            dy = event.getY() - startTouchY;
/* 
  注意：
  1. 这里不要用 (circleImage.getLeft() + dx)，因为每次layout之后已经改变了left的值，因此getLeft是已经更新后的值，这样将会导致dx全部累计，滑动使得view滑动的更快。 
  2. 一定要记得right也要跟着变，否则right不变的话，view将会被压缩或者拉伸。
*/
            circleImage.layout((int) (circleImageLeft + dx),
                    circleImage.getTop(),
                    (int) (circleImageRight + dx),
                    circleImage.getBottom());
            break;
        case MotionEvent.ACTION_UP:
            AppLog.e("ACTION_UP");
            break;
        case MotionEvent.ACTION_CANCEL:
            AppLog.e("ACTION_CANCEL");
            break;
    }

    return true;
}
~~~


#### MotionEvent.getX()、MotionEvent.getRawX()的区别
MotionEvent的getX(),getY()是当前触摸点相对于设置此监听触摸事件的view的左边和顶部的距离。
只有MotionEvent的getRawX(),getRawY()是绝对坐标。

#### getGlobalVisibleRect() 与 getLocalVisibleRect() 方法介绍

getGlobalVisibleRect() 是view**可见区域**相对与**屏幕**来说的坐标位置.
getLocalVisibleRect() 是view**可见区域**相对于**自己**坐标的位置.

`一定要记清楚是可见区域.`

详情查阅这篇文章 ：https://www.jianshu.com/p/2aa908f6a2e6


