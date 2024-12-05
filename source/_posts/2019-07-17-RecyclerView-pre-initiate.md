---
layout: post
title: RecyclerView预加载机制源码分析
category: accumulation
tags:
    - RecyclerView
keywords: RecyclerView, Design Pattern
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Farmhouse%20with%20Two%20Figures.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Farmhouse%20with%20Two%20Figures.jpg
toc: true
---

### RecyclerView四级缓存
再次总结一下 RecyclerView 的四级缓存
- Scrap
- Cache
- ViewCacheExtension
- RecycledViewPool
<!--more-->
**Scrap**: 对应ListView 的Active View，就是屏幕内的缓存数据，就是相当于换了个名字，可以直接拿来复用。

**Cache**: 刚刚移出屏幕的缓存数据，**默认大小是2个**，当其容量被充满同时又有新的数据添加的时候，会根据FIFO原则，把优先进入的缓存数据移出并放到下一级缓存中，然后再把新的数据添加进来。Cache里面的数据是干净的，也就是携带了原来的ViewHolder的所有数据信息，数据可以直接来拿来复用。需要注意的是，**cache是根据position来寻找数据的**，这个postion是根据第一个或者最后一个可见的item的position以及用户操作行为（上拉还是下拉）。
举个栗子：当前屏幕内第一个可见的item的position是1，用户进行了一个下拉操作，那么当前预测的position就相当于（1-1=0），也就是position=0的那个item要被拉回到屏幕，此时RecyclerView就从Cache里面找position=0的数据，如果找到了就直接拿来复用。

**ViewCacheExtension**: 是google留给开发者自己来自定义缓存的，这个ViewCacheExtension我个人建议还是要慎用，因为我扒拉扒拉网上其他的博客，没有找到对应的使用场景，而且这个类的api设计的也有些奇怪，只有一个`public abstract View getViewForPositionAndType(@NonNull Recycler recycler, int position, int type);`让开发者重写通过position和type拿到ViewHolder的方法，却没有提供如何产生ViewHolder或者管理ViewHolder的方法，给人一种只出不进的赶脚，还是那句话慎用。

**RecycledViewPool**: 刚才说了Cache默认的缓存数量是2个，当Cache缓存满了以后会根据FIFO（先进先出）的规则把Cache先缓存进去的ViewHolder移出并缓存到RecycledViewPool中，RecycledViewPool默认的缓存数量是5个。RecycledViewPool与Cache相比不同的是，从Cache里面移出的ViewHolder再存入RecycledViewPool之前ViewHolder的数据会被全部重置，相当于一个新的ViewHolder，而且Cache是根据position来获取ViewHolder，而**RecycledViewPool是根据itemType获取的**，如果没有重写getItemType()方法，itemType就是默认的。因为RecycledViewPool缓存的ViewHolder是全新的，所以取出来的时候需要走**onBindViewHolder()**方法。

#### ref
[让你彻底掌握RecyclerView的缓存机制](https://www.jianshu.com/p/3e9aa4bdaefd)

### Prefetch功能的使用
google官方在 Support Library v25 版本中，为RecyclerView增加了Prefetch。 并且在 v25.1.0 以及25.3.0版本中进行了完善。在最新的稳定版本25.3.1中已经基本稳定。
Prefetch 默认就是处理开启的状态，通过LinearLayoutManager的setItemPrefetchEnabled()我们可以手动控制该功能的开启关闭。

我们都知道android是通过每16ms刷新一次页面来保证ui的流畅程度，现在android系统中刷新ui会通过cpu产生数据，然后交给gpu渲染的形式来完成，从上图可以看出当cpu完成数据处理交给gpu后就一直处于空闲状态，需要等待下一帧才会进行数据处理，而这空闲时间就被白白浪费了，如何才能压榨cpu的性能，让它一直处于忙碌状态，这就是rv的预取功能(Prefetch)要做的事情，rv会预取接下来可能要显示的item，在下一帧到来之前提前处理完数据，然后将得到的itemholder缓存起来，等到真正要使用的时候直接从缓存取出来即可。

![](/images/blogimages/2019/recyclerview_prefetch.jpeg)

#### 预取代码理解

虽说预取是默认开启不需要我们开发者操心的事情，但是明白原理还是能加深该功能的理解。下面就说下自己在看预取源码时的一点理解。实现预取功能的一个关键类就是gapworker，可以直接在rv源码中找到该类
~~~ Java
GapWorker mGapWorker;
~~~

rv通过在ontouchevent中触发预取的判断逻辑，在手指执行move操作的代码末尾有这么段代码
~~~ Java
case MotionEvent.ACTION_MOVE: {
               ......
                    if (mGapWorker != null && (dx != 0 || dy != 0)) {
                        mGapWorker.postFromTraversal(this, dx, dy);
                    }
                }
            } break;
