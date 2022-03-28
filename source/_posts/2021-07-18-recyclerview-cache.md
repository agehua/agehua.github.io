---
layout: post
title: RecyclerView 缓存机制
category: accumulation
tags:
    - Android Source Code
    - RecyclerView
keywords: RecyclerView, RecyclerView cache
banner: https://cdn.conorlee.top/Garden%20Behind%20a%20House.jpg
thumbnail: https://cdn.conorlee.top/Garden%20Behind%20a%20House.jpg
toc: true
---

RecyclerView 内存性能优越，这得益于它独特的缓存机制

### 由问题引入
- 回收的是什么？复用什么？
- 回收到哪里去？从哪里获得复用？
- 什么时候回收？什么时候复用？
<!--more-->
### 从addView()开始
RecyclerView继承自 ViewGroup，遂以RecyclerView.addView()为切入点向上搜寻复用的代码。
在RecyclerView.java中全局搜索“addView”，发现RecyclerView()并没有对addView()函数重载，但找到一处addView()的调用：

~~~ java
//RecyclerView是ViewGroup的子类
public class RecyclerView extends ViewGroup implements ScrollingView, NestedScrollingChild2 {
    ...
    private void initChildrenHelper() {
        mChildHelper = new ChildHelper(new ChildHelper.Callback() {
            ...
            @Override
            public void addView(View child, int index) {
                if (VERBOSE_TRACING) {
                    TraceCompat.beginSection("RV addView");
                }
                //直接调用ViewGroup.addView()
                RecyclerView.this.addView(child, index);
                if (VERBOSE_TRACING) {
                    TraceCompat.endSection();
                }
                dispatchChildAttached(child);
            }
        }
    }
    ...
}
~~~

以ChildHelper.Callback.addView()为起点沿着调用链继续向上搜寻，经历了如下方法调用：

- ChildHelper.addView()
- LayoutManager.addViewInt()
- LayoutManager.addView()
- LinearLayoutManager.layoutChunk()：

~~~ java
void layoutChunk(RecyclerView.Recycler recycler, RecyclerView.State state,LayoutState layoutState, LayoutChunkResult result) {
        //获得下一个表项
        View view = layoutState.next(recycler);
        ...
        LayoutParams params = (LayoutParams) view.getLayoutParams();
        if (layoutState.mScrapList == null) {
            //将表项插入到列表中
            if (mShouldReverseLayout == (layoutState.mLayoutDirection == LayoutState.LAYOUT_START)) {
                addView(view);
            } else {
                addView(view, 0);
            }
        }
        ...
}
~~~

addView(view)中传入的view是函数layoutState.next()的返回值。猜测该函数是用来获得下一个表项的。表项不止一个，应该有一个循环不断的获得下一个表项才对。果然layoutChunk在 fill() 方法的 while 循环中被调用

~~~ java
public class LinearLayoutManager extends RecyclerView.LayoutManager implements ItemTouchHelper.ViewDropHandler, RecyclerView.SmoothScroller.ScrollVectorProvider {
    ...
    int fill(RecyclerView.Recycler recycler, LayoutState layoutState,
            RecyclerView.State state, boolean stopOnFocusable) {
        ...
        //recyclerview 剩余空间
        int remainingSpace = layoutState.mAvailable + layoutState.mExtra;
        LayoutChunkResult layoutChunkResult = mLayoutChunkResult;
        //不断填充，直到空间消耗完毕
        while ((layoutState.mInfinite || remainingSpace > 0) && layoutState.hasMore(state)) {
            layoutChunkResult.resetInternal();
            if (VERBOSE_TRACING) {
                TraceCompat.beginSection("LLM LayoutChunk");
            }
            //填充一个表项
            layoutChunk(recycler, state, layoutState, layoutChunkResult);
            ...
        }
        ...
    }
}
~~~

而fill()是在onLayoutChildren()中被调用：
~~~ java
/**
 * Lay out all relevant child views from the given adapter.
 * 布局所有给定adapter中相关孩子视图
 */
public void onLayoutChildren(Recycler recycler, State state) {
 	Log.e(TAG, "You must override onLayoutChildren(Recycler recycler, State state) ");
}
~~~

看完注释，感觉前面猜测应该是正确的。onLayoutChildren()是用来布局RecyclerView中所有的表项的。

回头去看一下layoutState.next()，表项复用逻辑应该就在其中.
~~~ java
public class LinearLayoutManager {
    static class LayoutState {       
       /**
         * Gets the view for the next element that we should layout.
         * 获得下一个元素的视图用于布局
         */
        View next(RecyclerView.Recycler recycler) {
            if (mScrapList != null) {
                return nextViewFromScrapList();
            }
            //调用了Recycler.getViewForPosition()
            final View view = recycler.getViewForPosition(mCurrentPosition);
            mCurrentPosition += mItemDirection;
            return view;
        }
    }
}
~~~

最终调用了Recycler.getViewForPosition()，Recycler是回收器的意思，感觉离想要找的“复用”逻辑越来越近了。 Recycler到底是做什么用的？ 
~~~ java
public class RecyclerView {
    /**
     * A Recycler is responsible for managing scrapped or detached item views for reuse.
     *  Recycler负责管理scrapped和detached表项的复用
     */
    public final class Recycler {
      ...
    }
}
~~~

终于找到你~~ ，Recycler用于表项的复用！沿着Recycler.getViewForPosition()的调用链继续向下搜寻，找到了一个关键函数（函数太长了，为了防止头晕，只列出了关键节点）
~~~ java
public class RecyclerView {
    public final class Recycler {
       /**
         * Attempts to get the ViewHolder for the given position, either from the Recycler scrap,
         * cache, the RecycledViewPool, or creating it directly.
         * 尝试获得指定位置的ViewHolder，要么从scrap，cache，RecycledViewPool中获取，要么直接重新创建
         */
        @Nullable
        ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
            ...
            boolean fromScrapOrHiddenOrCache = false;
            ViewHolder holder = null;
            //0 从changed scrap集合中获取ViewHolder
            if (mState.isPreLayout()) {
                holder = getChangedScrapViewForPosition(position);
                fromScrapOrHiddenOrCache = holder != null;
            }
            //1. 通过position从attach scrap或一级回收缓存中获取ViewHolder
            if (holder == null) {
                holder = getScrapOrHiddenOrCachedHolderForPosition(position, dryRun);
                ...
            }
            
