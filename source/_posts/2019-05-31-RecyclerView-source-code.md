---
layout: post
title: RecyclerView 源码分析
category: accumulation
tags:
    - RecyclerView
keywords: RecyclerView
banner: http://cdn.conorlee.top/Farmhouse%20in%20a%20Wheat%20Field.jpg
thumbnail: http://cdn.conorlee.top/Farmhouse%20in%20a%20Wheat%20Field.jpg
toc: true
---

> 本文承接上文[《RecyclerView 中的设计模式》](/2019/05/30/RecyclerView-Design-Pattern/)，结合源码分析Recyclerview绘制、滑动、和缓存等逻辑

<!--more-->
RecyclerView的代码设计结构，如下面两张图：

![RecyclerView的代码设计结构](/images/blogimages/2019/Recyclerview-source-code.png)
![RecyclerView的代码设计结构](/images/blogimages/2019/Recyclerview-source-code1.png)

- RecyclerViewDataObserver 数据观察器
- Recycler View循环复用系统，核心部件
- SavedState RecyclerView状态
- AdapterHelper 适配器更新
- ChildHelper 管理子View
- ViewInfoStore 存储子VIEW的动画信息
- Adapter 数据适配器
- LayoutManager 负责子VIEW的布局，核心部件
- ItemAnimator Item动画
- ViewFlinger 快速滑动管理
- NestedScrollingChildHelper 管理子VIEW嵌套滑动

### 绘制详情
可见RecyclerView涉及的类相当多，所以看代码的时候很容易迷失。因此我们需要抽丝剥茧，按照主线来进行分析。一般我们使用的时候是这样的

~~~ Java
recyclerView = (RecyclerView) findViewById(R.id.recyclerView);  
LinearLayoutManager layoutManager = new LinearLayoutManager(this);  
//设置布局管理器  
recyclerView.setLayoutManager(layoutManager);  
//设置为垂直布局，这也是默认的  
layoutManager.setOrientation(OrientationHelper. VERTICAL);  
//设置Adapter  
recyclerView.setAdapter( recycleAdapter);  
 //设置分隔线  
recyclerView.addItemDecoration( new DividerGridItemDecoration(this ));  
//设置增加或删除条目的动画  
recyclerView.setItemAnimator( new DefaultItemAnimator());
~~~
首先recyclerView = (RecyclerView) findViewById(R.id.recyclerView);会执行其构造方法,我们看一下干了些什么事:

~~~ Java
public RecyclerView(Context context, @Nullable AttributeSet attrs, int defStyle) {
    super(context, attrs, defStyle);
    setScrollContainer(true);
    setFocusableInTouchMode(true);
    final int version = Build.VERSION.SDK_INT;
    mPostUpdatesOnAnimation = version >= 16;
    final ViewConfiguration vc = ViewConfiguration.get(context);
    mTouchSlop = vc.getScaledTouchSlop();
    mMinFlingVelocity = vc.getScaledMinimumFlingVelocity();
    mMaxFlingVelocity = vc.getScaledMaximumFlingVelocity();
    setWillNotDraw(ViewCompat.getOverScrollMode(this) == ViewCompat.OVER_SCROLL_NEVER);
    mItemAnimator.setListener(mItemAnimatorListener);
    initAdapterManager();
    initChildrenHelper();
    // If not explicitly specified this view is important for accessibility.
    if (ViewCompat.getImportantForAccessibility(this)
            == ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_AUTO) {
        ViewCompat.setImportantForAccessibility(this,
                ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_YES);
    }
    mAccessibilityManager = (AccessibilityManager) getContext()
            .getSystemService(Context.ACCESSIBILITY_SERVICE);
    setAccessibilityDelegateCompat(new RecyclerViewAccessibilityDelegate(this));
    // Create the layoutManager if specified.
    boolean nestedScrollingEnabled = true;
    if (attrs != null) {
        int defStyleRes = 0;
        //获取布局属性值
        TypedArray a = context.obtainStyledAttributes(attrs, R.styleable.RecyclerView,
                defStyle, defStyleRes);
        String layoutManagerName = a.getString(R.styleable.RecyclerView_layoutManager);
        a.recycle();
        createLayoutManager(context, layoutManagerName, attrs, defStyle, defStyleRes);
        if (Build.VERSION.SDK_INT >= 21) {
            a = context.obtainStyledAttributes(attrs, NESTED_SCROLLING_ATTRS,
                    defStyle, defStyleRes);
            nestedScrollingEnabled = a.getBoolean(0, true);
            a.recycle();
        }
    }
    // Re-set whether nested scrolling is enabled so that it is set on all API levels
    setNestedScrollingEnabled(nestedScrollingEnabled);
}
~~~
代码进行了一系列的初始化工作,关键是createLayoutManager,创建了一个布局管理器

~~~ Java
private void createLayoutManager(Context context, String className, AttributeSet attrs,
        int defStyleAttr, int defStyleRes) {
    //如果布局属性存在
    if (className != null) {
        className = className.trim();
        if (className.length() != 0) {  // Can't use isEmpty since it was added in API 9.
            className = getFullClassName(context, className);
            try {
                ClassLoader classLoader;
                if (isInEditMode()) {
                    // Stupid layoutlib cannot handle simple class loaders.
                    classLoader = this.getClass().getClassLoader();
                } else {
                    classLoader = context.getClassLoader();
                }
                //根据布局属性值设置的layoutManager通过反射实例化layoutManager
                Class layoutManagerClass =
                        classLoader.loadClass(className).asSubclass(LayoutManager.class);
                Constructor constructor;
                Object[] constructorArgs = null;
                try {
                    constructor = layoutManagerClass
                            .getConstructor(LAYOUT_MANAGER_CONSTRUCTOR_SIGNATURE);
                    constructorArgs = new Object[]{context, attrs, defStyleAttr, defStyleRes};
                } catch (NoSuchMethodException e) {
                    try {
                        constructor = layoutManagerClass.getConstructor();
                    } catch (NoSuchMethodException e1) {
                        e1.initCause(e);
                        throw new IllegalStateException(attrs.getPositionDescription() +
                                ": Error creating LayoutManager " + className, e1);
                    }
                }
                constructor.setAccessible(true);
                setLayoutManager(constructor.newInstance(constructorArgs));
            } catch (ClassNotFoundException e) {
                throw new IllegalStateException(attrs.getPositionDescription()
                        + ": Unable to find LayoutManager " + className, e);
            } catch (InvocationTargetException e) {
                throw new IllegalStateException(attrs.getPositionDescription()
                        + ": Could not instantiate the LayoutManager: " + className, e);
            } catch (InstantiationException e) {
                throw new IllegalStateException(attrs.getPositionDescription()
                        + ": Could not instantiate the LayoutManager: " + className, e);
            } catch (IllegalAccessException e) {
                throw new IllegalStateException(attrs.getPositionDescription()
                        + ": Cannot access non-public constructor " + className, e);
            } catch (ClassCastException e) {
                throw new IllegalStateException(attrs.getPositionDescription()
                        + ": Class is not a LayoutManager " + className, e);
            }
        }
    }
}
~~~
如果在布局文件里面设置了布局管理器的类型，那么这里会通过反射的方式实例化出对应的布局管理器。最后将实例化出的布局管理器设置到当前的RecyclerView,参考文章在创建实例时候
~~~ Java
public void setLayoutManager(LayoutManager layout) {
    if (layout == mLayout) {
        return;
    }
    stopScroll();
    // TODO We should do this switch a dispachLayout pass and animate children. There is a good
    // chance that LayoutManagers will re-use views.
    if (mLayout != null) {
        if (mIsAttached) {
            mLayout.dispatchDetachedFromWindow(this, mRecycler);
        }
        mLayout.setRecyclerView(null);
    }
    mRecycler.clear();
    mChildHelper.removeAllViewsUnfiltered();
    mLayout = layout;
    if (layout != null) {
        if (layout.mRecyclerView != null) {
            throw new IllegalArgumentException("LayoutManager " + layout +
                    " is already attached to a RecyclerView: " + layout.mRecyclerView);
        }
        mLayout.setRecyclerView(this);
        if (mIsAttached) {
            mLayout.dispatchAttachedToWindow(this);
        }
    }
    requestLayout();
}
~~~
设置布局管理器之前会先清空所有之前的缓存VIEW。最后通知VIEW刷新,requestLayout可见要绘制了
~~~ Java
public void requestLayout() {
    if (mMeasureCache != null) mMeasureCache.clear();
    if (mAttachInfo != null && mAttachInfo.mViewRequestingLayout == null) {
        // Only trigger request-during-layout logic if this is the view requesting it,
        // not the views in its parent hierarchy
        ViewRootImpl viewRoot = getViewRootImpl();
        if (viewRoot != null && viewRoot.isInLayout()) {
            if (!viewRoot.requestLayoutDuringLayout(this)) {
                return;
            }
        }
        mAttachInfo.mViewRequestingLayout = this;
    }
    //为当前view设置标记位 PFLAG_FORCE_LAYOUT
    mPrivateFlags |= PFLAG_FORCE_LAYOUT;
    mPrivateFlags |= PFLAG_INVALIDATED;
    if (mParent != null && !mParent.isLayoutRequested()) {
        //向父容器请求布局
        mParent.requestLayout();
    }
    if (mAttachInfo != null && mAttachInfo.mViewRequestingLayout == this) {
        mAttachInfo.mViewRequestingLayout = null;
    }
}
~~~

![](/images/blogimages/2019/view-requestlayout.jpg)

