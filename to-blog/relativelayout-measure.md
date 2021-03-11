- Recyclerview 滑动相关功能总结


RecyclerView scroll 相关API有：
- scrollTo(int x, int y)
- scrollBy(int x, int y)
- scrollToPosition(int position)
- smoothScrollBy(@Px int dx, @Px int dy) 及其重载方法
- smoothScrollToPosition(int position)
- 当然，还有手势滑动


### scrollTo 和 scrollBy
scrollTo 和 scrollBy 是 View 相关方法，实现的是 View 内容的滑动，效果是 View 控件并没有滑动，而是控件上绘制的内容在控件范围内发生了滑动。
看下在 RecyclerView 里面的实现：
~~~ java
// RecyclerView
@Override
public void scrollTo(int x, int y) {
    // RecyclerView 不支持滚动到绝对位置，尝试使用 scrollToPosition 替换
    Log.w(TAG, "RecyclerView does not support scrolling to an absolute position. "
            + "Use scrollToPosition instead");
}

@Override
public void scrollBy(int x, int y) {
    // 这个方法可以配合实现 NestedScrollingChild 的控件联动
    if (mLayout == null) {
        Log.e(TAG, "Cannot scroll without a LayoutManager set. "
                + "Call setLayoutManager with a non-null argument.");
        return;
    }
    if (mLayoutSuppressed) {
        return;
    }
    final boolean canScrollHorizontal = mLayout.canScrollHorizontally();
    final boolean canScrollVertical = mLayout.canScrollVertically();
    if (canScrollHorizontal || canScrollVertical) {
        scrollByInternal(canScrollHorizontal ? x : 0, canScrollVertical ? y : 0, null);
    }
}
~~~

#### scrollByInternal 介绍
~~~ java
// RecyclerView
boolean scrollByInternal(int x, int y, MotionEvent ev) {
    int unconsumedX = 0;
    int unconsumedY = 0;
    int consumedX = 0;
    int consumedY = 0;

    consumePendingUpdateOperations();
    if (mAdapter != null) {
        mReusableIntPair[0] = 0;
        mReusableIntPair[1] = 0;
        // 这里处理滑动
        scrollStep(x, y, mReusableIntPair);
        consumedX = mReusableIntPair[0];
        consumedY = mReusableIntPair[1];
        unconsumedX = x - consumedX;
        unconsumedY = y - consumedY;
    }
    // ... 分发 nestedscroll 结果
    dispatchNestedScroll(consumedX, consumedY, unconsumedX, unconsumedY, mScrollOffset,
            TYPE_TOUCH, mReusableIntPair);
    // ... 通知 onScrolled 事件
    if (consumedX != 0 || consumedY != 0) {
        dispatchOnScrolled(consumedX, consumedY);
    }
    // ...
}
~~~
dispatchOnScrolled 会通知 onScrolled 的监听者

scrollStep方法，如下：
~~~ java
// RecyclerView
void scrollStep(int dx, int dy, @Nullable int[] consumed) {
    // ...
    int consumedX = 0;
    int consumedY = 0;
    if (dx != 0) { // 调用LayoutManager处理滑动
        consumedX = mLayout.scrollHorizontallyBy(dx, mRecycler, mState);
    }
    if (dy != 0) {
        consumedY = mLayout.scrollVerticallyBy(dy, mRecycler, mState);
    }
    // ...
}
~~~

本文以下都以 LinearLayoutManager 为例，继续看scrollHorizontallyBy 方法：

~~~ java
// LinearLayoutManager
int scrollBy(int delta, RecyclerView.Recycler recycler, RecyclerView.State state) {
    if (getChildCount() == 0 || delta == 0) {
        return 0;
    }
    ensureLayoutState();
    mLayoutState.mRecycle = true;
    final int layoutDirection = delta > 0 ? LayoutState.LAYOUT_END : LayoutState.LAYOUT_START;
    final int absDelta = Math.abs(delta);
    updateLayoutState(layoutDirection, absDelta, true, state);
    final int consumed = mLayoutState.mScrollingOffset
            + fill(recycler, mLayoutState, state, false);
    // ...
    final int scrolled = absDelta > consumed ? layoutDirection * consumed : delta;
    mOrientationHelper.offsetChildren(-scrolled);

    mLayoutState.mLastScrollDelta = scrolled;
    return scrolled;
}
~~~