            if (holder == null) {
                ...
                final int type = mAdapter.getItemViewType(offsetPosition);
                //2. 通过id在attach scrap集合和一级回收缓存中查找viewHolder
                if (mAdapter.hasStableIds()) {
                    holder = getScrapOrCachedViewForId(mAdapter.getItemId(offsetPosition),
                            type, dryRun);
                    ...
                }
                //3. 从自定义缓存中获取ViewHolder
                if (holder == null && mViewCacheExtension != null) {
                    // We are NOT sending the offsetPosition because LayoutManager does not
                    // know it.
                    final View view = mViewCacheExtension
                            .getViewForPositionAndType(this, position, type);
                    ...
                }
                //4.从缓存池中拿ViewHolder
                if (holder == null) { // fallback to pool
                    ...
                    holder = getRecycledViewPool().getRecycledView(type);
                    ...
                }
                //所有缓存都没有命中，只能创建ViewHolder
                if (holder == null) {
                    ...
                    holder = mAdapter.createViewHolder(RecyclerView.this, type);
                    ...
                }
            }

            boolean bound = false;
            if (mState.isPreLayout() && holder.isBound()) {
                // do not update unless we absolutely have to.
                holder.mPreLayoutPosition = position;
            }
            //只有invalid的viewHolder才能绑定视图数据
            else if (!holder.isBound() || holder.needsUpdate() || holder.isInvalid()) {
                final int offsetPosition = mAdapterHelper.findPositionOffset(position);
                //获得ViewHolder后，绑定视图数据
                bound = tryBindViewHolderByDeadline(holder, offsetPosition, position, deadlineNs);
            }
            ...
            return holder;
        }
    }
}
~~~

函数的名字以“tryGet”开头，“尝试获得”表示可能获得失败，再结合注释中说的：“尝试获得指定位置的ViewHolder，要么从scrap，cache，RecycledViewPool中，要么直接重新创建。”猜测scrap，cache，RecycledViewPool是回收表项的容器，相当于表项缓存，如果缓存未命中则只能重新创建。