> 在requestLayout方法中，首先先判断当前View树是否正在布局流程，接着为当前子View设置标记位，该标记位的作用就是标记了当前的View是需要进行重新布局的，接着调用mParent.requestLayout方法，这个十分重要，因为这里是向父容器请求布局，即调用父容器的requestLayout方法，为父容器添加PFLAG_FORCE_LAYOUT标记位，而父容器又会调用它的父容器的requestLayout方法，即requestLayout事件层层向上传递，直到DecorView，即根View，而根View又会传递给ViewRootImpl，也即是说子View的requestLayout事件，最终会被ViewRootImpl接收并得到处理。
纵观这个向上传递的流程，其实是采用了责任链模式，即不断向上传递该事件，直到找到能处理该事件的上级，在这里，只有ViewRootImpl能够处理requestLayout事件。

~~~ java
@Override
public void requestLayout() {
    if (!mHandlingLayoutInLayoutRequest) {
        checkThread();
        mLayoutRequested = true;
        scheduleTraversals();
    }
}
void scheduleTraversals() {
    if (!mTraversalScheduled) {
        mTraversalScheduled = true;
        mTraversalBarrier = mHandler.getLooper().getQueue().postSyncBarrier();
        mChoreographer.postCallback(
                Choreographer.CALLBACK_TRAVERSAL, mTraversalRunnable, null);
        if (!mUnbufferedInputDispatch) {
            scheduleConsumeBatchedInput();
        }
        notifyRendererOfFramePending();
        pokeDrawLockIfNeeded();
    }
}
final class TraversalRunnable implements Runnable {
    @Override
    public void run() {
        doTraversal();
    }
}
void doTraversal() {
    if (mTraversalScheduled) {
        mTraversalScheduled = false;
        mHandler.getLooper().getQueue().removeSyncBarrier(mTraversalBarrier);
        if (mProfile) {
            Debug.startMethodTracing("ViewAncestor");
        }
        performTraversals();
        if (mProfile) {
            Debug.stopMethodTracing();
            mProfile = false;
        }
    }
}
~~~

在这里，调用了scheduleTraversals方法，这个方法是一个异步方法，最终会调用到ViewRootImpl#performTraversals方法，这也是View工作流程的核心方法，在这个方法内部，分别调用measure、layout、draw方法来进行View的三大工作流程
~~~ Java
private void performTraversals() {
        ...
    if (!mStopped) {
        int childWidthMeasureSpec = getRootMeasureSpec(mWidth, lp.width);  // 1
        int childHeightMeasureSpec = getRootMeasureSpec(mHeight, lp.height);
        performMeasure(childWidthMeasureSpec, childHeightMeasureSpec);       
        }
    }
    if (didLayout) {
        performLayout(lp, desiredWindowWidth, desiredWindowHeight);
        ...
    }
    if (!cancelDraw && !newSurface) {
        if (!skipDraw || mReportNextDraw) {
            if (mPendingTransitions != null && mPendingTransitions.size() > 0) {
                for (int i = 0; i < mPendingTransitions.size(); ++i) {
                    mPendingTransitions.get(i).startChangingAnimations();
                }
                mPendingTransitions.clear();
            }
            performDraw();
        }
    }
    ...
}
~~~

这里很熟悉了吧,view,viewgroup的绘制,如果这里有问题的,自己百度吧,所以会最终调用到recyclerview的onMeature

recyclerView.setLayoutManager(layoutManager)就是桥接模式的体现，因为layoutManager的实现可以有多种,即桥接模式具体实现化逻辑ConcreteImplementor

- ListView功能 recyclerView.setLayoutManager(new LinearLayoutManager(this));
- GridView功能 recyclerView.setLayoutManager(new GridLayoutManager(this,3));
- 瀑布流形式功能 recyclerView.setLayoutManager(new StaggeredGridLayoutManager(2,StaggeredGridLayoutManager.VERTICAL));
- 横向ListView的功能 recyclerView.setLayoutManager(new LinearLayoutManager(this)); layoutManager.setOrientation(…);

~~~ Java
public LinearLayoutManager(Context context, int orientation, boolean reverseLayout) {
    setOrientation(orientation);
    setReverseLayout(reverseLayout);
    setAutoMeasureEnabled(true);
}
public StaggeredGridLayoutManager(Context context, AttributeSet attrs, int defStyleAttr,
        int defStyleRes) {
    Properties properties = getProperties(context, attrs, defStyleAttr, defStyleRes);
    setOrientation(properties.orientation);
    setSpanCount(properties.spanCount);
    setReverseLayout(properties.reverseLayout);
    setAutoMeasureEnabled(mGapStrategy != GAP_HANDLING_NONE);
    mLayoutState = new LayoutState();
    createOrientationHelpers();
}
~~~

`GridLayoutManager继承LinearLayoutManager`

可见其初始化时候会设置AutoMeasurEnabled,前面说过，RecyclerView会将测量与布局交给LayoutManager来做，并且LayoutManager有一个叫做mAutoMeasure的属性，这个属性用来控制LayoutManager是否开启自动测量，开启自动测量的话布局就交由RecyclerView使用一套默认的测量机制，否则，自定义的LayoutManager需要重写onMeasure来处理自身的测量工作。RecyclerView目前提供的几种LayoutManager都开启了自动测量，所以这里我们关注一下自动测量部分的逻辑：

~~~ Java
protected void onMeasure(int widthSpec, int heightSpec) {
    ...
    if (mLayout.mAutoMeasure) {
        final int widthMode = MeasureSpec.getMode(widthSpec);
        final int heightMode = MeasureSpec.getMode(heightSpec);
        final boolean skipMeasure = widthMode == MeasureSpec.EXACTLY
                && heightMode == MeasureSpec.EXACTLY;
        mLayout.onMeasure(mRecycler, mState, widthSpec, heightSpec);
        if (skipMeasure || mAdapter == null) {
            return;
        }
        if (mState.mLayoutStep == State.STEP_START) {
            dispatchLayoutStep1();
        }
        mLayout.setMeasureSpecs(widthSpec, heightSpec);
        mState.mIsMeasuring = true;
        dispatchLayoutStep2();
        mLayout.setMeasuredDimensionFromChildren(widthSpec, heightSpec);
        if (mLayout.shouldMeasureTwice()) {
            mLayout.setMeasureSpecs(
                    MeasureSpec.makeMeasureSpec(getMeasuredWidth(), MeasureSpec.EXACTLY),
                    MeasureSpec.makeMeasureSpec(getMeasuredHeight(), MeasureSpec.EXACTLY));
            mState.mIsMeasuring = true;
            dispatchLayoutStep2();
            mLayout.setMeasuredDimensionFromChildren(widthSpec, heightSpec);
        }
    }
    ...
}
~~~

自动测量的原理如下:当RecyclerView的宽高都为EXACTLY时，可以直接设置对应的宽高，然后返回，结束测量。

#### 补充MeasureSpect三种模式
三种模式是EXACTLY,UNSPECIFIED,AT_MOST,分别代表精确大小,不精确大小,最大值;通过MeasureSpect.getMode就可以获得该值,那么MeasureSpect到底是由什么决定的呢?MeasureSpect是由LayoutParameter通过父容器的施加的规则产生的下面我们来看一看三种模式产生的情况.

- MeasureSpec.EXACTLY父容器已经精确的检测出了子View的大小,子view的大小就是MeasureSpect.getSize()的值.适用情况:
  - a.子View的LayoutParameter使用具体的值(如:宽高为100dp),不管父容器的spectMode为什么,系统返回给子View的mode为EXACTLY,系统返回给子View的大小为子V诶额外自己指定的大小(100dp)    
  - b.子View的LayoutParams采用match_parent并且父容器的mode为EXACTLY,那么子View的mode即为EXACTLY,子View大小为父容器剩余的大小
- MeasureSpec.AT_MOST父容器期望对子View的最大值做了限定适用情况:    
  - c.子View的LayoutParams采用match_parent并且父容器的mode为AT_MOST,那么子View的mode即为AT_MOST,子View大小为父容器剩余的大小    
  - d.当子View的LayoutParams采用wrap_content时并且父容器的mode为EXACTLY或者AT_MOST时,子View的Mode就为AT_MOST，子View的specSize就为该父容器剩余的大小
- MeasureSpec.UNSPECIFIED父容器不限定大小,子View想多大就多大适应情况:    
  - e.当子View的LayoutParams采用wrap_content时并且父容器的mode为UNSPECIFIED时,子View的Mode就为UNSPECIFIED，子View的大小不做限制

如果宽高的测量规则不是EXACTLY的,则会在onMeasure()中开始布局的处理，这里首先要介绍一个很重要的类：

**RecyclerView.State** ，这个类封装了当前RecyclerView的有用信息。State的一个变量mLayoutStep表示了RecyclerView当前的布局状态，包括STEP_START、STEP_LAYOUT 、 STEP_ANIMATIONS三个，而RecyclerView的布局过程也分为三步，其中，STEP_START表示即将开始布局，需要调用dispatchLayoutStep1来执行第一步布局，接下来，布局状态变为STEP_LAYOUT，表示接下来需要调用dispatchLayoutStep2里进行第二步布局，同理，第二步布局后状态变为STEP_ANIMATIONS，需要执行第三步布局dispatchLayoutStep3。

这三个步骤的工作也各不相同，step1负责记录状态，step2负责布局，step3则与step1进行比较，根据变化来触发动画。