~~~

通过每次move操作来判断是否预取下一个可能要显示的item数据，判断的依据就是通过传入的dx和dy得到手指接下来可能要移动的方向，如果dx或者dy的偏移量会导致下一个item要被显示出来则预取出来，但是并不是说预取下一个可能要显示的item一定都是成功的，其实每次rv取出要显示的一个item本质上就是取出一个viewholder，根据viewholder上关联的itemview来展示这个item。而取出viewholder最核心的方法就是
~~~ Java
tryGetViewHolderForPositionByDeadline(int position,boolean dryRun, long deadlineNs)
~~~
名字是不是有点长，在rv源码中你会时不时见到这种巨长的方法名，看方法的参数也能找到和预取有关的信息,deadlineNs的一般取值有两种，一种是为了兼容版本25之前没有预取机制的情况，兼容25之前的参数为
~~~ Java
    static final long FOREVER_NS = Long.MAX_VALUE;
~~~
，另一种就是实际的deadline数值，超过这个deadline则表示预取失败，这个其实也好理解，预取机制的主要目的就是提高rv整体滑动的流畅性，如果要预取的viewholder会造成下一帧显示卡顿强行预取的话那就有点本末倒置了。
关于预取成功的条件通过调用
~~~ Java
boolean willCreateInTime(int viewType, long approxCurrentNs, long deadlineNs) {
            long expectedDurationNs = getScrapDataForType(viewType).mCreateRunningAverageNs;
            return expectedDurationNs == 0 || (approxCurrentNs + expectedDurationNs < deadlineNs);
}
~~~
来进行判断，approxCurrentNs的值为
~~~ Java
long start = getNanoTime();
if (deadlineNs != FOREVER_NS
                            && !mRecyclerPool.willCreateInTime(type, start, deadlineNs)) {
         // abort - we have a deadline we can't meet
        return null;
}
~~~
而mCreateRunningAverageNs就是创建同type的holder的平均时间，感兴趣的可以去看下这个值如何得到，不难理解就不贴代码了。关于预取就说到这里，感兴趣的可以自己去看下其余代码的实现方式，可以说google对于rv还是相当重视的，煞费苦心提高rv的各种性能，据说最近推出的viewpager2控件就是通过rv来实现的，大有rv控件一统天下的感觉。

#### 预取功能的应用
预取功能是默认开启的，在对应的RecyclerView.LayoutManager中提供了一个`setInitialPrefetchItemCount(int itemCount)`来设置预取个数。有一种场景，比如你在垂直列表里面有一个水平滚动列表的时候，竖屏每一行都是展示三个半item，可以调用**内部**（横向）Recyclerview的 `innerLLM.setInitialItemsPrefetchCount(4)`，这样当水平列表将要展示在屏幕上的时候，如果UI线程有空闲时间，RecyclerView会尝试在内部预先把这几个item取出来。

#### Ref

RecyclerView预加载机制源码分析 :https://blog.csdn.net/weishenhong/article/details/81150172

https://www.jianshu.com/p/1d2213f303fc

### 延伸阅读Recyclerview缓存
[RecyclerView 源码分析(三) - RecyclerView的缓存机制](https://www.jianshu.com/p/efe81969f69d)
[【进阶】RecyclerView源码解析(三)——深度解析缓存机制](https://www.jianshu.com/p/2b19e9bcda84)