函数的返回值是ViewHolder，难道回收和复用的是ViewHolder? 函数开头声明了局部变量ViewHolder holder = null;最终返回的也是这个局部变量，并且有4处holder == null的判断，这样的代码结构是不是有点像缓存？每次判空意味着上一级缓存未命中并继续尝试新的获取方法？缓存是不是有不止一种存储形式？ 让我们一次一次地看：

#### 第一次尝试
~~~ java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
      ...
      if (mState.isPreLayout()) {
          holder = getChangedScrapViewForPosition(position);
          fromScrapOrHiddenOrCache = holder != null;
      }
      ...
}
~~~
只有在mState.isPreLayout()为true时才会做这次尝试，这应该是一种特殊情况，先忽略。

#### 第二次尝试
~~~ java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
      ...
      if (holder == null) {
          holder = getScrapOrHiddenOrCachedHolderForPosition(position, dryRun);
          //下面一段代码蕴含着一个线索，买个伏笔，先把他略去
          ...
      }
      ...
}
~~~

当第一次尝试失败后，尝试通过getScrapOrHiddenOrCachedHolderForPosition()获得ViewHolder。
这里故意省略了一段代码，先埋个伏笔，待会分析。先沿着获取ViewHolder的调用链继续往下：
~~~ java
//省略非关键代码
        /**
         * Returns a view for the position either from attach scrap, hidden children, or cache.
         * 从attach scrap，hidden children或者cache中获得指定位置上的一个ViewHolder
         */
        ViewHolder getScrapOrHiddenOrCachedHolderForPosition(int position, boolean dryRun) {
            final int scrapCount = mAttachedScrap.size();
            // Try first for an exact, non-invalid match from scrap.
            //1.在attached scrap中搜索ViewHolder
            for (int i = 0; i < scrapCount; i++) {
                final ViewHolder holder = mAttachedScrap.get(i);
                if (!holder.wasReturnedFromScrap() && holder.getLayoutPosition() == position
                        && !holder.isInvalid() && (mState.mInPreLayout || !holder.isRemoved())) {
                    holder.addFlags(ViewHolder.FLAG_RETURNED_FROM_SCRAP);
                    return holder;
                }
            }
            //2.从移除屏幕的视图中搜索ViewHolder，找到了之后将他存入scrap回收集合中
            if (!dryRun) {
                View view = mChildHelper.findHiddenNonRemovedView(position);
                if (view != null) {
                    final ViewHolder vh = getChildViewHolderInt(view);
                    mChildHelper.unhide(view);
                    int layoutIndex = mChildHelper.indexOfChild(view);
                    ...
                    mChildHelper.detachViewFromParent(layoutIndex);
                    scrapView(view);
                    vh.addFlags(ViewHolder.FLAG_RETURNED_FROM_SCRAP
                            | ViewHolder.FLAG_BOUNCED_FROM_HIDDEN_LIST);
                    return vh;
                }
            }
            // Search in our first-level recycled view cache.
            //3.在缓存中搜索ViewHolder
            final int cacheSize = mCachedViews.size();
            for (int i = 0; i < cacheSize; i++) {
                final ViewHolder holder = mCachedViews.get(i);
                //若找到ViewHolder，还需要对ViewHolder的索引进行匹配判断
                if (!holder.isInvalid() && holder.getLayoutPosition() == position) {
                    ...
                    return holder;
                }
            }
            return null;
        }  
~~~
依次从三个地方搜索ViewHolder：1. mAttachedScrap  2. 隐藏表项  3. mCachedViews，找到立即返回。
其中mAttachedScrap和mCachedViews作为Recycler的成员变量，用来存储一组ViewHolder：
~~~ java
    public final class Recycler {
        final ArrayList<ViewHolder> mAttachedScrap = new ArrayList<>();
        ...
        final ArrayList<ViewHolder> mCachedViews = new ArrayList<ViewHolder>();
        ...
        RecycledViewPool mRecyclerPool;
    }
~~~

看到这里应该可以初步得出结论：RecyclerView回收机制中，**回收复用的对象是ViewHolder**，且以ArrayList为结构存储在Recycler对象中。

RecycledViewPool mRecyclerPool; 看着也像是回收容器，那待会是不是也会到这里拿 ViewHolder?