RecyclerView将布局划分的如此细致必然是有其原因的，在开启自动测量模式的情况，RecyclerView是支持WRAP_CONTENT属性的，比如我们可以很容易的在RecyclerView的下面放置其它的View，RecyclerView会根据子View所占大小动态调整自己的大小，这时候，RecyclerView就会将子控件的measure与layout提前到Recycler的onMeasure中，因为它需要确定子空间的大小与位置后，再来设置自己的大小。所以这时候就会在onMeasure中完成step1与step2。否则，就需要在onLayout中去完成整个布局过程。

综上，整个mLayout.mAutoMeasure就是在做前两步的布局，可见RecylerView的measure与layout是紧密相关的，所以我们来赶快瞧一瞧RecyclerView是如何layout的。

我们直接看下onLayout的代码：
~~~ Java
protected void onLayout(boolean changed, int l, int t, int r, int b) {
    TraceCompat.beginSection(TRACE_ON_LAYOUT_TAG);
    dispatchLayout();
    TraceCompat.endSection();
    mFirstLayoutComplete = true;
}
~~~
直接追进dispatchLayout：
~~~ Java
void dispatchLayout() {
    ...
    mState.mIsMeasuring = false;
    if (mState.mLayoutStep == State.STEP_START) {
        dispatchLayoutStep1();
        mLayout.setExactMeasureSpecsFrom(this);
        dispatchLayoutStep2();
    } else if (mAdapterHelper.hasUpdates() || mLayout.getWidth() != getWidth() ||mLayout.getHeight() != getHeight()) {
        // First 2 steps are done in onMeasure but looks like we have to run again due to
        // changed size.
        mLayout.setExactMeasureSpecsFrom(this);
        dispatchLayoutStep2();
    } else {
        // always make sure we sync them (to ensure mode is exact)
        mLayout.setExactMeasureSpecsFrom(this);
    }
    dispatchLayoutStep3();
}
~~~
通过查看dispatchLayout的代码正好验证了我们前文关于RecyclerView的layout三步走原则，如果在onMeasure中已经完成了step1与step2，则只会执行step3，否则三步会依次触发。接下来我们一步一步的进行分析

#### dispatchLayoutStep1
~~~ Java
private void dispatchLayoutStep1(){   
    ...
    if (mState.mRunSimpleAnimations) {
          int count = mChildHelper.getChildCount();
          for (int i = 0; i < count; ++i) {
              final ViewHolder holder = getChildViewHolderInt(mChildHelper.getChildAt(i));
              final ItemHolderInfo animationInfo = mItemAnimator
                      .recordPreLayoutInformation(mState, holder,
                              ItemAnimator.buildAdapterChangeFlagsForAnimations(holder),
                              holder.getUnmodifiedPayloads());
              mViewInfoStore.addToPreLayout(holder, animationInfo);
              ...
          }
    }
    ...
    mState.mLayoutStep = State.STEP_LAYOUT;
}
class ViewInfoStore {
  private static final boolean DEBUG = false;
  @VisibleForTesting
  final ArrayMap mLayoutHolderMap = new ArrayMap<>();
  @VisibleForTesting
  final LongSparseArray mOldChangedHolders = new LongSparseArray<>();
  void clear() {
      mLayoutHolderMap.clear();
      mOldChangedHolders.clear();
  }
}
public static class ItemHolderInfo {
    public int left;
    public int top;
    public int right;
    public int bottom;
    @AdapterChanges
    public int changeFlags;
    public ItemHolderInfo() {
    }
}
~~~

step的第一步目的就是在记录View的状态，首先遍历当前所有的View依次进行处理，mItemAnimator会根据每个View的信息封装成一个ItemHolderInfo，这个ItemHolderInfo中主要包含的就是当前View的位置状态等。然后ItemHolderInfo 就被存入mViewInfoStore中,由代码可见被存在ArrayMap和LongSparseArray中,其是对HashMap的android优化,是用两个数组来完成存储,arraymap的key可以是任意值,SparseArray的key只能为int,其核心是折半查找

注意这里调用的是mViewInfoStore的addToPreLayout方法，我们追进：
~~~ Java
void addToPreLayout(ViewHolder holder, ItemHolderInfo info) {
    InfoRecord record = mLayoutHolderMap.get(holder);
    if (record == null) {
        record = InfoRecord.obtain();
        mLayoutHolderMap.put(holder, record);
    }
    record.preInfo = info;
    record.flags |= FLAG_PRE;
}
~~~
addToPreLayout方法中会根据holder来查询InfoRecord信息，如果没有，则生成，然后将info信息赋值给InfoRecord的preInfo变量。最后标记FLAG_PRE信息，如此，完成函数。所以纵观整个layout的第一步，就是在记录当前的View信息，因为进入第二步后，View的信息就将被改变了。我们来看第二步：

#### dispatchLayoutStep2
~~~ Java
private void dispatchLayoutStep2() {
    ...
    mLayout.onLayoutChildren(mRecycler, mState);
     ...
    mState.mLayoutStep = State.STEP_ANIMATIONS;
}
~~~
layout的第二步主要就是真正的去布局View了，前面也说过，RecyclerView的布局是由LayoutManager负责的，所以第二步的主要工作也都在LayoutManager中，由于每种布局的方式不一样，这里我们以常见的**LinearLayoutManager**为例。我们看其onLayoutChildren方法：

~~~ Java
public void onLayoutChildren(RecyclerView.Recycler recycler, RecyclerView.State state) {    
    ...
    if (!mAnchorInfo.mValid || mPendingScrollPosition != NO_POSITION ||
            mPendingSavedState != null) {
        updateAnchorInfoForLayout(recycler, state, mAnchorInfo);
    }
    ...
    if (mAnchorInfo.mLayoutFromEnd) {
        firstLayoutDirection = mShouldReverseLayout ? LayoutState.ITEM_DIRECTION_TAIL :
                LayoutState.ITEM_DIRECTION_HEAD;
    } else {
        firstLayoutDirection = mShouldReverseLayout ? LayoutState.ITEM_DIRECTION_HEAD :
                LayoutState.ITEM_DIRECTION_TAIL;
    }
    ...
    onAnchorReady(recycler, state, mAnchorInfo, firstLayoutDirection);
    ...
    if (mAnchorInfo.mLayoutFromEnd) {
       ...
    } else {
        // fill towards end
        updateLayoutStateToFillEnd(mAnchorInfo);
        fill(recycler, mLayoutState, state, false);
          ...     
        // fill towards start
        updateLayoutStateToFillStart(mAnchorInfo);
          ...
        fill(recycler, mLayoutState, state, false);
        ...
    }
    ...
}
~~~
整个onLayoutChildren过程还是很复杂的，这里我尽量省略了一些与流程关系不大的细节处理代码。整个onLayoutChildren过程可以大致整理如下：

- 找到anchor点
- 根据anchor一直向前布局，直至填充满anchor点前面的所有区域
- 根据anchor一直向后布局，直至填充满anchor点后面的所有区域这里我以垂直布局来说明，mAnchorInfo为布局锚点信息，包含了子控件在Y轴上起始绘制偏移量（coordinate），ItemView在Adapter中的索引位置（position）和布局方向（mLayoutFromEnd）——这里是指start、end方向。

这部分代码的功能就是：确定布局锚点，以此为起点向开始和结束方向填充ItemView，如图所示：

![onLayoutChildren](/images/blogimages/2019/Recyclerview-find_anchor.png)

anchor点的寻找是由updateAnchorInfoForLayout函数负责的：
~~~ Java
private void updateAnchorInfoForLayout(RecyclerView.Recycler recycler, RecyclerView.State state,
                                       AnchorInfo anchorInfo) {
    ...
    if (updateAnchorFromChildren(recycler, state, anchorInfo)) {
        return;
    }
    ...
    anchorInfo.assignCoordinateFromPadding();
    anchorInfo.mPosition = mStackFromEnd ? state.getItemCount() - 1 : 0;
}
~~~
函数内首先通过子view来获取anchor，如果没有获取到，就根据就取头/尾点来作为anchor。所以这里我们主要关注updateAnchorFromChildren函数：

~~~ Java
private boolean updateAnchorFromChildren(RecyclerView.Recycler recycler,
                                         RecyclerView.State state, AnchorInfo anchorInfo) {
    if (getChildCount() == 0) {
        return false;
    }
    final View focused = getFocusedChild();
    if (focused != null && anchorInfo.isViewValidAsAnchor(focused, state)) {
        anchorInfo.assignFromViewAndKeepVisibleRect(focused);
        return true;
    }
    if (mLastStackFromEnd != mStackFromEnd) {
        return false;
    }
    View referenceChild = anchorInfo.mLayoutFromEnd
            ? findReferenceChildClosestToEnd(recycler, state)
            : findReferenceChildClosestToStart(recycler, state);
    if (referenceChild != null) {
        anchorInfo.assignFromView(referenceChild);
        ...
        return true;
    }
    return false;
}
~~~
updateAnchorFromChildren内部做的事情也很容易理解，首先寻找被focus的child，找到的话以此child作为anchor，否则根据布局的方向寻找最合适的child来作为anchor，如果找到则将child的信息赋值给anchorInfo，其实anchorInfo主要记录的信息就是view的物理位置与在adapter中的位置。找到后返回true，否则返回false则交由上一步的函数做处理。

综上，刚刚的所追踪的代码都是在寻找anchor点。在我们寻找后，LinearLayoutManager还给了我们更改anchor的时机，就是 onAnchorReady 函数，我们可以继承LinearLayoutManager 来重写onAnchorReady方法，就可以实现某些特定的功能，比如进入RecyclerView时定位在某一项等等。