`mOrientationHelper.offsetChildren`，最终会调用到列表里每个View的 offsetLeftAndRight 方法：
~~~ java
// View
public void offsetLeftAndRight(int offset) {
    if (offset != 0) {
        final boolean matrixIsIdentity = hasIdentityMatrix();
        if (matrixIsIdentity) {
            if (isHardwareAccelerated()) {
                invalidateViewProperty(false, false);
            } else {
                final ViewParent p = mParent;
                if (p != null && mAttachInfo != null) {
                    final Rect r = mAttachInfo.mTmpInvalRect;
                    int minLeft;
                    int maxRight;
                    if (offset < 0) {
                        minLeft = mLeft + offset;
                        maxRight = mRight;
                    } else {
                        minLeft = mLeft;
                        maxRight = mRight + offset;
                    }
                    r.set(0, 0, maxRight - minLeft, mBottom - mTop);
                    p.invalidateChild(this, r);
                }
            }
        } else {
            invalidateViewProperty(false, false);
        }

        mLeft += offset;
        mRight += offset;
        mRenderNode.offsetLeftAndRight(offset);
        if (isHardwareAccelerated()) {
            invalidateViewProperty(false, false);
            invalidateParentIfNeededAndWasQuickRejected();
        } else {
            if (!matrixIsIdentity) {
                invalidateViewProperty(false, true);
            }
            invalidateParentIfNeeded();
        }
        notifySubtreeAccessibilityStateChangedIfNeeded();
    }
}
~~~
这里RecyclerView的每个子View都是通过改变子 View 的 mLeft、mTop 等坐标，将初始位置值及偏移量传入，即需要滑动到的位置的坐标完成滑动。
这里的 mLeft、mTop是指基于**父控件**的`视图坐标系`中的坐标

> Android 中有两种坐标系，一种是视图坐标系，以当前控件左上角为坐标原点，向右为 X 轴正方向，向下为 Y 轴正方向，MotionEvent 的 getX()、getY() 方法获取的是点击位置在视图坐标系中的坐标。另一种是 Android 坐标系，以屏幕左上角为坐标原点，向右为 X 轴正方向，向下为 Y 轴正方向，MotionEvent 的 getRawX()、getRawY() 方法获取的是点击位置在 Android 坐标系中的坐标


额外补充一点RecyclerView的布局过程，看下上面的 fill 方法
~~~ java
// LinearLayoutManager
// fill填充方法， 返回的是填充ItemView需要的像素，以便拿去做滚动
int fill(RecyclerView.Recycler recycler, LayoutState layoutState,
        RecyclerView.State state, boolean stopOnFocusable) {
    // 填充起始位置
    final int start = layoutState.mAvailable;
    if (layoutState.mScrollingOffset != LayoutState.SCROLLING_OFFSET_NaN) {
        //如果有滚动就执行一次回收
        recycleByLayoutState(recycler, layoutState);
    }
    // 计算剩余可用的填充空间
    int remainingSpace = layoutState.mAvailable + layoutState.mExtraFillSpace;
    // 用于记录每一次while循环的填充结果
    LayoutChunkResult layoutChunkResult = mLayoutChunkResult;

    // ================== 核心while循环 ====================

    while ((layoutState.mInfinite || remainingSpace > 0) && layoutState.hasMore(state)) {

        // ====== 填充itemView核心填充方法 ====== 屏幕还有剩余可用空间并且还有数据就继续执行
        layoutChunk(recycler, state, layoutState, layoutChunkResult);
        // ...
    }
    // 填充完成后修改起始位置
    return start - layoutState.mAvailable;
}
~~~