值得注意的是，**当成功从mCachedViews中获取ViewHolder对象后，还需要对其索引进行判断，这就意味着 mCachedViews中缓存的ViewHolder只能复用于指定位置** ，打个比方：手指向上滑动，列表向下滚动，第2个表项移出屏幕，第4个表项移入屏幕，此时再滑回去，第2个表项再次出现，这个过程中第4个表项不能复用被回收的第2个表项的ViewHolder，因为他们的位置不同，而再次进入屏幕的第2个表项就可以成功复用。 待会可以对比一下其他复用是否也需要索引判断

回到刚才埋下的伏笔，把第二次尝试获取ViewHolder的代码补全：
~~~ java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
      ...
      if (holder == null) {
          holder = getScrapOrHiddenOrCachedHolderForPosition(position, dryRun);
          //下面一段代码蕴含这一个线索，买个伏笔，先把他略去
          if (holder != null) {
               //检验ViewHolder有效性
               if (!validateViewHolderForOffsetPosition(holder)) {
                    // recycle holder (and unscrap if relevant) since it can not be used
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
                          //若不满足有效性检验，则回收ViewHolder
                          recycleViewHolderInternal(holder);
                    }
                    holder = null;
               } else {
                    fromScrapOrHiddenOrCache = true;
               }
          }
      }
      ...
}
~~~
如果成功获得ViewHolder则检验其有效性，若检验失败则将其回收。好不容易获取了ViewHoler对象，一言不合就把他回收？难道对所有复用的 ViewHolder 都有这么严格的检验吗？ 暂时无法回答这些疑问，还是先把复用逻辑看完吧：

#### 第三次尝试
~~~ java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
      ...
      //只有当Adapter设置了id，才会进行这次查找
      if (mAdapter.hasStableIds()) {
           holder = getScrapOrCachedViewForId(mAdapter.getItemId(offsetPosition),type, dryRun);
           if (holder != null) {
                // update position
                holder.mPosition = offsetPosition;
                fromScrapOrHiddenOrCache = true;
           }
      }
      ...
}
~~~
这一次尝试调用的函数名（“byId”）和上一次（“byPosition”）只是后缀不一样。上一次是通过表项位置，这一次是通过表项id。内部实现也几乎一样，判断的依据从表项位置变成表项id。为表项设置id属于特殊情况，先忽略。