总之，我们现在找到了anchor信息，接下来就是根据anchor来布局了。无论从上到下还是从下到上布局，都调用的是fill方法，我们进入fill方法来查看一番：
~~~ Java
int fill(RecyclerView.Recycler recycler, LayoutState layoutState,
        RecyclerView.State state, boolean stopOnFocusable) {
    final int start = layoutState.mAvailable;
    if (layoutState.mScrollingOffset != LayoutState.SCROLLING_OFFSET_NaN) {
        recycleByLayoutState(recycler, layoutState);
    }
    int remainingSpace = layoutState.mAvailable + layoutState.mExtra;
    LayoutChunkResult layoutChunkResult = mLayoutChunkResult;
    while ((layoutState.mInfinite || remainingSpace > 0) && layoutState.hasMore(state)) {
        layoutChunk(recycler, state, layoutState, layoutChunkResult);
        ...
    }
    return start - layoutState.mAvailable;
}
~~~
这里同样省略了很多代码，我们关注重点：

首先比较重要的函数是recycleByLayoutState，这个函数就厉害了，它会根据当前信息对不需要的View进行回收：

~~~ Java
private void recycleByLayoutState(RecyclerView.Recycler recycler, LayoutState layoutState) {
     if (layoutState.mLayoutDirection == LayoutState.LAYOUT_START) {
            ...
     } else {
         recycleViewsFromStart(recycler, layoutState.mScrollingOffset);
     }
}
~~~
我们继续追进recycleViewsFromStart：
~~~ Java
private void recycleViewsFromStart(RecyclerView.Recycler recycler, int dt) {
    final int limit = dt;
    final int childCount = getChildCount();
    if (mShouldReverseLayout) {
        ...
    } else {
        for (int i = 0; i < childCount; i++) {
            View child = getChildAt(i);
            if (mOrientationHelper.getDecoratedEnd(child) > limit
                    || mOrientationHelper.getTransformedEndWithDecoration(child) > limit) {
                recycleChildren(recycler, 0, i);
                return;
            }
        }
    }
}
~~~
这个函数的作用就是遍历所有的子View，找出逃离边界的View进行回收，回收函数我们锁定在recycleChildren里，而这个函数最后又会调到removeAndRecycleViewAt：
~~~ Java
public void removeAndRecycleViewAt(int index, Recycler recycler) {
    final View view = getChildAt(index);
    removeViewAt(index);
    recycler.recycleView(view);
}
~~~
这个函数首先调用removeViewAt函数，这个函数的作用是将View从RecyclerView中移除， 紧接着我们看到，是recycler执行了view的回收逻辑。这里我们暂且打住，关于recycler我们会单独进行说明，这里我们只需要理解，在fill函数的一开始会去回收逃离出屏幕的view。我们再次回到fill函数，关注这里：

~~~ Java
while ((layoutState.mInfinite || remainingSpace > 0) && layoutState.hasMore(state)) {
      layoutChunk(recycler, state, layoutState, layoutChunkResult);
        ...
}
~~~
这段代码很容易理解，只要还有剩余空间，就会执行layoutChunk方法：
~~~ Java
void layoutChunk(RecyclerView.Recycler recycler, RecyclerView.State state,
        LayoutState layoutState, LayoutChunkResult result) {
    View view = layoutState.next(recycler);
    ...
    LayoutParams params = (LayoutParams) view.getLayoutParams();
    if (layoutState.mScrapList == null) {
        if (mShouldReverseLayout == (layoutState.mLayoutDirection
                == LayoutState.LAYOUT_START)) {
            addView(view);
        } else {
            addView(view, 0);
        }
    } else {
       ...
    }
    ...
    layoutDecoratedWithMargins(view, left, top, right, bottom);
    ...
}
~~~
我们首先看到，layoutState的next方法返回了一个View，凭空变出一个View，好神奇，追进去看一下：
~~~ Java
View next(RecyclerView.Recycler recycler) {
    ...
    final View view = recycler.getViewForPosition(mCurrentPosition);
    return view;
}
~~~
可见，view的获取逻辑也由recycler来负责，所以，这里我们同样打住，只需要清楚recycler可以根据位置返回一个view即可。

再回到layoutChunk看一下对刚刚生成的view作何处理：
~~~ Java
if (mShouldReverseLayout == (layoutState.mLayoutDirection
            == LayoutState.LAYOUT_START)) {
        addView(view);
    } else {
        addView(view, 0);
}
~~~
很明显调用了addView方法，虽然这个方法是LayoutManager的，但这个方法最终会多次辗转调用到RecyclerView的addView方法，将view添加在RecyclerView中。综上，我们就梳理了整个第二步布局的过程，此过程完成了子View的测量与布局，任务还是相当繁重。

#### dispatchLayoutStep3

接下来，就到了布局的最后一步了，我们直接看下dispatchLayoutStep3方法：
~~~ Java
private void dispatchLayoutStep3() {
    mState.mLayoutStep = State.STEP_START;
    if (mState.mRunSimpleAnimations) {
        for (int i = mChildHelper.getChildCount() - 1; i >= 0; i--) {
            ...
            final ItemHolderInfo animationInfo = mItemAnimator
                    .recordPostLayoutInformation(mState, holder);
                mViewInfoStore.addToPostLayout(holder, animationInfo);
        }
        mViewInfoStore.process(mViewInfoProcessCallback);
    }
    ...
}
~~~
这一步是与第一步呼应的的，此时由于子View都已完成布局，所以子View的信息都发生了变化。我们会看到第一步出现的mViewInfoStore和mItemAnimator再次登场，这次mItemAnimator调用的是recordPostLayoutInformation方法，而mViewInfoStore调用的是addToPostLayout方法，还记得刚刚我强调的吗，之前是pre，也就是真正布局之前的状态，而现在要记录布局之后的状态，我们追进addToPostLayout：
~~~ Java
void addToPostLayout(ViewHolder holder, ItemHolderInfo info) {
    InfoRecord record = mLayoutHolderMap.get(holder);
    if (record == null) {
        record = InfoRecord.obtain();
        mLayoutHolderMap.put(holder, record);
    }
    record.postInfo = info;
    record.flags |= FLAG_POST;
}
~~~
和第一步的addToPreLayout类似，不过这次info信息被赋值给了record的postInfo变量，这样，一个record中就包含了布局前后view的状态。

最后，mViewInfoStore调用了process方法，这个方法就是根据mViewInfoStore中的View信息，来执行动画逻辑，这又是一个可以展看很多的点，这里不做探讨，感兴趣的可以深入的看一下，会对动画流程有更直观的体会。

接下来就是onDraw,RecyclerView的draw过程可以分为２部分来看：RecyclerView负责绘制所有decoration；ItemView的绘制由ViewGroup处理，这里的绘制是android常规绘制逻辑，本文就不再阐述了。下面来看看RecyclerView的draw()和onDraw()方法：
~~~ Java
@Override
public void draw(Canvas c) {
    super.draw(c);
    final int count = mItemDecorations.size();
    for (int i = 0; i < count; i++) {
        mItemDecorations.get(i).onDrawOver(c, this, mState);
    }
    ...
}
@Override
public void onDraw(Canvas c) {
    super.onDraw(c);
    final int count = mItemDecorations.size();
    for (int i = 0; i < count; i++) {
        mItemDecorations.get(i).onDraw(c, this, mState);
    }
}
~~~
好了,测量,布局,绘制都大体讲了一下,回到我们开头,setLayoutManager已经完成。接下来是setAdapter(适配器模式),我们一起结合动画的实现(观察者模式)来解读,先看一下adapter类

#### setAdapter(适配器模式)

~~~ Java
public static abstract class Adapter {
    private final AdapterDataObservable mObservable = new AdapterDataObservable();
    public void registerAdapterDataObserver(AdapterDataObserver observer) {
        mObservable.registerObserver(observer);
    }
    public void unregisterAdapterDataObserver(AdapterDataObserver observer) {
        mObservable.unregisterObserver(observer);
    }
    public final void notifyItemInserted(int position) {
        mObservable.notifyItemRangeInserted(position, 1);
    }
}
~~~
RecyclerView的Adapter，这个控件需要的是View(dst),而我们有的一般是datas(src),所以适配器Adapter就是完成了数据源datas 转化成 ItemView的工作。带入src->Adapter->dst中，即datas->Adapter->View. 

通过public abstract void onBindViewHolder(VH holder, int position);将datas绑定到view然后返回ViewHolder我们可以看到Adapter中包含一个AdapterDataObservable的对象mObservable，这个是一个可观察者，在可观察者中可以注册一系列的观察者AdapterDataObserver。在我们调用的notify函数的时候，就是可观察者发出通知，这时已经注册的观察者都可以收到这个通知，然后依次进行处理。哈哈,是不是我们前面的Subject…那么我们看一下注册观察者的地方。

注册观察者的地方就是在RecyclerView的这个函数中。这个是setAdapter方法最终调用的地方。它主要做了：

