---
layout: post
title: CoordinatorLayout源码解读
category: accumulation
tags:
    - CoordinatorLayout
keywords: NestedScrollingChild2, NestedScrollingParent
banner: http://cdn.conorlee.top/Field%20with%20Ploughman%20and%20Mill.jpg
thumbnail: http://cdn.conorlee.top/Field%20with%20Ploughman%20and%20Mill.jpg
toc: true
---
### CoordinatorLayout
`本文源码基于 27.1.1`
AppBarLayout 和 CoordinatorLayout是 2015年就已经推出的Material Design控件，一直以来也是只知其一
这次通过阅读源码参考实现一个水平方向的 CoordinatorLayout和AppBarLayout组合。
<!--more-->
CoordinatorLayout是一个增强型的FrameLayout。
CoordinatorLayout#LayoutParams是用在子view上，对应在xml里子view的”`app:layout_xxx`“属性。
通常的一个包含CoordinatorLayout的布局文件，大概是这样：
~~~ Java
<android.support.design.widget.CoordinatorLayout
    xmlns:android="http://schemas.android.com/apk/res/android"
    xmlns:app="http://schemas.android.com/apk/res-auto"
    xmlns:tools="http://schemas.android.com/tools"
    android:layout_width="match_parent"
    android:layout_height="match_parent"
    android:id="@+id/coordinator">
    <android.support.design.widget.AppBarLayout
        android:layout_width="match_parent"
        android:id="@+id/appbar"
        android:layout_height="220dp"
        android:background="#ffffff">
        ....
    </android.support.design.widget.AppBarLayout>
  
    <android.support.v7.widget.RecyclerView
        android:layout_width="match_parent"
        android:layout_height="500dp"
        android:background="#1d9d29"
        app:layout_behavior="@string/appbar_scrolling_view_behavior">
        ....
    </android.support.v7.widget.RecyclerView>

</android.support.design.widget.CoordinatorLayout>
~~~

来看一下CoordinatorLayout#LayoutParams的解析
~~~ Java
// CoordinatorLayout#LayoutParams
LayoutParams(Context context, AttributeSet attrs) {
    super(context, attrs);
    final TypedArray a = context.obtainStyledAttributes(attrs,
            R.styleable.CoordinatorLayout_Layout);
    // ...
    mBehaviorResolved = a.hasValue(
            R.styleable.CoordinatorLayout_Layout_layout_behavior);
    if (mBehaviorResolved) {
        // 这里通过反射获得对应的Behavior对象
        mBehavior = parseBehavior(context, attrs, a.getString(
                R.styleable.CoordinatorLayout_Layout_layout_behavior));
    }
    a.recycle();
    if (mBehavior != null) {
        // If we have a Behavior, dispatch that it has been attached
        mBehavior.onAttachedToLayoutParams(this);
    }
}
~~~
感兴趣的可以继续点到`CoordinatorLayout_Layout`style里面看看。

这里简单介绍几个属性：
- app:layout_anchor 
- app:layout_anchorGravity
这两个一般配合FloatingActionButton使用，用来指定位置
- app:layout_behavior
是本文的重点，为子View指定一个Behavider，是一个字符串，这是使用反射方式取得Behavior类。具体解析方式去看上面的`parseBehavior()方法`，这里不介绍了

### Bahavior介绍
在上面的布局文件里，RecyclerView指定了一个 appbar_scrolling_view_behavior，对应的类为： **android.support.design.widget.AppBarLayout$ScrollingViewBehavior**。
这个Behavior也可以配合其他view使用，比如ViewPager、ListView、ScrollView。

AppBarLayout为什么没有指定layout_behavior呢？
因为CoordinatorLayout提供了一个注解：
~~~ Java
@Deprecated
@Retention(RetentionPolicy.RUNTIME)
public @interface DefaultBehavior {
    Class<? extends Behavior> value();
}
~~~

而AppBarLayout正是通过注解的方式添加的：
~~~ Java
@CoordinatorLayout.DefaultBehavior(AppBarLayout.Behavior.class)
public class AppBarLayout extends LinearLayout {
    //...
}
~~~
关于这个注解的解析在getResolvedLayoutParams(View child)这个方法里，感兴趣的可以去看一下。
Behavior解析出来后，调用child.layoutParams.setBehavior()，保存在了child的LayoutParams的成员变量里。

CoordinatorLayout.Behavior是一个抽象类，总共有9个子类，直接子类有4个，如下：