#### 第四次尝试
~~~ java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
      ...
      if (holder == null && mViewCacheExtension != null) {
           // We are NOT sending the offsetPosition because LayoutManager does not
           // know it.
          final View view = mViewCacheExtension.getViewForPositionAndType(this, position, type);
           if (view != null) {
                //获得view对应的ViewHolder
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
      ...
}
~~~
经过从mAttachedScrap和mCachedViews获取ViewHolder未果后，继续尝试通过ViewCacheExtension 获取：
~~~ java
    /**
     * ViewCacheExtension is a helper class to provide an additional layer of view caching that can
     * be controlled by the developer.
     * ViewCacheExtension提供了额外的表项缓存层，用户帮助开发者自己控制表项缓存
     * <p>
     * When {@link Recycler#getViewForPosition(int)} is called, Recycler checks attached scrap and
     * first level cache to find a matching View. If it cannot find a suitable View, Recycler will
     * call the {@link #getViewForPositionAndType(Recycler, int, int)} before checking
     * {@link RecycledViewPool}.
     * 当Recycler从attached scrap和first level cache中未能找到匹配的表项时，它会在去RecycledViewPool中查找之前，先尝试从自定义缓存中查找
     * <p>
     */
    public abstract static class ViewCacheExtension {

        /**
         * Returns a View that can be binded to the given Adapter position.
         * <p>
         * This method should <b>not</b> create a new View. Instead, it is expected to return
         * an already created View that can be re-used for the given type and position.
         * If the View is marked as ignored, it should first call
         * {@link LayoutManager#stopIgnoringView(View)} before returning the View.
         * <p>
         * RecyclerView will re-bind the returned View to the position if necessary.
         */
        public abstract View getViewForPositionAndType(Recycler recycler, int position, int type);
    }
~~~

注释揭露了很多信息：ViewCacheExtension用于开发者自定义表项缓存，且这层缓存的访问顺序位于mAttachedScrap和mCachedViews之后，RecycledViewPool 之前。这和`Recycler.tryGetViewHolderForPositionByDeadline()`中的代码逻辑一致，那接下来的第五次尝试，应该是从 RecycledViewPool 中获取 ViewHolder

#### 第五次尝试
~~~ java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
      ...
      if (holder == null) { 
          ...
          //从回收池中获取ViewHolder对象
          holder = getRecycledViewPool().getRecycledView(type);
          if (holder != null) {
               holder.resetInternal();
               if (FORCE_INVALIDATE_DISPLAY_LIST) {
                    invalidateDisplayListInt(holder);
               }
          }
      }
      ...
}
~~~
前四次尝试都未果，最后从RecycledViewPool 中获取ViewHolder。稍等片刻！相对于从mAttachedScrap  和 mCachedViews 中获取 ViewHolder，此处并没有严格的检验逻辑。为啥要区别对待不同的缓存？ 大大的问号悬在头顶，但现在暂时无法解答，还是接着看RecycledViewPool 的结构吧~
~~~ java
public final class Recycler {
    ...
    RecycledViewPool mRecyclerPool;
    //获得RecycledViewPool实例
    RecycledViewPool getRecycledViewPool() {
          if (mRecyclerPool == null) {
              mRecyclerPool = new RecycledViewPool();
          }
          return mRecyclerPool;
    }
    ...
}
public static class RecycledViewPool {
    ...
    //从回收池中获取ViewHolder对象
    public ViewHolder getRecycledView(int viewType) {
          final ScrapData scrapData = mScrap.get(viewType);
          if (scrapData != null && !scrapData.mScrapHeap.isEmpty()) {
              final ArrayList<ViewHolder> scrapHeap = scrapData.mScrapHeap;
              return scrapHeap.remove(scrapHeap.size() - 1);
          }
          return null;
    }
    ...
}
~~~
函数中只要访问了类成员变量，它的复杂度就提高了，因为类成员变量的作用于超出了函数体，使得函数就和类中其他函数耦合，所以不得不进行阅读更多以帮助理解该函数：
~~~ java
    public static class RecycledViewPool {
        //同类ViewHolder缓存个数上限
        private static final int DEFAULT_MAX_SCRAP = 5;

        /**
         * Tracks both pooled holders, as well as create/bind timing metadata for the given type.
         * 回收池中存放单个类型ViewHolder的容器
         */
        static class ScrapData {
            //同类ViewHolder存储在ArrayList中
            ArrayList<ViewHolder> mScrapHeap = new ArrayList<>();
            //每种类型的ViewHolder最多存5个
            int mMaxScrap = DEFAULT_MAX_SCRAP;
        }
        //回收池中存放所有类型ViewHolder的容器
        SparseArray<ScrapData> mScrap = new SparseArray<>();
        ...
        //ViewHolder入池 按viewType分类入池，一个类型的ViewType存放在一个ScrapData中
        public void putRecycledView(ViewHolder scrap) {
            final int viewType = scrap.getItemViewType();
            final ArrayList<ViewHolder> scrapHeap = getScrapDataForType(viewType).mScrapHeap;
            //如果超限了，则放弃入池
            if (mScrap.get(viewType).mMaxScrap <= scrapHeap.size()) {
                return;
            }
            if (DEBUG && scrapHeap.contains(scrap)) {
                throw new IllegalArgumentException("this scrap item already exists");
            }
            scrap.resetInternal();
            //回收时，ViewHolder从列表尾部插入
            scrapHeap.add(scrap);
        }
        //从回收池中获取ViewHolder对象
        public ViewHolder getRecycledView(int viewType) {
              final ScrapData scrapData = mScrap.get(viewType);
              if (scrapData != null && !scrapData.mScrapHeap.isEmpty()) {
                  final ArrayList<ViewHolder> scrapHeap = scrapData.mScrapHeap;
                  //复用时，从列表尾部获取ViewHolder（优先复用刚入池的ViewHoler）
                  return scrapHeap.remove(scrapHeap.size() - 1);
              }
              return null;
        }
}
~~~

上述代码列出了RecycledViewPool 中最关键的一个成员变量和两个函数。至此可以得出结论：RecycledViewPool中的ViewHolder存储在SparseArray中，并且按viewType分类存储（即是Adapter.getItemViewType()的返回值），同一类型的ViewHolder存放在ArrayList 中，且默认最多存储5个。