如果之前存在Adapter，先移除原来的，注销观察者，和从RecyclerView Detached。
然后根据参数，决定是否清除原来的ViewHolder
然后重置AdapterHelper，并更新Adapter，注册观察者。
~~~ Java
public void setAdapter(Adapter adapter) {
    // bail out if layout is frozen
    setLayoutFrozen(false);
    setAdapterInternal(adapter, false, true);
    requestLayout();
}
~~~
看一看setAdapterInternal
~~~ Java
private void setAdapterInternal(Adapter adapter, boolean compatibleWithPrevious,
        boolean removeAndRecycleViews) {
    if (mAdapter != null) {
        mAdapter.unregisterAdapterDataObserver(mObserver);
        mAdapter.onDetachedFromRecyclerView(this);
    }
    if (!compatibleWithPrevious || removeAndRecycleViews) {
        // end all running animations
        if (mItemAnimator != null) {
            mItemAnimator.endAnimations();
        }
        // Since animations are ended, mLayout.children should be equal to
        // recyclerView.children. This may not be true if item animator's end does not work as
        // expected. (e.g. not release children instantly). It is safer to use mLayout's child
        // count.
        if (mLayout != null) {
            mLayout.removeAndRecycleAllViews(mRecycler);
            mLayout.removeAndRecycleScrapInt(mRecycler);
        }
        // we should clear it here before adapters are swapped to ensure correct callbacks.
        mRecycler.clear();
    }
    mAdapterHelper.reset();
    final Adapter oldAdapter = mAdapter;
    mAdapter = adapter;
    if (adapter != null) {
        adapter.registerAdapterDataObserver(mObserver);
        adapter.onAttachedToRecyclerView(this);
    }
    if (mLayout != null) {
        mLayout.onAdapterChanged(oldAdapter, mAdapter);
    }
    mRecycler.onAdapterChanged(oldAdapter, mAdapter, compatibleWithPrevious);
    mState.mStructureChanged = true;
    markKnownViewsInvalid();
}
~~~
从这里我们可以看出，mObserver这个成员变量就是注册的观察者，那么我们去看看这个成员变量的内容。

该成员变量是一个RecyclerViewDataObserver的实例，那么RecyclerViewDataObserver实现了AdapterDataObserver中的方法。其中onItemRangeInserted(int positionStart, int itemCount)就是观察者接受到有数据插入通知的方法。那么我们来分析这个方法。看注释。
~~~ Java
private final RecyclerViewDataObserver mObserver = new RecyclerViewDataObserver();
private class RecyclerViewDataObserver extends AdapterDataObserver {
    ...
    mPostUpdatesOnAnimation = version >= 16;
    @Override
    public void onItemRangeInserted(int positionStart, int itemCount) {
        // 1) 断言不在布局或者滚动过程中，其实就是如果在布局或者滚动过程中，则不会执
        // 行下面的内容
        assertNotInLayoutOrScroll(null);
        // 2) 这里小型，不要小看if括号中的内容，这是关键。我们去看看这个方法的实现。
        // 见下面注释 3)，在 3) 返回true之后执行triggerUpdateProcessor方法，
        // triggerUpdateProcessor方法分析请看注释 4)。
        if (mAdapterHelper.onItemRangeInserted(positionStart, itemCount)) {
            triggerUpdateProcessor();
        }
    }
}
~~~
AdapterHelper中onItemRangeInserted函数即相关内容，请看注释 `3)`。
~~~ Java
class AdapterHelper implements OpReorderer.Callback {
    // 一个待处理更新操作的列表，该列表中存放所有等待处理的操作信息。
    final ArrayList mPendingUpdates = new ArrayList();
    // 3) 该方法将插入操作的信息存储到一个UpdateOp中，并添加到待处理更新操作列表中，
    // 如果操作列表中的值是1，就返回真表示需要处理操作，等于1的判断避免重复触发处理操作。
    // obtainUpdateOp内部是通过池来得到一个UpdateOp对象。那么下面回去看我们注释 4)。
    boolean onItemRangeInserted(int positionStart, int itemCount) {
        if (itemCount < 1) {
            return false;
        }
        mPendingUpdates.add(obtainUpdateOp(UpdateOp.ADD, positionStart, itemCount, null));
        mExistingUpdateTypes |= UpdateOp.ADD;
        return mPendingUpdates.size() == 1;
    }
}
// 4) 触发更新处理操作，分为两种情况，在 版本大于16 且 已经Attach 并且 设置了大小固定 的情况下，
// 进行mUpdateChildViewsRunnable中的操作。否则请求布局。
void triggerUpdateProcessor() {
    if (mPostUpdatesOnAnimation && mHasFixedSize && mIsAttached) {
        ViewCompat.postOnAnimation(RecyclerView.this, mUpdateChildViewsRunnable);
    } else {
        mAdapterUpdateDuringMeasure = true;
        requestLayout();
    }
}
// 5) 其中核心代码是consumePendingUpdateOperations()那么继续往下看。
private final Runnable mUpdateChildViewsRunnable = new Runnable() {
    public void run() {
        ...
        consumePendingUpdateOperations();
    }
};
private void consumePendingUpdateOperations() {
    ...
    if (mAdapterHelper.hasAnyUpdateTypes(UpdateOp.UPDATE) && !mAdapterHelper
            .hasAnyUpdateTypes(UpdateOp.ADD | UpdateOp.REMOVE | UpdateOp.MOVE)) {
        // 6) 如果只有更新类型的操作(这里指内容的更新，不影响View位置的改变)的情况下，
        // 先进行预处理，然后在没有View更新的情况下消耗延迟的更新操作，否则调用
        // dispatchLayout方法对RecyclerView中的View重新布局。那么接下来分析
        // preProcess()方法。
        mAdapterHelper.preProcess();
        if (!mLayoutRequestEaten) {
            if (hasUpdatedView()) {
                dispatchLayout();
            } else {
                mAdapterHelper.consumePostponedUpdates();
            }
        }
        resumeRequestLayout(true);
    } else if (mAdapterHelper.hasPendingUpdates()) {
        // 7) 在既有更新操作又有添加或者删除或者移动中任意一个的情况下，调用
        // dispatchLayout方法对RecyclerView中的View重新布局
        dispatchLayout();
    }
}
// 8) 预处理做了以下几件事情，<1> 先将待处理操作重排。<2> 应用所有操作 <3> 清空待处理操作列表，
// 以ADD为例分析流程。
void preProcess() {
    mOpReorderer.reorderOps(mPendingUpdates);
    final int count = mPendingUpdates.size();
    for (int i = 0; i < count; i++) {
        UpdateOp op = mPendingUpdates.get(i);
        switch (op.cmd) {
            case UpdateOp.ADD:
                applyAdd(op);
                break;
            case UpdateOp.REMOVE:
                applyRemove(op);
                break;
            case UpdateOp.UPDATE:
                applyUpdate(op);
                break;
            case UpdateOp.MOVE:
                applyMove(op);
                break;
        }
        if (mOnItemProcessedCallback != null) {
            mOnItemProcessedCallback.run();
        }
    }
    mPendingUpdates.clear();
}
// 9) 直接看postponeAndUpdateViewHolders
private void applyAdd(UpdateOp op) {
    postponeAndUpdateViewHolders(op);
}
// 10) 先将操作添加到推迟的操作列表中。然后将操作的内容交给回调处理。
private void postponeAndUpdateViewHolders(UpdateOp op) {
    mPostponedList.add(op);
    switch (op.cmd) {
        case UpdateOp.ADD:
            mCallback.offsetPositionsForAdd(op.positionStart, op.itemCount);
            break;
        case UpdateOp.MOVE:
            mCallback.offsetPositionsForMove(op.positionStart, op.itemCount);
            break;
        case UpdateOp.REMOVE:
            mCallback.offsetPositionsForRemovingLaidOutOrNewView(op.positionStart,
                    op.itemCount);
            break;
        case UpdateOp.UPDATE:
            mCallback.markViewHoldersUpdated(op.positionStart, op.itemCount, op.payload);
            break;
        default:
            throw new IllegalArgumentException("Unknown update op type for " + op);
    }
}
// 11) 直接看offsetPositionRecordsForInsert
@Override
public void offsetPositionsForAdd(int positionStart, int itemCount) {
    offsetPositionRecordsForInsert(positionStart, itemCount);
    mItemsAddedOrRemoved = true;
}
// 12) 该方法主要是便利所有的ViewHolder，然后把在插入位置之后的ViewHolder的位置
// 向后移动插入的个数，最后在对Recycler中缓存的ViewHolder做同样的操作，最后申请重新布局。
void offsetPositionRecordsForInsert(int positionStart, int itemCount) {
    final int childCount = mChildHelper.getUnfilteredChildCount();
    for (int i = 0; i < childCount; i++) {
        final ViewHolder holder = getChildViewHolderInt(mChildHelper.getUnfilteredChildAt(i));
        if (holder != null && !holder.shouldIgnore() && holder.mPosition >= positionStart) {
            holder.offsetPosition(itemCount, false);
            mState.mStructureChanged = true;
        }
    }
    mRecycler.offsetPositionRecordsForInsert(positionStart, itemCount);
    requestLayout();
}
~~~
前面讲过ViewInfoStore这个类是用来追踪View所要做的动画的。其中有一个内部类InfoRecord，该类用来存储ViewHolder前后的信息，以及ViewHolder状态的flag。其中还有一个非常重要的方法，process，该方法会处理所有的mLayoutHolderMap中的值，并根据其flag和前后的信息来判断ViewHolder的动作，并将这个动作反应给ProcessCallback。分别有4种行为：消失，出现，一直存在，为使用。然后交给外面去处理。

