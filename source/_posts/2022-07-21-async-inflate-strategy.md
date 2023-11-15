---
layout: post
title: RecyclerView inflate优化
category: accumulation
tags:
  - RecyclerView optimization
  - AsyncInflate
keywords: RecyclerView, infalte
banner: https://cdn.conorlee.top/Green%20Wheat%20Fields.jpg
thumbnail: https://cdn.conorlee.top/Green%20Wheat%20Fields.jpg
toc: true
---

### 背景
本文是在工作中对App启动耗时中页面展现耗时的一个优化，特意记录优化方案和遇到的问题。
主要是针对首页Recyclerview itemview的一个优化，减少itemview inflate耗时，从而减少onCreateViewHolder耗时，最终减少页面展现的耗时

<!--more-->
### 思路
关于view的异步inflate，官方给出了一个方案：[AsyncLayoutInflater](https://developer.android.com/jetpack/androidx/releases/asynclayoutinflater?hl=zh-cn)。对其原理感兴趣的朋友可以看下这篇文章：
<https://juejin.cn/post/6844904170508681224>

关于如何减少ViewHolder的inflate时间，基于官方的方法，我做了两套方案：

### 1.异步初始化viewholder和 2.提前初始化viewholder

~~~ kotlin
private var sUseAsyncInflate = false
private var sUseAheadInflate = false

fun init(application: Application?) {
    if (Build.VERSION.SDK_INT < Build.VERSION_CODES.Q) { // android 10
        // 低端设备使用提前初始化viewholder
        aheadInflateView(application, sFirstScreenTypes)
        sUseAheadInflate = true
//            sUseAsyncInflate = true // 可以都开启，但是低端设备也异步会导致主线程事务增多
    } else {
        // 高端设备异步初始化viewholder
//            aheadInflateView(application, sFirstScreenTypes) // 提前初始化多个，可能未初始化完首页就已经展示了
        sUseAsyncInflate = true
    }
}
~~~

### 一、提前初始化viewholder
提前初始化比较简单，只需要在首页展示之前初始化好对应的viewholder，我的方案细节如下：

- 在Application里仅异步初始化首屏viewholder
- AsyncLayoutInflater异步初始化需要指定一个ViewParent，这种场景是Recyclerview
- Recyclerview必须指定一个LayoutManager，这里和首页一样，使用的LinearLayoutManager
- 异步初始化完成后，需要移除itemview的parent

~~~ kotlin
private var sFirstScreenTypes: IntArray = intArrayOf( // 首屏对应的viewholder的type
    CommonConstants.card_show_subtype_13,
    CommonConstants.card_show_subtype_551,
    CommonConstants.card_show_subtype_14
)

/**
* 提前初始化首屏 view
*/
private fun aheadInflateView(application: Context?, viewTypes: IntArray) {
    sFirstScreenTypes = viewTypes
    sAsyncLayoutInflater = MyAsyncLayoutInflater(application!!)
    sFakeParent = RecyclerView(application)
    sFakeParent!!.layoutManager = LinearLayoutManager(application)
    for (viewType in viewTypes) {
        val model = ViewHolderTypeManager.transformViewHolder(viewType) // model是一个自定义类，绑定了viewtype和layoutid
        var resid = model.layoutId
        if (resid < 0) {
            resid = CommonGlobalContext.getAppContext().resources.getIdentifier(
                model.layout, "layout",
                CommonGlobalContext.getAppContext().packageName
            )
        }
        sAsyncLayoutInflater!!.inflate(
            resid, sFakeParent, viewType, null
        ) { view, resid, parent, viewType, fakeViewHolder ->
            if (null != view.parent) { // 移除itemview的parent
                (view.parent as ViewGroup).removeView(view)
            }
            DebugLog.d(TAG, "aheadInflateView 完成 $viewType")
            saveAsyncInflateView(application, viewType, view) // 保存初始化好的viewholder，方便首页使用
        }
    }
}



/**
    * 根据type，提前异步初始化一个view
*/
private fun saveAsyncInflateView(context: Context, type: Int, view: View?): Int {
    if (!typeToContainer.containsKey(type)) {
        val container = ViewHolderContainer(context, type, mReferenceQueue)
        container.position = -1
        container.isFake = false
        container.view = view
        typeToContainer[type] = container
    } else {
        // TODO 后续处理提前初始化一个type多个view的情况
    }
    return -1
}
~~~

ViewHolderContainer 是一个链表类，内部有一个next指针，所以typeToContainer其实是一个type对应一个链表
~~~ kotlin
private val typeToContainer: MutableMap<Int, ViewHolderContainer?> = HashMap() // map(viewtype, container)作为存储容器
~~~

使用的时候直接在onCreateViewHolder中判断即可

### 二、异步初始化viewholder
异步初始化稍微复杂一点，思路是需要先绑定一个`fake viewholder`，等 `real viewholder` infalte完成在替换为`real viewholder`

基本原理如下：

#### 1.区分占位viewholer和原viewholder
因为是异步的，在`real view`没有inflate完成时，需要展示一个占位的view。**占位view的viewtype = 原viewtype * (-1)**

#### 2.原viewholder inflate完成后，替换掉占位viewholer
占位view在原本view inflate完成后，需要被替换，需要保存占位view的位置，然后更新为 `real view`

实现方案：

- **1.不缓存占位（fake）viewholder**

~~~ java
// 此处 type 为负数
mRecyclerView.getRecycledViewPool().setMaxRecycledViews(type, 0); // RecycledViewPool不缓存 负数类型的viewholder
~~~

- **2.因为不缓存，所以在调用 notifyItemChanged(int position) 时会重新走到 onCreateViewHolder**

简单说下 Recyclerview 分为四级缓存，1.mChangedScrap 2.mAttachedScrap、mCachedViews 3.mViewCacheExtension 4.mRecyclerPool

更多Recyclerview缓存相关的细节分析，请看我的这篇文章：[RecyclerView 缓存机制](<https://conorlee.top/2021/07/18/recyclerview-cache/>)

下面贴一下大概的实现方案：

~~~ java
    @Override
    public int getItemViewType(int position) {
       int type = ViewHolderTypeManager.transformViewHolder((Card) mDataList.get(position)).getType();
       // 先拿到正确的type，判断该type是否有开启了异步初始化，然后判断是否是fake
       if (mUseAsyncInflate && AsyncInflateHelper.INSTANCE.isFake(mContext, type, originPos, false)) {// 这里并未绑定type和position
            type *=-1;
            mRecyclerView.getRecycledViewPool().setMaxRecycledViews(type, 0); // recycledPool 不缓存 fake的
        }
    }

    // 
    public RecyclerView.ViewHolder onCreateViewHolder(@NonNull ViewGroup parent, int viewType) {
        boolean useFake = mUseAsyncInflate;
        if(useFake) {
            View view = AsyncInflateHelper.INSTANCE.getContainerForType(viewType);
            if (!isFake) {
                if (null != view) {
                    DebugLog.d("ConorLee", "use async inflate viewtype is " + viewType);
                    tmpViewHolder = (BaseNewViewHolder) getViewHolder(view, model.getClassName());
                } else {
                    useFake = false;
                }
            } else {
                View viewGroup = LayoutInflater.from(mContext).inflate(R.layout.common_album_item_default, parent, false);
                tmpViewHolder = new EmptyViewHolder(mContext, viewGroup);
                DebugLog.d("ConorLee", "fake onCreate is  "+ viewType);

                int layoutId = model.getLayoutId();
                if (layoutId < 0) {
                    layoutId = CommonGlobalContext.getAppContext().getResources().getIdentifier(model.getLayout(), "layout",
                            CommonGlobalContext.getAppContext().getPackageName());
                }
                MyAsyncLayoutInflater asyncLayoutInflater = new MyAsyncLayoutInflater(mContext);
                asyncLayoutInflater.inflate(layoutId, parent, viewType, tmpViewHolder, new MyAsyncLayoutInflater.OnInflateFinishedListener() {
                    @Override
                    public void onInflateFinished(@NonNull View view, int resid, @Nullable ViewGroup parent, int viewType, BaseNewViewHolder fakeViewHolder) {
                        DebugLog.d("ConorLee", "async inflate complete  "+ viewType); // inflate完成
                        int pos = AsyncInflateHelper.INSTANCE.updatePosAndView(viewType, view);
                        notifyItemChanged(pos); // 这里pos是在onBindViewHolder()方法里绑定好了
                        if (pos == -1) {
                            DebugLog.d("ConorLee", "async inflate error "+ viewType);
                        }
                    }
                });
            }
        }
    }
~~~

在onBindViewHolder()方法中如果发现holder是fake，那么可以直接return，显著减少onBind时间
~~~ java
public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position, @NonNull List<Object> payloads) {
    int viewHolderType = holder.getItemViewType();
    if (mUseAsyncInflate && viewHolderType < 0) {
        AsyncInflateHelper.INSTANCE.isFake(mContext, viewHolderType, position, true); // 这里绑定绑定type和position
        return;
    }
    // ...
}
~~~

这里再说明一下：

- fakeviewholder是在onCreateViewHolder中创建，并且在onBindViewHolder和position绑定，绑定之后由前面的typeToContainer类保存。
- 异步初始化完成后，根据type在typeToContainer中找到第一个fakeviewholder，拿到位置并更新

### 跨页面缓存相同type的ViewHolder，如何回收？
可以参考LeakCanary检测Activity是否泄漏的方法，检测viewholder的View或Context是否被回收。
基本原理如下：
- 用弱引用（WeakReference）包裹实体类ViewHolderContainer，然后给这个弱引用绑定一个引用队列。
- 如果弱引用的内容被回收，该弱引用会被加入到引用队列。然后某个时间点遍历引用队列，回收ViewHolderContainer
- 因为Activity生命周期的原因，如果在onDestroy里立刻去遍历引用队列，可能还未回收，所以onDestroy方法不是一个完美的时间点
- 建议可以在上一个页面的onResume方法中延迟去做

清除的方法大概如下：

~~~ kotlin
fun clearViewContainer() {
    var needLoop = true
    while (needLoop) {
        val ref = mReferenceQueue.poll() as KeyedWeakReference<*>?
        ref?.let {
            var head = typeToContainer[ref.viewHolderType]
            var prev: ViewHolderContainer? = null
            while (head != null) {
                if (head.mKeyedWeakReference.get() == null) {
                    DebugLog.d(TAG, "页面退出，回收container type is " + ref.viewHolderType + " pos is  " + ref.viewHolderPos)
                    // 移除head节点
                    if (null == prev) {
                        head = head.next
                        typeToContainer[ref.viewHolderType] = head
                    } else {
                        prev.next = head.next
                    }
                    break
                }
                prev = head
                head = head.next
            }
        } ?: let{ needLoop = false}
    }
}
~~~


### 结论

采用这两种方式，可以显著减少viewholder的onCreateViewHolder和onBindViewHolder这两个方法的耗时，从而页面整体展现可以较以前提高100~200ms

**不足：**

- 1.异步初始化完成后，其实应该根据当前列表的firstvisiblepos和lastvisiblepos找到对应的fakeviewholder，这样效率更高。目前为了实现简单，每次异步完成都是遍历列表，拿第一个fakeviewholder，儿没有判断是否可见
- 2.不支持在viewholder中new Handler(), 因为底层用的官方AsyncLayoutInflater，初始化view在子线程，此时new Handler()会抛出异常

### Ref

[View 的异步 Inflate+ 全局缓存：加速你的页面](<https://www.infoq.cn/article/pwylrfrsh8wce1ltyb1u>)