相比较于mCachedViews，**从mRecyclerPool中成功获取ViewHolder对象后并没有做合法性和表项位置校验，只检验viewType是否一致**。所以从mRecyclerPool中取出的ViewHolder只能复用于相同viewType的表项。

#### 创建ViewHolder并绑定数据

~~~ java
ViewHolder tryGetViewHolderForPositionByDeadline(int position,boolean dryRun, long deadlineNs) {
            ...
            //所有缓存都没有命中，只能创建ViewHolder
            if (holder == null) {
                ...
                holder = mAdapter.createViewHolder(RecyclerView.this, type);
                ...
            }
            ...
            boolean bound = false;
            if (mState.isPreLayout() && holder.isBound()) {
                // do not update unless we absolutely have to.
                holder.mPreLayoutPosition = position;
            }
            //如果表项没有绑定过数据 或 表项需要更新 或 表项无效 且表项没有被移除时绑定表项数据
            else if (!holder.isBound() || holder.needsUpdate() || holder.isInvalid()) {
                if (DEBUG && holder.isRemoved()) {
                    throw new IllegalStateException("Removed holder should be bound and it should"
                            + " come here only in pre-layout. Holder: " + holder
                            + exceptionLabel());
                }
                final int offsetPosition = mAdapterHelper.findPositionOffset(position);
                //为表项绑定数据
                bound = tryBindViewHolderByDeadline(holder, offsetPosition, position, deadlineNs);
            }
}
~~~

- 再进行了上述所有尝试后，如果依然没有获得ViewHolder，只能重新创建并绑定数据。沿着调用链往下，就会找到熟悉的`onCreateViewHolder()`和`onBindViewHolder()`。
- 绑定数据的逻辑嵌套在一个大大的if中（原来**并不是每次都要绑定数据，只有满足特定条件时才需要绑定**）
- 那什么情况下需要绑定，什么情况下不需要呢？这就要引出“缓存优先级”这个概念。

### 缓存优先级

缓存有优先级一说，在使用图片二级缓存（内存+磁盘）时，会先尝试去优先级高的内存中获取，若未命中再去磁盘中获取。优先级越高意味着性能越好。RecyclerView的缓存机制中是否也能套用“缓存优先级”这一逻辑？

虽然为了获取ViewHolder做了5次尝试（共从6个地方获取），先排除3种特殊情况，即从mChangedScrap获取、通过id获取、从自定义缓存获取，正常流程中只剩下3种获取方式，优先级从高到低依次是：

- 1.从mAttachedScrap获取
- 2.从mCachedViews获取
- 3.从mRecyclerPool 获取

这样的缓存优先级是不是意味着，对应的复用性能也是从高到低？（复用性能越好意味着所做的昂贵操作越少）

- 最坏情况：重新创建ViewHodler并重新绑定数据
- 次好情况：复用ViewHolder但重新绑定数据
- 最好情况：复用ViewHolder且不重新绑定数据

毫无疑问，所有缓存都未命中的情况下会发生最坏情况。剩下的两种情况应该由3种获取方式来分摊，猜测优先级最低的 mRecyclerPool 方式应该命中次好情况，而优先级最高的 mAttachedScrap 应该命中最好情况，去源码中验证一下：

~~~ java
ViewHolder getScrapOrHiddenOrCachedHolderForPosition(int position, boolean dryRun) {
       final int scrapCount = mAttachedScrap.size();

       // Try first for an exact, non-invalid match from scrap.
       //1.从attached scrap回收集合中
       for (int i = 0; i < scrapCount; i++) {
           final ViewHolder holder = mAttachedScrap.get(i);
           //只有当holder是有效时才返回
           if (!holder.wasReturnedFromScrap() && holder.getLayoutPosition() == position
                   && !holder.isInvalid() && (mState.mInPreLayout || !holder.isRemoved())) {
               holder.addFlags(ViewHolder.FLAG_RETURNED_FROM_SCRAP);
               return holder;
           }
       }
}