前面说过 dispatchLayoutStep3mViewInfoStore.process(mViewInfoProcessCallback);,之后我们看一下mViewInfoStore的ProcessCallback的实现mViewInfoProcessCallback，这里只拿processAppeared做分析：
~~~ Java
private final ViewInfoStore.ProcessCallback mViewInfoProcessCallback =
            new ViewInfoStore.ProcessCallback() {
    ...
    @Override
    public void processAppeared(ViewHolder viewHolder,
            ItemHolderInfo preInfo, ItemHolderInfo info) {
        animateAppearance(viewHolder, preInfo, info);
    }
    ...
};
~~~
然后看一下animateAppearance方法：
~~~ Java
private void animateAppearance(@NonNull ViewHolder itemHolder,
        @Nullable ItemHolderInfo preLayoutInfo, @NonNull ItemHolderInfo postLayoutInfo) {
    itemHolder.setIsRecyclable(false);
    if (mItemAnimator.animateAppearance(itemHolder, preLayoutInfo, postLayoutInfo)) {
        postAnimationRunner();
    }
}
~~~
该方法中不要忽略if中的内容：`mItemAnimator.animateAppearance(itemHolder, preLayoutInfo, postLayoutInfo)`那么进入该方法：看注释。
~~~ Java
@Override
public boolean animateAppearance(@NonNull ViewHolder viewHolder,
        @Nullable ItemHolderInfo preLayoutInfo, @NonNull ItemHolderInfo postLayoutInfo) {
    // 该方法通过前后的布局信息来判断是移动还是添加。下面我们以添加为例分析
    if (preLayoutInfo != null && (preLayoutInfo.left != postLayoutInfo.left
            || preLayoutInfo.top != postLayoutInfo.top)) {
        return animateMove(viewHolder, preLayoutInfo.left, preLayoutInfo.top,
                postLayoutInfo.left, postLayoutInfo.top);
    } else {
        return animateAdd(viewHolder);
    }
}
~~~
真正实现是在DefaultItenAnimator中：这里做了三件事情，重置holder的动画，设置显示属性，然后添加到mPendingAdditions中，mPendingAdditions是一个存储添加ViewHolder的List，表示待处理的添加动画的ViewHolder。同样在DefaultItenAnimator总也有，移动的，移除的列表。
~~~ Java
@Override
public boolean animateAdd(final ViewHolder holder) {
    resetAnimation(holder);
    ViewCompat.setAlpha(holder.itemView, 0);
    mPendingAdditions.add(holder);
    return true;
}
~~~
最后返回true，进入if，执行postAnimationRunner方法。
~~~ Java
private void postAnimationRunner() {
    if (!mPostedAnimatorRunner && mIsAttached) {
        ViewCompat.postOnAnimation(this, mItemAnimatorRunner);
        mPostedAnimatorRunner = true;
    }
}
~~~
去看mItemAnimatorRunner，其中调用的ItemAnimator的runPendingAnimations方法。
~~~ Java
private Runnable mItemAnimatorRunner = new Runnable() {
    @Override
    public void run() {
        if (mItemAnimator != null) {
            mItemAnimator.runPendingAnimations();
        }
        mPostedAnimatorRunner = false;
    }
};
~~~
然后分析runPendingAnimations方法：该方法并不难，按照移除，移动，改变，添加，依次处理之前的待处理列表中的内容。这里还是以添加的做为例子来分析，看注释。
~~~ Java
public void runPendingAnimations() {
    boolean removalsPending = !mPendingRemovals.isEmpty();
    boolean movesPending = !mPendingMoves.isEmpty();
    boolean changesPending = !mPendingChanges.isEmpty();
    boolean additionsPending = !mPendingAdditions.isEmpty();
    if (!removalsPending && !movesPending && !additionsPending && !changesPending) {
        return;
    }
    for (ViewHolder holder : mPendingRemovals) {
        ...
    }
    mPendingRemovals.clear();
    if (movesPending) {
        ...
    }
    if (changesPending) {
        ...
    }
    if (additionsPending) {
        final ArrayList additions = new ArrayList<>();
        additions.addAll(mPendingAdditions);
        mAdditionsList.add(additions);
        mPendingAdditions.clear();
        // 重要的是这个adder。其中重要的是 animateAddImpl(holder) 方法。那么来分析这个方法。
        Runnable adder = new Runnable() {
            public void run() {
                for (ViewHolder holder : additions) {
                    animateAddImpl(holder);
                }
                additions.clear();
                mAdditionsList.remove(additions);
            }
        };
        if (removalsPending || movesPending || changesPending) {
            long removeDuration = removalsPending ? getRemoveDuration() : 0;
            long moveDuration = movesPending ? getMoveDuration() : 0;
            long changeDuration = changesPending ? getChangeDuration() : 0;
            long totalDelay = removeDuration + Math.max(moveDuration, changeDuration);
            View view = additions.get(0).itemView;
            ViewCompat.postOnAnimationDelayed(view, adder, totalDelay);
        } else {
            adder.run();
        }
    }
}
~~~
这个方法其实就是通过属性动画对ViewHolder中的View做渐变动画。
~~~ Java
private void animateAddImpl(final ViewHolder holder) {
    final View view = holder.itemView;
    final ViewPropertyAnimatorCompat animation = ViewCompat.animate(view);
    mAddAnimations.add(holder);
    animation.alpha(1).setDuration(getAddDuration()).
            setListener(new VpaListenerAdapter() {
                @Override
                public void onAnimationStart(View view) {
                    dispatchAddStarting(holder);
                }
                @Override
                public void onAnimationCancel(View view) {
                    ViewCompat.setAlpha(view, 1);
                }
                @Override
                public void onAnimationEnd(View view) {
                    animation.setListener(null);
                    dispatchAddFinished(holder);
                    mAddAnimations.remove(holder);
                    dispatchFinishedWhenDone();
                }
            }).start();
}
~~~
到这里，终于是触发了我们的动画。其它的动作，流程类似，细节不同而已。那么通过流程我们可以深入理解以下2点：

如果我们的RecyclerView的高度和宽度不变，那么通过手动执行setHasFixedSize(true)，可以在一定程度上减少计算，提高性能。可以在 `4)` 步的时候绕过requestLayout，只走自身的布局流程。而requestLayout是申请父控件重新布局流程，两者的计算量是不一样的。
自定义ItemAnimator的时候，如果在animateAppearance，animateDisappearance……方法中直接运行了动画，就返回false，如果是暂存起来，就返回true，然后将真正执行动画的操作放在runPendingAnimations方法中。

#### 滑动
RecyclerView的滑动过程可以分为2个阶段：手指在屏幕上移动，使RecyclerView滑动的过程，可以称为scroll；手指离开屏幕，RecyclerView继续滑动一段距离的过程，可以称为fling。现在先看看RecyclerView的触屏事件处理onTouchEvent()方法：
~~~ Java
public boolean onTouchEvent(MotionEvent e) {
    ...
    if (mVelocityTracker == null) {
        mVelocityTracker = VelocityTracker.obtain();
    }
    ...
    switch (action) {
        ...
        case MotionEvent.ACTION_MOVE: {
            ...
            final int x = (int) (MotionEventCompat.getX(e, index) + 0.5f);
            final int y = (int) (MotionEventCompat.getY(e, index) + 0.5f);
            int dx = mLastTouchX - x;
            int dy = mLastTouchY - y;
            ...
            if (mScrollState != SCROLL_STATE_DRAGGING) {
                ...
                if (canScrollVertically && Math.abs(dy) > mTouchSlop) {
                    if (dy > 0) {
                        dy -= mTouchSlop;
                    } else {
                        dy += mTouchSlop;
                    }
                    startScroll = true;
                }
                if (startScroll) {
                    setScrollState(SCROLL_STATE_DRAGGING);
                }
            }
            if (mScrollState == SCROLL_STATE_DRAGGING) {
                mLastTouchX = x - mScrollOffset[0];
                mLastTouchY = y - mScrollOffset[1];
                if (scrollByInternal(
                        canScrollHorizontally ? dx : 0,
                        canScrollVertically ? dy : 0,
                        vtev)) {
                    getParent().requestDisallowInterceptTouchEvent(true);
                }
            }
        } break;
        ...
        case MotionEvent.ACTION_UP: {
            ...
            final float yvel = canScrollVertically ?
                    -VelocityTrackerCompat.getYVelocity(mVelocityTracker, mScrollPointerId) : 0;
            if (!((xvel != 0 || yvel != 0) && fling((int) xvel, (int) yvel))) {
                setScrollState(SCROLL_STATE_IDLE);
            }
            resetTouch();
        } break;
        ...
    }
    ...
}
~~~
这里我以垂直方向的滑动来说明。当RecyclerView接收到ACTION_MOVE事件后，会先计算出手指移动距离（dy），并与滑动阀值（mTouchSlop）比较，当大于此阀值时将滑动状态设置为SCROLL_STATE_DRAGGING，而后调用scrollByInternal()方法，使RecyclerView滑动，这样RecyclerView的滑动的第一阶段scroll就完成了；当接收到ACTION_UP事件时，会根据之前的滑动距离与时间计算出一个初速度yvel，这步计算是由VelocityTracker实现的，然后再以此初速度，调用方法fling()，完成RecyclerView滑动的第二阶段fling。显然滑动过程中关键的方法就2个：scrollByInternal()与fling()。接下来同样以垂直线性布局来说明。先来说明scrollByInternal()，跟踪进入后，会发现它最终会调用到LinearLayoutManager.scrollBy()方法，这个过程很简单，我就不列出源码了，但是分析到这里先暂停下，去看看fling()方法：
~~~ Java
public boolean fling(int velocityX, int velocityY) {
    ...
    mViewFlinger.fling(velocityX, velocityY);
    ...
}
~~~
有用的就这一行，其它乱七八糟的不看也罢。mViewFlinger是一个Runnable的实现ViewFlinger的对象，就是它来控件着ReyclerView的fling过程的算法的。下面来看下类ViewFlinger的一段代码：
~~~ Java
void postOnAnimation() {
    if (mEatRunOnAnimationRequest) {
        mReSchedulePostAnimationCallback = true;
    } else {
        removeCallbacks(this);
        ViewCompat.postOnAnimation(RecyclerView.this, this);
    }
}
public void fling(int velocityX, int velocityY) {
    setScrollState(SCROLL_STATE_SETTLING);
    mLastFlingX = mLastFlingY = 0;
    mScroller.fling(0, 0, velocityX, velocityY,
            Integer.MIN_VALUE, Integer.MAX_VALUE, Integer.MIN_VALUE, Integer.MAX_VALUE);
    postOnAnimation();
}
~~~
可以看到，其实RecyclerView的fling是借助Scroller实现的；然后postOnAnimation()方法的作用就是在将来的某个时刻会执行我们给定的一个Runnable对象，在这里就是这个mViewFlinger对象，这部分原理我就不再深入分析了，它已经不属于本文的范围了。并且，关于Scroller的作用及原理，本文也不会作过多解释。对于这两点各位可以自行查阅，有很多文章对于作过详细阐述的。接下来看看ViewFlinger.run()方法：
~~~ Java
public void run() {
    ...
    if (scroller.computeScrollOffset()) {
        final int x = scroller.getCurrX();
        final int y = scroller.getCurrY();
        final int dx = x - mLastFlingX;
        final int dy = y - mLastFlingY;
        ...
        if (mAdapter != null) {
            ...
            if (dy != 0) {
                vresult = mLayout.scrollVerticallyBy(dy, mRecycler, mState);
                overscrollY = dy - vresult;
            }
            ...
        }
        ...
        if (!awakenScrollBars()) {
            invalidate();//刷新界面
        }
        ...
        if (scroller.isFinished() || !fullyConsumedAny) {
            setScrollState(SCROLL_STATE_IDLE);
        } else {
            postOnAnimation();
        }
    }
    ...
}
~~~
本段代码中有个方法mLayout.scrollVerticallyBy()，跟踪进入你会发现它最终也会走到LinearLayoutManager.scrollBy()，这样虽说RecyclerView的滑动可以分为两阶段，但是它们的实现最终其实是一样的。这里我先解释下上段代码。第一，dy表示滑动偏移量，它是由Scroller根据时间偏移量（Scroller.fling()开始时间到当前时刻）计算出的，当然如果是RecyclerView的scroll阶段，这个偏移量也就是手指滑动距离。第二，上段代码会多次执行，至到Scroller判断滑动结束或已经滑动到边界。再多说一下，postOnAnimation()保证了RecyclerView的滑动是流畅，这里涉及到著名的“android 16ms”机制，简单来说理想状态下，上段代码会以16毫秒一次的速度执行，这样其实，Scroller每次计算的滑动偏移量是很小的一部分，而RecyclerView就会根据这个偏移量，确定是平移ItemView，还是除了平移还需要再创建新ItemView。