这个方法是用来填充内容的，更多布局过程可以参考这篇文章：[《图文详解LinearLayoutManager填充、测量、布局过程》](https://www.jianshu.com/p/e9752f8890c8)

### scrollToPosition
再来看下 scrollToPosition，这个方法会使 RecyclerView 滚动最小的距离，以使目标位置可见，如果目标位置的view没有创建，则滑动不会发生

~~~ java
// RecyclerView.java
public void scrollToPosition(int position) {
    if (mLayoutSuppressed) {
        return;
    }
    stopScroll();
    if (mLayout == null) {
        Log.e(TAG, "Cannot scroll to position a LayoutManager set. "
                + "Call setLayoutManager with a non-null argument.");
        return;
    }
    mLayout.scrollToPosition(position);  // 实际调用的是LayoutManager的对应方法
    awakenScrollBars();
}
~~~
这个方法调用了LayoutManager的同名方法：

~~~ java
// LinearLayoutManager.java
@Override
public void scrollToPosition(int position) {
    // 用 mPendingScrollPosition 变量保存方法传入的位置
    mPendingScrollPosition = position; 
    mPendingScrollPositionOffset = INVALID_OFFSET;
    if (mPendingSavedState != null) {
        mPendingSavedState.invalidateAnchor();
    }
    requestLayout(); // 请求刷新布局
}
~~~

requestLayout()，会重走onMeasure, onLayout过程，在 RecyclerView 的  dispatchLayoutStep1() 中 会调用onLayoutChildren()方法。
同时 LinearLayoutManager 重写了onLayoutChildren方法
~~~ java
// 还是 LinearLayoutManager.java
@Override
public void onLayoutChildren(RecyclerView.Recycler recycler, RecyclerView.State state) {
    // ...
    final View focused = getFocusedChild();
    if (!mAnchorInfo.mValid || mPendingScrollPosition != RecyclerView.NO_POSITION
            || mPendingSavedState != null) {
        // mPendingScrollPosition 值已更新，会进到这里
        mAnchorInfo.reset();
        mAnchorInfo.mLayoutFromEnd = mShouldReverseLayout ^ mStackFromEnd;
        // 调用下面这个方法，计算 anchor 位置和坐标
        updateAnchorInfoForLayout(recycler, state, mAnchorInfo);
        mAnchorInfo.mValid = true;
    } else if (focused != null && (mOrientationHelper.getDecoratedStart(focused)
                    >= mOrientationHelper.getEndAfterPadding()
            || mOrientationHelper.getDecoratedEnd(focused)
            <= mOrientationHelper.getStartAfterPadding())) {
        // This case relates to when the anchor child is the focused view and due to layout
        // shrinking the focused view fell outside the viewport, e.g. when soft keyboard shows
        // up after tapping an EditText which shrinks RV causing the focused view (The tapped
        // EditText which is the anchor child) to get kicked out of the screen. Will update the
        // anchor coordinate in order to make sure that the focused view is laid out. Otherwise,
        // the available space in layoutState will be calculated as negative preventing the
        // focused view from being laid out in fill.
        // Note that we won't update the anchor position between layout passes (refer to
        // TestResizingRelayoutWithAutoMeasure), which happens if we were to call
        // updateAnchorInfoForLayout for an anchor that's not the focused view (e.g. a reference
        // child which can change between layout passes).
        mAnchorInfo.assignFromViewAndKeepVisibleRect(focused, getPosition(focused));
    }

    // ... 如果 mPendingScrollPosition 有效，则会在 prelayout 阶段在用到这个值
    if (state.isPreLayout() && mPendingScrollPosition != RecyclerView.NO_POSITION
            && mPendingScrollPositionOffset != INVALID_OFFSET) {
        // if the child is visible and we are going to move it around, we should layout
        // extra items in the opposite direction to make sure new items animate nicely
        // instead of just fading in
        final View existing = findViewByPosition(mPendingScrollPosition);
        if (existing != null) {
            final int current;
            final int upcomingOffset;
            if (mShouldReverseLayout) {
                current = mOrientationHelper.getEndAfterPadding()
                        - mOrientationHelper.getDecoratedEnd(existing);
                upcomingOffset = current - mPendingScrollPositionOffset;
            } else {
                current = mOrientationHelper.getDecoratedStart(existing)
                        - mOrientationHelper.getStartAfterPadding();
                upcomingOffset = mPendingScrollPositionOffset - current;
            }
            if (upcomingOffset > 0) {
                extraForStart += upcomingOffset;
            } else {
                extraForEnd -= upcomingOffset;
            }
        }
    }
    // ...
}

// 更新 anchorInfo
private boolean updateAnchorFromPendingData(RecyclerView.State state, AnchorInfo anchorInfo) {
    //...
    // 简单理解，如果位置大于itemCount，不会更新 anchorInfo，并且清除 mPendingScrollPosition 的值
    if (mPendingScrollPosition < 0 || mPendingScrollPosition >= state.getItemCount()) {
        mPendingScrollPosition = RecyclerView.NO_POSITION;
        mPendingScrollPositionOffset = INVALID_OFFSET;
        if (DEBUG) {
            Log.e(TAG, "ignoring invalid scroll position " + mPendingScrollPosition);
        }
        return false;
    }
    // ...
}
~~~

上面代码可以看出，如果scrollToPosition(position) 的参数 `position` 所在位置还没有view被layout，则滑动不会被处理。

同时也可以看出 scrollToPosition 是 LayoutManager 通过 `requestLayout` 方式来刷新 ReycclerView，所以并不会通知 onScrollStateChanged

### smoothScrollBy
smoothScrollBy有很多重载方法，直接看最终的：

~~~ java
void smoothScrollBy(@Px int dx, @Px int dy, @Nullable Interpolator interpolator,
        int duration, boolean withNestedScrolling) {
    // ... 前面都是是否能滑动判断
    if (dx != 0 || dy != 0) {
        boolean durationSuggestsAnimation = duration == UNDEFINED_DURATION || duration > 0;
        if (durationSuggestsAnimation) {
            if (withNestedScrolling) {
                int nestedScrollAxis = ViewCompat.SCROLL_AXIS_NONE;
                if (dx != 0) {
                    nestedScrollAxis |= ViewCompat.SCROLL_AXIS_HORIZONTAL;
                }
                if (dy != 0) {
                    nestedScrollAxis |= ViewCompat.SCROLL_AXIS_VERTICAL;
                }
                startNestedScroll(nestedScrollAxis, TYPE_NON_TOUCH);
            }
            mViewFlinger.smoothScrollBy(dx, dy, duration, interpolator);
        } else {
            scrollBy(dx, dy);
        }
    }
}
~~~
最终调用了mViewFlinger.smoothScrollBy 方法：
~~~ java
// ViewFlinger.smoothScrollBy 
public void smoothScrollBy(int dx, int dy, int duration,
            @Nullable Interpolator interpolator) {

        // Handle cases where parameter values aren't defined.
        if (duration == UNDEFINED_DURATION) {
            duration = computeScrollDuration(dx, dy, 0, 0);
        }
        if (interpolator == null) {
            interpolator = sQuinticInterpolator;
        }

        // If the Interpolator has changed, create a new OverScroller with the new
        // interpolator.
        if (mInterpolator != interpolator) {
            mInterpolator = interpolator;
            mOverScroller = new OverScroller(getContext(), interpolator);
        }

        // Reset the last fling information.
        mLastFlingX = mLastFlingY = 0;

        // 调用RecyclerView的setScrollState方法，通知滑动事件开始
        setScrollState(SCROLL_STATE_SETTLING);  
        mOverScroller.startScroll(0, 0, dx, dy, duration);

        if (Build.VERSION.SDK_INT < 23) {
            mOverScroller.computeScrollOffset();
        }

        postOnAnimation();
}
~~~

RecyclerView.setScrollState 最终会通知 onScrollStateChanged 的监听


### 再来看下 smoothScrollToPosition
~~~ java
  @Override
    public void smoothScrollToPosition(RecyclerView recyclerView, RecyclerView.State state,
            int position) {
        LinearSmoothScroller linearSmoothScroller =
                new LinearSmoothScroller(recyclerView.getContext());
        linearSmoothScroller.setTargetPosition(position);
        startSmoothScroll(linearSmoothScroller);
    }
~~~
最终调用的是 mRecyclerView.mViewFlinger.postOnAnimation(); 

mViewFlinger 是一个 Runnable 对象，在其run()方法中，调用了dispatchOnScrolled(consumedX, consumedY)，通知了onScrollStateChanged()





### scrollToPositionWithOffset

RecyclerView#addOnScrollListener(OnScrollListener) 监听滑动事件

RecyclerView 调用这个方法通知layoutmanager，scroll状态已经改变
public void onScrollStateChanged(int state) {
    if (state == SCROLL_STATE_IDLE) {
        //滑动停止
    }
}
onScrollStateChanged 调用的时机有限制


ViewFlinger 会调用这个方法，包括 fling ，smoothScrollBy 操作
~~~ java
// 还要其他同名方法，这里就不一一列举了
public void smoothScrollBy(@Px int dx, @Px int dy) {
    smoothScrollBy(dx, dy, null);
}
~~~