ViewHolder tryGetViewHolderForPositionByDeadline(int position,
                boolean dryRun, long deadlineNs) {
      ...
      if (holder == null) { 
          ...
          //从回收池中获取ViewHolder对象
          holder = getRecycledViewPool().getRecycledView(type);
          if (holder != null) {
               //重置ViewHolder
               holder.resetInternal();
               if (FORCE_INVALIDATE_DISPLAY_LIST) {
                    invalidateDisplayListInt(holder);
               }
          }
      }
      ...
      //如果表项没有绑定过数据 或 表项需要更新 或 表项无效 且表项没有被移除时绑定表项数据
      else if (!holder.isBound() || holder.needsUpdate() || holder.isInvalid()) {
          if (DEBUG && holder.isRemoved()) {
              throw new IllegalStateException("Removed holder should be bound and it should"
                            + " come here only in pre-layout. Holder: " + holder
                            + exceptionLabel());
          }
          final int offsetPosition = mAdapterHelper.findPositionOffset(position);
          //为表项绑定数据
          bound = tryBindViewHolderByDeadline(holder, offsetPosition, position, deadlineNs);
      }
      ...
}

public abstract static class ViewHolder {
        /**
         * This ViewHolder has been bound to a position; mPosition, mItemId and mItemViewType
         * are all valid.
         * 绑定标志位
         */
        static final int FLAG_BOUND = 1 << 0;
        /**
         * This ViewHolder’s data is invalid. The identity implied by mPosition and mItemId
         * are not to be trusted and may no longer match the item view type.
         * This ViewHolder must be fully rebound to different data.
         * 无效标志位
         */
        static final int FLAG_INVALID = 1 << 2;
        //判断ViewHolder是否无效
        boolean isInvalid() {
            //将当前ViewHolder对象的flag和无效标志位做位与操作
            return (mFlags & FLAG_INVALID) != 0;
        }
        //判断ViewHolder是否被绑定
        boolean isBound() {
            //将当前ViewHolder对象的flag和绑定标志位做位与操作
            return (mFlags & FLAG_BOUND) != 0;
        }
        /**
         * 将ViewHolder重置
         */
        void resetInternal() {
            //将ViewHolder的flag置0
            mFlags = 0;
            mPosition = NO_POSITION;
            mOldPosition = NO_POSITION;
            mItemId = NO_ID;
            mPreLayoutPosition = NO_POSITION;
            mIsRecyclableCount = 0;
            mShadowedHolder = null;
            mShadowingHolder = null;
            clearPayload();
            mWasImportantForAccessibilityBeforeHidden = ViewCompat.IMPORTANT_FOR_ACCESSIBILITY_AUTO;
            mPendingAccessibilityState = PENDING_ACCESSIBILITY_STATE_NOT_SET;
            clearNestedRecyclerViewIfNotNested(this);
        }
}
~~~

温故知新，回看 mRecyclerPool 复用逻辑时，发现在成功获得ViewHolder对象后，立即对其重置（将flag置0）。这样就满足了绑定数据的判断条件（因为0和非0位与之后必然为0）。

同样的，在才mAttachedScrap中获取ViewHolder时，只有当其是有效的才会返回。所以猜测成立：从mRecyclerPool中复用的ViewHolder需要重新绑定数据，从mAttachedScrap 中复用的ViewHolder不要重新出创建也不需要重新绑定数据。

### 总结

- 在 RecyclerView 中，并不是每次绘制表项，都会重新创建 ViewHolder 对象，也不是每次都会重新绑定 ViewHolder 数据。

- RecyclerView 通过Recycler获得下一个待绘制表项。
- Recycler有4个层次用于缓存 ViewHolder 对象，优先级从高到底依次为
    - `ArrayList<ViewHolder> mAttachedScrap`、
    - `ArrayList<ViewHolder> mCachedViews`、
    - `ViewCacheExtension mViewCacheExtension`、
    - `RecycledViewPool mRecyclerPool`。

- 如果四层缓存都未命中，则重新创建并绑定 ViewHolder 对象。

- RecycledViewPool 对 ViewHolder 按**viewType分类存储**（通过SparseArray），**同类 ViewHolder 存储在默认大小为5的ArrayList中**。

- **从mRecyclerPool中复用的 ViewHolder 需要重新绑定数据，从mAttachedScrap 中复用的 ViewHolder 不需要重新创建也不需要重新绑定数据**。

- **从mRecyclerPool中复用的ViewHolder ，只能复用于viewType相同的表项，从mCachedViews中复用的 ViewHolder ，只能复用于指定位置的表项**。