![RecyclerView滑动](/images/blogimages/2019/recyclerview-flip.png)

现在就来看看LinearLayoutManager.scrollBy()方法：
~~~ Java
int scrollBy(int dy, RecyclerView.Recycler recycler, RecyclerView.State state) {
    ...
    final int absDy = Math.abs(dy);
    updateLayoutState(layoutDirection, absDy, true, state);
    final int consumed = mLayoutState.mScrollingOffset
            + fill(recycler, mLayoutState, state, false);
    ...
    final int scrolled = absDy > consumed ? layoutDirection * consumed : dy;
    mOrientationHelper.offsetChildren(-scrolled);
    ...
}
~~~
如上文所讲到的fill()方法，作用就是向可绘制区间填充ItemView，那么在这里，可绘制区间就是滑动偏移量！再看方法mOrientationHelper.offsetChildren()作用就是平移ItemView。好了整个滑动过程就分析完成了，当然RecyclerView的滑动还有个特性叫平滑滑动（smooth scroll），其实它的实现就是一个fling滑动，所以就不再赘述了。

#### 缓存逻辑
前面的章节对于Recycler这个类相关的操作我们都直接进行了忽略，这里我们好好的来看下RecylerView是如何工作的。

与ListView不同，RecyclerView的缓存是分为多级的，但其实整个的缓存逻辑还是很容易理解的，Recycler的作用就是重用ItemView。在填充ItemView的时候，ItemView是从它获取的；滑出屏幕的ItemView是由它回收的。对于不同状态的ItemView存储在了不同的集合中，比如有scrapped、cached、exCached、recycled，当然这些集合并不是都定义在同一个类里。

~~~ Java
public final class Recycler {
        // 一级缓存
        final ArrayList<ViewHolder> mAttachedScrap = new ArrayList<>();
        ArrayList<ViewHolder> mChangedScrap = null;
        // 二级缓存
        final ArrayList<ViewHolder> mCachedViews = new ArrayList<ViewHolder>();
        // 三级缓存
        private ViewCacheExtension mViewCacheExtension;
        // 四级缓存
        RecycledViewPool mRecyclerPool;
}
~~~

回到之前的layoutChunk方法中，有行代码layoutState.next(recycler)，它的作用自然就是获取ItemView，我们进入这个方法查看，最终它会调用到RecyclerView.Recycler.getViewForPosition()方法：

~~~ Java
View getViewForPosition(int position, boolean dryRun) {
    return tryGetViewHolderForPositionByDeadline(position, dryRun, FOREVER_NS).itemView;
}
~~~
getViewForPosition 可以看到其实每次 rv 取出要显示的一个item本质上就是取出一个viewholder，根据viewholder上关联的**itemview**来展示这个item。而取出viewholder最核心的方法就是 **tryGetViewHolderForPositionByDeadline**
~~~ Java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,
        boolean dryRun, long deadlineNs) {
    // ...
    boolean fromScrapOrHiddenOrCache = false;
    ViewHolder holder = null;
    // 0) 先从 mChangedScrap 取viewholder（一级缓存）
    if (mState.isPreLayout()) {
        holder = getChangedScrapViewForPosition(position);
        fromScrapOrHiddenOrCache = holder != null;
    }
    // 1) 从mAttachedScrap(一级缓存) 或 mCachedViews（二级缓存）中取viewholder
    if (holder == null) {
        holder = getScrapOrHiddenOrCachedHolderForPosition(position, dryRun);
        if (holder != null) {
            if (!validateViewHolderForOffsetPosition(holder)) {
                // recycle holder (and unscrap if relevant) since it can't be used
                if (!dryRun) {
                    // we would like to recycle this but need to make sure it is not used by
                    // animation logic etc.
                    holder.addFlags(ViewHolder.FLAG_INVALID);
                    if (holder.isScrap()) {
                        removeDetachedView(holder.itemView, false);
                        holder.unScrap();
                    } else if (holder.wasReturnedFromScrap()) {
                        holder.clearReturnedFromScrapFlag();
                    }
                    recycleViewHolderInternal(holder);
                }
                holder = null;
            } else {
                fromScrapOrHiddenOrCache = true;
            }
        }
    }
    if (holder == null) {
        // ... stable ids的逻辑
        // 3) 三级缓存
        if (holder == null && mViewCacheExtension != null) {
            // We are NOT sending the offsetPosition because LayoutManager does not
            // know it.
            final View view = mViewCacheExtension
                    .getViewForPositionAndType(this, position, type);
            if (view != null) {
                holder = getChildViewHolder(view);
                if (holder == null) {
                    throw new IllegalArgumentException("getViewForPositionAndType returned"
                            + " a view which does not have a ViewHolder"
                            + exceptionLabel());
                } else if (holder.shouldIgnore()) {
                    throw new IllegalArgumentException("getViewForPositionAndType returned"
                            + " a view that is ignored. You must call stopIgnoring before"
                            + " returning this view." + exceptionLabel());
                }
            }
        }
        if (holder == null) { // fallback to pool
            // 4) 四级缓存
            holder = getRecycledViewPool().getRecycledView(type);
            if (holder != null) {
                holder.resetInternal();
                if (FORCE_INVALIDATE_DISPLAY_LIST) {
                    invalidateDisplayListInt(holder);
                }
            }
        }
        if (holder == null) {
            // ...
            holder = mAdapter.createViewHolder(RecyclerView.this, type);
            // ...
        }
    }

    //生成LayoutParams的代码 ...
    return holder.itemView;
}
~~~

获取View的逻辑可以整理成如下：
根据列表位置获取ItemView，先后从scrapped、cached、exCached、recycled集合中查找相应的ItemView，如果没有找到，就创建（ **Adapter.createViewHolder()** ），最后与数据集绑定。

其中scrapped、cached和exCached集合定义在RecyclerView.Recycler中，分别表示将要在RecyclerView中删除的ItemView、一级缓存ItemView和二级缓存ItemView，cached集合的大小默认为２，exCached是需要我们通过RecyclerView.ViewCacheExtension自己实现的，默认没有；

recycled集合其实是一个Map,private SparseArray<ArrayList<ViewHolder>> mScrap = new SparseArray<ArrayList<ViewHolder>>();，定义在RecyclerView.RecycledViewPool中，将ItemView以ItemType分类保存了下来，这里算是RecyclerView设计上的亮点，通过RecyclerView.RecycledViewPool可以实现在**不同的RecyclerView之间共享ItemView**，只要为这些不同RecyclerView设置同一个RecyclerView.RecycledViewPool就可以了。