- AppBarLayout.ScrollingViewBehavior 继承自 HeaderScrollingViewBehavior
- AppBarLayout.Behavior 继承自 HeaderBehavior
- BaseTransientBottomBar.Behavior 继承自 SwipeDismissBehavior
- BottomSheetBehavior
- FloatingActionButton.Behavior
- HeaderBehavior 继承自 ViewOffsetBehavior
- HeaderScrollingViewBehavior 继承自 ViewOffsetBehavior
- SwipeDismissBehavior
- ViewOffsetBehavior

下面以CoordinatorLayout + AppBarLayout + Recyclerview为例，分析用到的Behavior类结构如下：
![部分Behavior类图](/images/blogimages/2019/uml_classes_behavior.png)

现在CoordinatorLayout的子view都有了Behavior类，那么它们之间怎么和CoordinatorLayout交互并联动呢？

### 以AppBarLayout + Recyclerview为例分析子View之间联动
CoordinatorLayout是通过CoordinatorLayout.Behavior来控制子view的连动，这里涉及到触摸事件的处理。

~~~ Java
@Override
public boolean onTouchEvent(MotionEvent ev) {
    boolean handled = false;
    boolean cancelSuper = false;
    MotionEvent cancelEvent = null;

    final int action = ev.getActionMasked();
    // 这里mBehaviorTouchView 会在 performIntercept()方法返回true时被赋值，表示将事件交给CoordinatorLayout处理
    if (mBehaviorTouchView != null || (cancelSuper = performIntercept(ev, TYPE_ON_TOUCH))) {
        // Safe since performIntercept guarantees that
        // mBehaviorTouchView != null if it returns true
        final LayoutParams lp = (LayoutParams) mBehaviorTouchView.getLayoutParams();
        final Behavior b = lp.getBehavior();
        // 通过mBehaviorTouchView的layoutParams来获取Behavior对象，交由mBehaviorTouchView的Behavior对象来处理事件
        if (b != null) {
            handled = b.onTouchEvent(this, mBehaviorTouchView, ev);
        }
    }

    // Keep the super implementation correct
    if (mBehaviorTouchView == null) { // 如果mBehaviorTouchView 为空，则还是交由父类去处理touch事件
        handled |= super.onTouchEvent(ev);
    }
    //...
}
~~~

从上面分析可以看出，CoordinatorLayout本身并没有处理touch事件，而是哪个子view确定拦截事件后，交给这个子view的Behavior对象来处理这个事件。

我们以AppBarLayout + Recyclerview这两个比较常见的子View形式来分析。首先有两种滑动逻辑：

#### 当手指触摸AppBarLayout时候的滑动逻辑

为弄清手指触摸AppBarLayout时候的滑动逻辑，我们先看一下AppBarLayout.Behavior这个类。关于这个类的继承关系可以看上面的类图，这里先总结一下两个类的作用，需要详细的实现的请自行阅读源码吧：

ViewOffsetBehavior：该Behavior主要运用于View的移动，从名字就可以看出来，该类中提供了上下移动，左右移动的方法。
HeaderBehavior：该类主要用于View处理触摸事件以及触摸后的fling事件，AppBarLayout.Behavior的onTouchEvent和onInterceptTouchEvent处理逻辑都是在这里

HeaderBehavior的滑动都是通过setHeaderTopBottomOffset()方法处理的，这个方法在AppBarLayout.Behavior中进行了重写。