- **mCachedViews是离屏缓存，用于缓存指定位置的 ViewHolder ，只有“列表回滚”这一种场景（刚滚出屏幕的表项再次进入屏幕）**，才有可能命中该缓存。该缓存存放在默认大小为 2 的ArrayList中。

这篇文章粗略的回答了关于“复用”的4个问题，即“复用什么？”、“从哪里获得复用？”、“什么时候复用？”、“复用优先级”。读到这里，可能会有很多疑问：

### 预布局、预测动画是什么？
关于预测动画和预布局的介绍，可以看这篇文章：[深入理解 RecyclerView 的缓存机制](https://juejin.cn/post/6844904146684870669)

### RecyclerView的滚动和回收机制：

RecyclerView之所以能滚动，就是因为它在监听到手指滑动之后，不断地更新Item的位置，也就是反复layout子View了，这部分工作由LayoutManager负责。

LayoutManager在layout子View之前，会先把RecyclerView的每个子View所对应的ViewHolder都放到mAttachedScrap中，然后根据当前滑动距离，筛选出哪些Item需要layout。获取子View对象，会通过getViewForPosition方法来获取。这个方法就是题目中说的那样：先从mAttachedScrap中找，再......

Item布局完成之后，会对刚刚没有再次布局的Item进行缓存（回收），这个缓存分两种：

- 取出（即将重用）时无须重新绑定数据（即不用执行onBindViewHolder方法）。这种缓存只适用于特定position的Item（名花有主）；
- 取出后会回调onBindViewHolder方法，好让对应Item的内容能正确显示。这种缓存适用所有同类型的Item（云英未嫁）；
第一种缓存，由Recycler.mCachedViews来保管，第二种放在RecycledViewPool中。

如果回收的Item它的状态（包括：INVALID、REMOVED、UPDATE、POSITION_UNKNOWN）没有变更，就会放到mCachedViews中，否则扔RecycledViewPool里。

emmmm，除了滚动过程中，会对Item进行回收和重新布局，还有一种就是，当Adapter数据有更新时：

Inserted：如果刚好插入在屏幕可见范围内，会从RecycledViewPool中找一个相同类型的ViewHolder（找不到就create）来重新绑定数据并layout；

Removed：会把对应ViewHolder扔到mAttachedScrap中并播放动画，动画播放完毕后移到RecycledViewPool里；

Changed：这种情况并不是大家所认为的：直接将这个ViewHolder传到Adapter的onBindViewHolder中重新绑定数据。而是先把旧的ViewHolder扔mChangedScrap中，然后像Inserted那样从RecycledViewPool中找一个相同类型的ViewHolder来重新绑定数据。旧ViewHolder对象用来播放动画，动画播完，同样会移到RecycledViewPool里；

> 注意：如果是使用notifyDataSetChanged方法来通知更新的话，那么所有Item都会直接扔RecycledViewPool中，然后逐个重新绑定数据的。

**这几个存放缓存的集合，各自的作用以及使用场景？**

- mAttachedScrap：LayoutManager每次`layout`子View之前，那些已经添加到RecyclerView中的Item以及被删除的Item的临时存放地。使用场景就是RecyclerView滚动时、还有在可见范围内删除Item后用`notifyItemRemoved`方法通知更新时；

- mChangedScrap：作用：**存放可见范围内有更新的Item**。使用场景：可见范围内的Item有更新，并且使用`notifyItemChanged`方法通知更新时；

- mCachedViews：作用：**存放滚动过程中没有被重新使用且状态无变化的那些旧Item**。场景：滚动，prefetch；

- RecycledViewPool：作用：**缓存Item的最终站**，用于保存那些`Removed`、`Changed`、以及mCachedViews满了之后更旧的Item。场景：Item被移除、Item有更新、滚动过程；

写到这里发现漏讲了一个prefetch，好吧，这个prefetch机制就是RecyclerView在滚动和惯性滚动的时候，借助Handler来事先从RecycledViewPool中取出即将要显示的Item，随即扔到mCachedViews中，这样的话，当layout到这个Item时，就能直接拿来用而不用绑定数据了。




### Ref
[深入理解 RecyclerView 的缓存机制](https://juejin.cn/post/6844904146684870669)