上面解释了ItemView从不同集合中获取的方式，那么RecyclerView又是在什么时候向这些集合中添加ItemView的呢？下面我逐个介绍下。scrapped集合中存储的其实是正在执行REMOVE操作的ItemView，这部分会在后文进一步描述。在fill()方法的循环体中有行代码recycleByLayoutState(recycler, layoutState);，最终这个方法会执行到RecyclerView.Recycler.recycleViewHolderInternal()方法：
~~~ Java
void recycleViewHolderInternal(ViewHolder holder) {
   ...
   if (holder.isRecyclable()) {
       if (!holder.hasAnyOfTheFlags(ViewHolder.FLAG_INVALID | ViewHolder.FLAG_REMOVED
               | ViewHolder.FLAG_UPDATE)) {
           int cachedViewSize = mCachedViews.size();
           if (cachedViewSize >= mViewCacheMax && cachedViewSize > 0) {
               recycleCachedViewAt(0);
               cachedViewSize --;
           }
           if (cachedViewSize < mViewCacheMax) {
               mCachedViews.add(holder);
               cached = true;
           }
       }
       if (!cached) {
           addViewHolderToRecycledViewPool(holder);
           recycled = true;
       }
   }
   ...
}
~~~
View的回收并不像View的创建那么复杂，这里只涉及了两层缓存mCachedViews与mRecyclerPool，mCachedViews相当于一个先进先出的数据结构，每当有新的View需要缓存时都会将新的View存入mCachedViews，而mCachedViews则会移除头元素，并将头元素放入mRecyclerPool，所以mCachedViews相当于一级缓存，mRecyclerPool则相当于二级缓存，并且mRecyclerPool是可以多个RecyclerView共享的，这在类似于多Tab的新闻类应用会有很大的用处，因为多个Tab下的多个RecyclerView可以共用一个二级缓存。减少内存开销。

RecyclerView定义了4种针对数据集的操作，分别是ADD、REMOVE、UPDATE、MOVE，封装在了AdapterHelper.UpdateOp类中，并且所有操作由一个大小为30的对象池管理着。当我们要对数据集作任何操作时，都会从这个对象池中取出一个UpdateOp对象，放入一个等待队列中，最后调用RecyclerView.RecyclerViewDataObserver.triggerUpdateProcessor()方法，根据这个等待队列中的信息，对所有子控件重新测量、布局并绘制且执行动画。以上就是我们调用Adapter.notifyItemXXX()系列方法后发生的事。显然当我们对某个ItemView做操作时，它很有可以会影响到其它ItemView。下面我以REMOVE为例来梳理下这个流程。

![](/images/blogimages/2019/design_pattern_bridge_uml.png)

首先调用Adapter.notifyItemRemove()，追溯到方法RecyclerView.RecyclerViewDataObserver.onItemRangeRemoved()：
~~~ Java
public void onItemRangeRemoved(int positionStart, int itemCount) {
    assertNotInLayoutOrScroll(null);
    if (mAdapterHelper.onItemRangeRemoved(positionStart, itemCount)) {
        triggerUpdateProcessor();
    }
}
~~~
这里的mAdapterHelper.onItemRangeRemoved()就是向之前提及的等待队列添加一个类型为REMOVE的UpdateOp对象， triggerUpdateProcessor()方法就是调用View.requestLayout()方法，这会导致界面重新布局，也就是说方法RecyclerView.onLayout()会随后调用，这之后的流程就和在绘制流程一节中所描述的一致了。

### 与AdapterView比较

谈到RecyclerView，总避免不了与它的前辈AdapterView家族进行一撕，这里我整理了一下RecylerView与AdapterView的各自特点：

![](/images/blogimages/2019/design_pattern_bridge_uml.png)

前面四点两位都提供了各自的实现，但也各有各自的特点：

#### 点击事件
ListView原生提供Item单击、长按的事件, 而RecyclerView则需要使用onTouchListener，相对自己实现会比较复杂。

#### 分割线
ListView可以很轻松的设置divider属性来显示Item之间的分割线，RecyclerView需要我们自己实现ItemDecoration，前者使用简单，后者可定制性强。

#### 布局类型
AdapterView提供了ListView与GridView两种类型，分别对应流式布局与网格式布局。RecyclerView提供了LinearLayoutManager、GridLayoutManager与之抗衡，相对而言，使用RecyclerView来进行更换布局方式更为轻松。只需要更换一个变量即可，而对于AdapterView而言则是需要更换一个View了。

#### 缓存方式
ListView使用了一个名为RecyclerBin的类负责试图的缓存，而Recycler则使用Recycler来进行缓存，原理上两者基本一致。RecyclerView：里面存储的不是View，而是ViewHolder

#### 局部刷新
这是一个很有用的功能，在ListView中我们想局部刷新某个Item需要自己来编写刷新逻辑，而在RecyclerView中我们可以通过 notifyItemChanged(position) 来刷新单个Item，甚至可以通过 notifyItemChanged(position, payload) 来传入一个payload信息来刷新单个Item中的特定内容。

#### 动画
作为视觉动物，我相信很多人喜欢RecylerView都和它简单的动画API有关，因为之前对ListView做动画比较困难，并且不舒服。

#### 嵌套布局
嵌套布局也是最近比较火的一个概念，RecyclerView实现了 NestedScrollingChild 接口，使得它可以和一些嵌套组件很好的工作。我们再来看ListView原生独有的几个特点：

#### 头部与尾部的支持
ListView原生支持添加头部与尾部，虽然RecyclerView可以通过定义不同的Type来做支持，但实际应用中，如果封装的不好，是很容易出问题的，因为Adapter中的数据位置与物理数据位置发生了偏移。
#### 多选
支持多选、单选也是ListView的一大长处，其实如果要我们自己在RecyclerView中去做支持还是需要不少代码量的。多数据源的支持ListView提供了CursorAdapter、ArrayAdapter，可以让我们很方便的从数据库或者数组中获取数据，这在测试的时候很有用。

#### 总结
综上，我们会发现RecycerView的最大特点就是灵活，正因为这种灵活，因此会牺牲了某些便利性。而AdapterView相对来讲就比较刻板，但它原生为我们提供了很多有用的方法来便于我们快速开发。ListView并不像当年的ActivityGroup，在Fragment出来后就被标记为Deprecated。两者目前还是一种互补的关系，起码在短时间内RecyclerView还并不能完全替代AdapterView，个人感觉原因有两个，一是目前太多的应用使用了ListView，并且ListView向RecyclerView转变也没有无损的方法。第二点，比如我就是想添加个头部，每个item带个点击事件这类简单的需求，ListView完全可以很轻松的胜任，没必要舍近求远来使用RecyclerView。因此，在实际应用中选择更适合自己的就好。

当然，从Google最近几次的更新来看，RecyclerView的进化还是很迅速的，而ListView则几乎没什么变动，所以RecyclerView绝对是大大的潜力股呀。

### 设计精巧的类

在翻看RecycleView源码的过程中也遇见了许多之前没有注意过的类，这些类都可以复用在我们的日常工作当中。这里列举出其中具有代表性的几位。

#### Bucket
如果一个对象有大量的是与非的状态需要表示，通常我们会使用BitMask 技术来节省内存，在 Java 中，一个 byte 类型，有 8 位（bit），可以表达 8 个不同的状态，而 int 类型，则有 32 位，可以表达 32 种状态。再比如Long类型，有64位，则可以表达64中状态。一般情况下使用一个Long已经足够我们使用了。但如果有不设上限的状态需要我们表示呢？在ChildHelper里有一个静态内部类Bucket，基本源码如下：
~~~ Java
static class Bucket {
    final static int BITS_PER_WORD = Long.SIZE;
    final static long LAST_BIT = 1L << (Long.SIZE - 1);
    long mData = 0;
    Bucket next;
    void set(int index) {
        if (index >= BITS_PER_WORD) {
            ensureNext();
            next.set(index - BITS_PER_WORD);
        } else {
            mData |= 1L << index;
        }
    }
  ...
}
~~~
可以看到，Bucket是一个链表结构，当index大于64的时候，它便会去下一个Bucket去寻找，所以，Bucket可以不设上限的表示状态。

#### Pools
熟悉Message回收机制的朋友可能了解，在使用Message对象时最好通过 Message.obtain() 方法来获取，这样可以在很多情况下避免创建新对象。在使用完之后调用 message.recycle() 来回收消息。谷歌为这种机制也提供了抽象的实现,就是位于v4包下Pools类, 内部接口Pool提供了 acquire 与 release 两个方法,不过需要注意的是这个 acquire 方法可能返回空,毕竟Pools不是业务类,它不应该清楚对象的具体创建逻辑.还有一点是Pools与Message类的实现机制不同,每个Message对象内部都持有一个引用下一个message的指针,相当于一个链表结构,而Pool的实现类 SimplePool 中使用的是数组.Pool机制在 RecycleView 中有如下几处应用：

RecycleView将item的增删改封装为 UpdateOp 类。
ViewInfoStore 类中静态内部类 InfoRecord 。

### 总结

RecyclerView也不是万能的，它的灵活性也是有一定限制的，比如我就遇到了一不是很好解决的问题：Recyler的缓存级别是一个Item的整个View，而我们没办法自定义缓存级别，这样说比较抽象，举个例子，我的某些Item的某个子View加载很耗时，所以我希望我在上下滑动的时候，Item的其它View是可以被回收利用的，但这个加载很耗时的View是不要重复使用的。即我希望用空间换取时间来获取滑动的流畅性。当然，这样的需求不常见，RecyclerView也不能很好的满足这一点。


RecyclerView也应该算作一个明星控件了，自从其诞生开始就备受欢迎，仔细的学习也能让我们在工作中更容易的、更恰当的使用。本文也只是分析了RecyclerView的一部分，关于动画、滑动、嵌套滑动等等还需要大家自行去研究。有兴趣的可以看看[RecyclerView 和 ListView 使用对比分析](https://www.jianshu.com/p/f592f3715ae2?utm_campaign=haruki&utm_content=note&utm_medium=reader_share&utm_source=weixin&from=singlemessage&isappinstalled=1)