我们直接看AppBarLayout.Behavior中的源码：
~~~ Java
@Override
//newOffeset传入了dy，也就是我们手指移动距离上一次移动的距离，
//minOffset等于AppBarLayout的负的height，maxOffset等于0。
int setHeaderTopBottomOffset(CoordinatorLayout coordinatorLayout,
        AppBarLayout appBarLayout, int newOffset, int minOffset, int maxOffset) {
    final int curOffset = getTopBottomOffsetForScrollingSibling();//获取当前的滑动Offset
    int consumed = 0;
	//AppBarLayout滑动的距离如果超出了minOffset或者maxOffset，则直接返回0
    if (minOffset != 0 && curOffset >= minOffset && curOffset <= maxOffset) {
        //矫正newOffset，使其minOffset<=newOffset<=maxOffset
        newOffset = MathUtils.clamp(newOffset, minOffset, maxOffset);
		//由于默认没设置Interpolator，所以interpolatedOffset=newOffset;
        if (curOffset != newOffset) {
            final int interpolatedOffset = appBarLayout.hasChildWithInterpolator()
                    ? interpolateOffset(appBarLayout, newOffset)
                    : newOffset;
			//调用ViewOffsetBehvaior的方法setTopAndBottomOffset(...)，最终通过
			//ViewCompat.offsetTopAndBottom()移动AppBarLayout
            final boolean offsetChanged = setTopAndBottomOffset(interpolatedOffset);

            //记录下消费了多少的dy。
            consumed = curOffset - newOffset;
            //没设置Interpolator的情况， mOffsetDelta永远=0
            mOffsetDelta = newOffset - interpolatedOffset;
			//....
            //分发回调OnOffsetChangedListener.onOffsetChanged(...)
            appBarLayout.dispatchOffsetUpdates(getTopAndBottomOffset());

            updateAppBarLayoutDrawableState(coordinatorLayout, appBarLayout, newOffset,
                    newOffset < curOffset ? -1 : 1, false);
        }
    //...
    return consumed;
}
~~~

上面注释也解释的比较清楚了，通过setTopAndBottomOffset()来计算AppBarLayout滑动多少距离，达到移动AppBarLayout的目的，那么这里AppBarLayout就可以跟着手上下移动了.
**但是，RecyclerView是如何跟随滑动的呢？**
在上面的布局文件里，我们给RecyclerView指定了一个layout_behavior，前面也说了这个Behavior是通过反射获取的，点击去可以找到这个类，它是：
~~~ Java
android.support.design.widget.AppBarLayout$ScrollingViewBehavior
~~~
`注意区分，这个类不是我们上面提到的AppBarLayout.Behavior，两者并不相同`

AppBarLayout$ScrollingViewBehavior重写了下面两个方法，并通过这两个方法将AppBarLayout和Recyclerview关联了起来
~~~ Java
@Override
public boolean layoutDependsOn(CoordinatorLayout parent, View child, View dependency) {
    // We depend on any AppBarLayouts
    return dependency instanceof AppBarLayout;
}

@Override
public boolean onDependentViewChanged(CoordinatorLayout parent, View child,
        View dependency) {
    offsetChildAsNeeded(parent, child, dependency);
    return false;
}
~~~
Recyclerview依赖于AppBarLayout，在AppBarLayout移动的过程中，Recyclerview会随着AppBarLayout的移动回调onDependentViewChanged()方法，进而调用 offsetChildAsNeeded(parent, child, dependency)

~~~ Java
private void offsetChildAsNeeded(CoordinatorLayout parent, View child, View dependency) {
    final CoordinatorLayout.Behavior behavior =
            ((CoordinatorLayout.LayoutParams) dependency.getLayoutParams()).getBehavior();
    if (behavior instanceof Behavior) {
        // Offset the child, pinning it to the bottom the header-dependency, maintaining
        // any vertical gap and overlap
        final Behavior ablBehavior = (Behavior) behavior;
        ViewCompat.offsetTopAndBottom(child, (dependency.getBottom() - child.getTop())
                + ablBehavior.mOffsetDelta
                + getVerticalLayoutGap()
                - getOverlapPixelsForOffset(dependency));
    }
}
~~~

这样我们就知道了当手指移动AppBarLayout时候的过程，下面整理一下：

首先通过Behavior.onTouchEvent(...)收到滑动距离，进而通知AppBarLayout.Behavior调用ViewCompat.offsetTopAndBottom()进行滑动；在AppBarLayout滑动的过程中，由于Recyclerview中的ScrollingViewBehavior会依赖于AppBarLayout，所以在AppBarLayout滑动时候，Recyclerview也会随着滑动，调用的方法也是ViewCompat.offsetTopAndBottom()。

#### 当手指触摸Recyclerview时的滑动逻辑

在Touch触发的时候，先走到CoordinatorLayout的onInterceptTouchEvent()方法，然后走到performIntercept()，调用Behavior的onInterceptTouchEvent()方法，最终Recyclerview绑定的ScrollingViewBehavior并没有重写onInterceptTouchEvent()，所以父viewCoordinatorLayout并不会拦截这个Touch事件，所以会在Recyclerview的onInterceptTouchEvent进行处理。
~~~ Java
@Override
    public boolean onInterceptTouchEvent(MotionEvent e) {
        //...

        final int action = e.getActionMasked();
        final int actionIndex = e.getActionIndex();

        switch (action) {
            case MotionEvent.ACTION_DOWN:
                if (mIgnoreMotionEventTillDown) {
                    mIgnoreMotionEventTillDown = false;
                }
                mScrollPointerId = e.getPointerId(0);
                mInitialTouchX = mLastTouchX = (int) (e.getX() + 0.5f);
                mInitialTouchY = mLastTouchY = (int) (e.getY() + 0.5f);

                if (mScrollState == SCROLL_STATE_SETTLING) {
                    getParent().requestDisallowInterceptTouchEvent(true);
                    setScrollState(SCROLL_STATE_DRAGGING);
                }

                // Clear the nested offsets
                mNestedOffsets[0] = mNestedOffsets[1] = 0;

                int nestedScrollAxis = ViewCompat.SCROLL_AXIS_NONE;
                if (canScrollHorizontally) {
                    nestedScrollAxis |= ViewCompat.SCROLL_AXIS_HORIZONTAL;
                }
                if (canScrollVertically) {
                    nestedScrollAxis |= ViewCompat.SCROLL_AXIS_VERTICAL;
                }
                // 在这里触发nestedscroll
                startNestedScroll(nestedScrollAxis, TYPE_TOUCH);
                break;
                //...
        }
         return mScrollState == SCROLL_STATE_DRAGGING;
    }
~~~

Recyclerview实现了NestedScrollingChild2接口，但是具体实现都在NestedScrollingChildHelper类中
~~~ Java
public boolean startNestedScroll(@ScrollAxis int axes, @NestedScrollType int type) {
    if (hasNestedScrollingParent(type)) {
        // Already in progress
        return true;
    }
    if (isNestedScrollingEnabled()) { //是否支持嵌套滑动
        ViewParent p = mView.getParent();
        View child = mView; // mView就是当前的Recyclerview
        //从子View向外查询第一个接收滑动的父View，当前p就是CoordinatorLayout
        while (p != null) {
            if (ViewParentCompat.onStartNestedScroll(p, child, mView, axes, type)) {
                setNestedScrollingParentForType(type, p);
                //调用父view的onNestedScrollAccepted()
                ViewParentCompat.onNestedScrollAccepted(p, child, mView, axes, type);
                return true;
            }
            if (p instanceof View) {
                child = (View) p;
            }
            p = p.getParent();
        }
    }
    return false;
}
~~~

下面是ViewParentCompat.onStartNestedScroll()的处理代码：
~~~ Java
public static boolean onStartNestedScroll(ViewParent parent, View child, View target,
        int nestedScrollAxes, int type) {
    if (parent instanceof NestedScrollingParent2) {
        // First try the NestedScrollingParent2 API
        return ((NestedScrollingParent2) parent).onStartNestedScroll(child, target,
                nestedScrollAxes, type);
    } else if (type == ViewCompat.TYPE_TOUCH) {
        // Else if the type is the default (touch), try the NestedScrollingParent API
        return IMPL.onStartNestedScroll(parent, child, target, nestedScrollAxes);
    }
    return false;
}
~~~

CoordinatorLayout正好实现了NestedScrollingParent2接口，所以由CoordinatorLayout来处理onStartNestedScroll()方法
~~~ Java
@Override
public boolean onStartNestedScroll(View child, View target, int axes, int type) {
    boolean handled = false;

    final int childCount = getChildCount();
    for (int i = 0; i < childCount; i++) {
        final View view = getChildAt(i);
        if (view.getVisibility() == View.GONE) {
            // If it's GONE, don't dispatch
            continue;
        }
        final LayoutParams lp = (LayoutParams) view.getLayoutParams();
        final Behavior viewBehavior = lp.getBehavior();
        // 由CoordinatorLayout转发给各个Behavior
        if (viewBehavior != null) {
            final boolean accepted = viewBehavior.onStartNestedScroll(this, view, child,
                    target, axes, type);
            handled |= accepted;
            lp.setNestedScrollAccepted(type, accepted);
        } else {
            lp.setNestedScrollAccepted(type, false);
        }
    }
    return handled;
}
~~~
上面代码可以看出，由CoordinatorLayout转发给各个Behavior，根据其返回值作进一步处理

下面是Behavior的onStartNestedScroll()方法的注释：
~~~ Java
/**
     * Called when a descendant of the CoordinatorLayout attempts to initiate a nested scroll.
     *
     * <p>Any Behavior associated with any direct child of the CoordinatorLayout may respond
     * to this event and return true to indicate that the CoordinatorLayout should act as
     * a nested scrolling parent for this scroll. Only Behaviors that return true from
     * this method will receive subsequent nested scroll events.</p>
     //...
     * @see NestedScrollingParent2#onStartNestedScroll(View, View, int, int)
 */
~~~
当CoordinatorLayout的子类想要启动一个nested scroll（嵌套滑动）时需要重写这个方法。CoordinatorLayout的子类可以响应这个事件，返回true表示CoordinatorLayout应该扮演一个嵌套滑动的父类。只有返回true的Behavior才会收到后续的嵌套滑动事件。

在这个例子中（AppBarLayout+ Recyclerview），只有AppBarLayout.Behavior重写了这个方法，并返回了true。但是这里并没有移动view的方法。

> 简单总结一下：Recyclerview作为子View滑动时候会首先调用startNestedScroll(...)方法来询问父View即CoordinatorLayout是否需要消费事件，
CoordinatorLayout作为代理做发给对应Behavior，这里就分发给了AppBarLayout.Behavior，并返回true，说明AppBarLayout需要进行消费事件的处理.
而ScrollingViewBehavior作为事件的触发者，并没有重写startNestedScroll()方法，所以返回false，表示不需要对事件进行消费。

前面分析了Recyclerview的onInterceptTouchEvent()的处理流程，父类并没有拦截Touch事件。
所以，Touch事件肯定是在Recyclerview的 onTouchEvent()里进行了处理。最终，在Recyclerview的onTouchEvent对ACTION_MOVE的处理中看到了下面的代码：
~~~ Java
    case MotionEvent.ACTION_MOVE: {
        final int index = e.findPointerIndex(mScrollPointerId);
        if (index < 0) {
            Log.e(TAG, "Error processing scroll; pointer index for id "
                    + mScrollPointerId + " not found. Did any MotionEvents get skipped?");
            return false;
        }

        final int x = (int) (e.getX(index) + 0.5f);
        final int y = (int) (e.getY(index) + 0.5f);
        int dx = mLastTouchX - x;
        int dy = mLastTouchY - y;
        // 1.这里向父view（CoordinatorLayout）分发了滑动处理
        if (dispatchNestedPreScroll(dx, dy, mScrollConsumed, mScrollOffset, TYPE_TOUCH)) {
            // 剪掉被AppBarLayout消耗的距离，剩下距离作为Recyclerview的滑动距离
            dx -= mScrollConsumed[0];
            dy -= mScrollConsumed[1];
            vtev.offsetLocation(mScrollOffset[0], mScrollOffset[1]);
            // Updated the nested offsets
            mNestedOffsets[0] += mScrollOffset[0];
            mNestedOffsets[1] += mScrollOffset[1];
        }
        //...

        if (mScrollState == SCROLL_STATE_DRAGGING) {
            mLastTouchX = x - mScrollOffset[0];
            mLastTouchY = y - mScrollOffset[1];
            // 2.这里向父view（CoordinatorLayout）分发了滑动处理
            if (scrollByInternal(
                    canScrollHorizontally ? dx : 0,
                    canScrollVertically ? dy : 0,
                    vtev)) {
                getParent().requestDisallowInterceptTouchEvent(true);
            }
            if (mGapWorker != null && (dx != 0 || dy != 0)) {
                mGapWorker.postFromTraversal(this, dx, dy);
            }
        }
    }
~~~

进入上面注释2处：
~~~ Java
boolean scrollByInternal(int x, int y, MotionEvent ev) {
    int unconsumedX = 0, unconsumedY = 0;
    int consumedX = 0, consumedY = 0;

    consumePendingUpdateOperations();
    if (mAdapter != null) {
        startInterceptRequestLayout();
        onEnterLayoutOrScroll();
        TraceCompat.beginSection(TRACE_SCROLL_TAG);
        fillRemainingScrollValues(mState);
        if (x != 0) {
            consumedX = mLayout.scrollHorizontallyBy(x, mRecycler, mState);
            unconsumedX = x - consumedX;
        }
        //...
    }
    // dispatchNestedScroll也向CoordinatorLayout转发了处理
    if (dispatchNestedScroll(consumedX, consumedY, unconsumedX, unconsumedY, mScrollOffset,
            TYPE_TOUCH)) {
        // Update the last touch co-ords, taking any scroll offset into account
        mLastTouchX -= mScrollOffset[0];
        mLastTouchY -= mScrollOffset[1];
        if (ev != null) {
            ev.offsetLocation(mScrollOffset[0], mScrollOffset[1]);
        }
        mNestedOffsets[0] += mScrollOffset[0];
        mNestedOffsets[1] += mScrollOffset[1];
    } else if (getOverScrollMode() != View.OVER_SCROLL_NEVER) {
        if (ev != null && !MotionEventCompat.isFromSource(ev, InputDevice.SOURCE_MOUSE)) {
            pullGlows(ev.getX(), unconsumedX, ev.getY(), unconsumedY);
        }
        considerReleasingGlowsOnScroll(x, y);
    }
    if (consumedX != 0 || consumedY != 0) {
        dispatchOnScrolled(consumedX, consumedY);
    }
    if (!awakenScrollBars()) {
        invalidate();
    }
    return consumedX != 0 || consumedY != 0;
}
~~~
从上面两段代码可以发现在Recyclerview的onTouch事件中，有两处将事件分发了出去，先简单说一下两个分发的流程顺序：
1.dispatchNestedPreScroll流程是:
> Recyclerview.dispatchNestedPreScroll(...) -> NestedScrollingChildHelper.dispatchNestedPreScroll(...)
->ViewParentCompat.onNestedPreScroll()->NestedScrollingParent2.onNestedPreScroll()->CoordinatorLayout.onNestedPreScroll(...)

2.dispatchNestedScroll流程是:
> Recyclerview.dispatchNestedScroll(...) -> NestedScrollingChildHelper.dispatchNestedScroll(...)
->ViewParentCompat.onNestedScroll()->NestedScrollingParent2.onNestedScroll()->CoordinatorLayout.onNestedScroll(...)

#### onNestedPreScroll和onNestedScroll区别
关于这两者的区别，看一下谷歌给出的介绍：
> onNestedPreScroll is called each time the nested scroll is updated by the nested scrolling child, before the nested scrolling child has consumed the scroll distance itself. Each Behavior responding to the nested scroll will receive the same values. The CoordinatorLayout will report as consumed the maximum number of pixels in either direction that any Behavior responding to the nested scroll reported as consumed.

`这里的the nested scrolling child指的是实现了NestedScrollingChild2或NestedScrollingChild接口的view。`

onNestedPreScroll由the nested scrolling child发起，在the nested scrolling child消费滑动距离之前调用这个方法。
任何响应嵌套滑动的Behavior消耗的像素数都是由CoordinatorLayout来转发。

> onNestedScroll is called each time the nested scroll is updated by the nested scrolling child, with both consumed and unconsumed components of the scroll supplied in pixels. Each Behavior responding to the nested scroll will receive the same values.

onNestedScroll也是由the nested scrolling child发起，并且每个响应嵌套滑动的Behavior都会收到相同的消耗和未消耗的像素数。

总结一下就是：
- onNestedPreScroll : 接收**滑动控件**处理滑动前的滑动距离信息，所有Behavior控件并且Behavior的isNestedScrollAccepted()返回true，都可以优先响应滑动操作。消耗部分或者全部滑动距离.
- onNestedScroll : 接收子控件处理完滑动后的滑动距离信息, 在这里外控件可以选择是否处理剩余的滑动距离.

#### onNestedPreScroll流程分析
下面先以onNestedPreScroll为例，分析滑动处理流程。
在CoordinatorLayout的onNestedPreScroll()方法中
~~~ Java
@Override
public void onNestedPreScroll(View target, int dx, int dy, int[] consumed, int  type) {
    int xConsumed = 0;
    int yConsumed = 0;
    boolean accepted = false;

    final int childCount = getChildCount();
    for (int i = 0; i < childCount; i++) {
        final View view = getChildAt(i);
        if (view.getVisibility() == GONE) {
            // If the child is GONE, skip...
            continue;
        }

        final LayoutParams lp = (LayoutParams) view.getLayoutParams();
        if (!lp.isNestedScrollAccepted(type)) {
            // 没有接受事件，这里直接返回
            continue;
        }
        // 将事件转发给各个Behavior
        final Behavior viewBehavior = lp.getBehavior();
        if (viewBehavior != null) {
            mTempIntPair[0] = mTempIntPair[1] = 0;
            viewBehavior.onNestedPreScroll(this, view, target, dx, dy, mTempIntPair, type);

            xConsumed = dx > 0 ? Math.max(xConsumed, mTempIntPair[0])
                    : Math.min(xConsumed, mTempIntPair[0]);
            yConsumed = dy > 0 ? Math.max(yConsumed, mTempIntPair[1])
                    : Math.min(yConsumed, mTempIntPair[1]);

            accepted = true;
        }
    }

    consumed[0] = xConsumed;
    consumed[1] = yConsumed;

    if (accepted) {
        onChildViewsChanged(EVENT_NESTED_SCROLL);
    }
}
~~~
看到这里CoordinatorLayout遍历子view的Behavior，大家应该比较熟悉了。
前面代码CoordinatorLayout的onStartNestedScroll()里有`lp.setNestedScrollAccepted(type, accepted);`这样一句话，由于Recyclerview的Behavior没有重写这个方法，所以accepted的值是false，导致这里`lp.isNestedScrollAccepted(type)`得到的值也是false，所以Recyclerview并不会执行onNestedPreScroll()方法。
只有另一个子View(AppBarLayout) 会触发这个方法：AppBarLayout.Behavior的onNestedPreScroll()
~~~ Java
@Override
public void onNestedPreScroll(CoordinatorLayout coordinatorLayout, AppBarLayout child,
        View target, int dx, int dy, int[] consumed, int type) {
    if (dy != 0) {
        int min, max;
        if (dy < 0) {
            // We're scrolling down
            min = -child.getTotalScrollRange();
            max = min + child.getDownNestedPreScrollRange();
        } else {
            // We're scrolling up
            min = -child.getUpNestedPreScrollRange();
            max = 0;
        }
        if (min != max) {
            // AppBarLayout只支持垂直方向，这里只需要更新Y方向值就可以
            consumed[1] = scroll(coordinatorLayout, child, dy, min, max);
        }
    }
}
~~~
scroll()会调用setHeaderTopBottomOffset()方法，这个方法在前面介绍手指触摸AppBarLayout时已经贴过，这里就不贴了，需要注意的是AppBarLayout.Behavior对setHeaderTopBottomOffset()方法进行了重写。

consumed的值有两种情况：
- 当滑动的距离在minOffset和maxOffset区间之内，则consume!=0，也就说明需要AppBarLayout进行消费，这里对应着AppBarLayout还没移出我们的视线时候的消费情况。
- 当滑动的距离超出了minOffset或者maxOffset后，则consume==0，也就说明需要AppBarLayout不进行消费了，这里对应着AppBarLayout移出我们的视线时候的消费情况。

再看一下刚才CoordinatorLayout.onNestedPreScroll()方法中最后一句`onChildViewsChanged(EVENT_NESTED_SCROLL);`

~~~ Java
final void onChildViewsChanged(@DispatchChangeEvent final int type) {
        //...
        for (int i = 0; i < childCount; i++) {
            //...
            // Get the current draw rect of the view
            getChildRect(child, true, drawRect);
            //...
            if (type != EVENT_VIEW_REMOVED) {
                // Did it change? if not continue
                getLastChildRect(child, lastDrawRect);
                if (lastDrawRect.equals(drawRect)) { //当前view的宽高没有发生变化
                    continue;
                }
                recordLastChildRect(child, drawRect);
            }
            // Update any behavior-dependent views for the change
            for (int j = i + 1; j < childCount; j++) {
                // j 从1开始，checkChild就是Recyclerview
                final View checkChild = mDependencySortedChildren.get(j);
                final LayoutParams checkLp = (LayoutParams) checkChild.getLayoutParams();
                final Behavior b = checkLp.getBehavior();
                // 这里b是AppBarLayout.ScrollingViewBehavior，layoutDependsOn()返回true
                if (b != null && b.layoutDependsOn(this, checkChild, child)) {
                    if (type == EVENT_PRE_DRAW && checkLp.getChangedAfterNestedScroll()) {
                        // If this is from a pre-draw and we have already been changed
                        // from a nested scroll, skip the dispatch and reset the flag
                        checkLp.resetChangedAfterNestedScroll();
                        continue;
                    }

                    final boolean handled;
                    switch (type) {
                        case EVENT_VIEW_REMOVED:
                            // EVENT_VIEW_REMOVED means that we need to dispatch
                            // onDependentViewRemoved() instead
                            b.onDependentViewRemoved(this, checkChild, child);
                            handled = true;
                            break;
                        default:
                            // Otherwise we dispatch onDependentViewChanged()
                            // 传进来的type = EVENT_NESTED_SCROLL
                            handled = b.onDependentViewChanged(this, checkChild, child);
                            break;
                    }

                    if (type == EVENT_NESTED_SCROLL) {
                        // If this is from a nested scroll, set the flag so that we may skip
                        // any resulting onPreDraw dispatch (if needed)
                        checkLp.setChangedAfterNestedScroll(handled);
                    }
                }
            }
        }
    }
~~~

在AppBarLayout.ScrollingViewBehavior的onDependentViewChanged()方法中，会调用其offsetChildAsNeeded()方法
~~~ Java
private void offsetChildAsNeeded(CoordinatorLayout parent, View child, View dependency) {
    final CoordinatorLayout.Behavior behavior =
            ((CoordinatorLayout.LayoutParams) dependency.getLayoutParams()).getBehavior();
    if (behavior instanceof Behavior) {
        // Offset the child, pinning it to the bottom the header-dependency, maintaining
        // any vertical gap and overlap
        final Behavior ablBehavior = (Behavior) behavior;
        ViewCompat.offsetTopAndBottom(child, (dependency.getBottom() - child.getTop())
                + ablBehavior.mOffsetDelta
                + getVerticalLayoutGap()
                - getOverlapPixelsForOffset(dependency));
    }
}
~~~
dependency这里就是AppBarLayout，child就是Recyclerview，而parent当然就是CoordinatorLayout了。
这个方法就是根据AppBarLayout的位置，来调整Recyclerview的位置。如果注释掉这个代码，滑动过程中，Recyclerview左边距位置不会调整，会和AppBarLayout之间出现空白。


#### onNestedScroll流程分析
与上面类似，来看CoordinatorLayout的onNestedScroll()方法：
~~~ Java
@Override
public void onNestedScroll(View target, int dxConsumed, int dyConsumed,
                            int dxUnconsumed, int dyUnconsumed, int type) {
    final int childCount = getChildCount();
    boolean accepted = false;

    for (int i = 0; i < childCount; i++) {
        final View view = getChildAt(i);
        if (view.getVisibility() == GONE) {
            // If the child is GONE, skip...
            continue;
        }
        //这里也是先判断是否接受嵌套滑动
        final LayoutParams lp = (LayoutParams) view.getLayoutParams();
        if (!lp.isNestedScrollAccepted(type)) {
            continue;
        }
        // 分发给每个Behavior
        final Behavior viewBehavior = lp.getBehavior();
        if (viewBehavior != null) {
            viewBehavior.onNestedScroll(this, view, target, dxConsumed, dyConsumed,
                    dxUnconsumed, dyUnconsumed, type);
            accepted = true;
        }
    }

    if (accepted) {
        onChildViewsChanged(EVENT_NESTED_SCROLL);
    }
}
~~~
同样，在我们例子中，Recyclerview的isNestedScrollAccepted返回的是false，所以也不会处理onNestedScroll()逻辑。
因为当前是拖动Recyclerview来移动AppBarLayout，所以Recyclerview没必要处理onNestedScroll()方法.

找到AppBarLayout#Behavior类的onNestedScroll()方法中：
~~~ Java
@Override
public void onNestedScroll(CoordinatorLayout coordinatorLayout, AppBarLayout child,
        View target, int dxConsumed, int dyConsumed, int dxUnconsumed, int dyUnconsumed,
        int type) {
    if (dyUnconsumed < 0) { // 小于0表示view要向下滑动
        // If the scrolling view is scrolling down but not consuming, it's probably be at
        // the top of it's content
        scroll(coordinatorLayout, child, dyUnconsumed,
                -child.getDownNestedScrollRange(), 0);
    }
}
~~~
可以看到这里AppBarLayout处理了垂直方向向下的滑动。

### 水平方向联动
系统的AppBarLayout只支持垂直方向：
~~~ Java
@Override
public void setOrientation(int orientation) {
    if (orientation != VERTICAL) {
        throw new IllegalArgumentException("AppBarLayout is always vertical and does"
                + " not support horizontal orientation");
    }
    super.setOrientation(orientation);
}
~~~
要实现水平方向的AppBarLayout，只需要按照上面的分析，重写对应的方法，具体代码放在了这里[Github agehua/HorizontalCoordinatorDemo](https://github.com/agehua/HorizontalCoordinatorDemo)

### Ref

[CoordinatorLayout三部曲学习之三：AppBarLayout联动源码学习](https://my.oschina.net/u/3863980/blog/1922943)

[安卓自定义View进阶-多点触控详解](https://www.gcssloop.com/customview/multi-touch)
