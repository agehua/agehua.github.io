---
layout: post
title: RxJava 学习
category: technology
tags:
  - RxJava
  - RxAndroid
keywords: RxJava RxAndroid
description: RxAndroid学习
banner: http://cdn.conorlee.top/Bulb%20Fields.jpg
thumbnail: http://cdn.conorlee.top/Bulb%20Fields.jpg
---


### 1.向前辈致敬
[给Android开发者的RxJava详解](http://gank.io/post/560e15be2dca930e00da1083)，这篇文章讲解详细，如果耐心看完，相信肯定收获不小

    本篇博文用作学习RxJava过程中，记录知识和心得，如有冒犯还请见谅！

### 2.简单介绍
RxJava是通过一种扩展的观察者模式来实现的。
RxJava有四个基本概念：Observable(可观察者，即被观察者)、 Observer (观察者)、 subscribe (订阅)、事件。Observable和Observer通过subscribe()方法实现订阅关系，从而Observable可以在需要的时候发出事件来通知Observer。

<!--more-->

与传统观察者模式不同， RxJava 的事件回调方法除了普通事件onNext()（相当于onClick()/onEvent()）之外，还定义了两个特殊的事件：onCompleted()和onError()。

> RxJava的基本实现主要有三点：Observer的创建、Observable的创建、Scheduler线程控制

### 3.Observer的创建方法

Observer 即观察者，它决定事件触发的时候将有怎样的行为。 RxJava 中的 Observer 接口的实现方式：

~~~ Java
Observer<String> observer = new Observer<String>() {
    @Override
    public void onNext(String s) {
        Log.d(tag, "Item: " + s);
    }

    @Override
    public void onCompleted() {
        Log.d(tag, "Completed!");
    }

    @Override
    public void onError(Throwable e) {
        Log.d(tag, "Error!");
    }
};
~~~
除了 Observer 接口之外，RxJava还内置了一个实现了Observer的抽象类：Subscriber。 Subscriber对Observer接口进行了一些扩展，但他们的基本使用方式是完全一样的：

~~~ Java
Subscriber<String> subscriber = new Subscriber<String>() {
    @Override
    public void onNext(String s) {
        Log.d(tag, "Item: " + s);
    }

    @Override
    public void onCompleted() {
        Log.d(tag, "Completed!");
    }

    @Override
    public void onError(Throwable e) {
        Log.d(tag, "Error!");
    }
};
~~~

Observer和Subscriber区别对于使用者来说主要有两点：

- onStart(): 这是Subscriber增加的方法。

    它会在subscribe刚开始，而事件还未发送之前被调用，可以用于做一些准备工作，例如数据的清零或重置。这是一个可选方法，默认情况下它的实现为空。
    > 需要注意的是，如果对准备工作的线程有要求（例如弹出一个显示进度的对话框，这必须在主线程执行）， onStart()就不适用了，因为它总是在subscribe 所发生的线程被调用，而不能指定线程。要在指定的线程来做准备工作，可以使用 doOnSubscribe() 方法。

- unsubscribe(): 这是Subscriber所实现的另一个接口Subscription的方法，用于取消订阅。

    在这个方法被调用后，Subscriber将不再接收事件。一般在这个方法调用前，可以使用isUnsubscribed()先判断一下状态。 **unsubscribe()**这个方法很重要，因为在subscribe()之后，Observable会持有 Subscriber 的引用，这个引用**如果不能及时被释放**，将有**内存泄露的风险**。所以最好保持一个原则：要在不再使用的时候尽快在合适的地方（例如 onPause() onStop() 等方法中）调用 unsubscribe() 来解除引用关系，以避免内存泄露的发生。

### 4.Observable的几种创建方法：

#### **1.Observable.just(T...)和from(T[])/from(Iterable<? extends T>)**

  - 1). just(T...): 将传入的参数依次发送出来

  - 2). from(T[])/from(Iterable<? extends T>): 将传入的数组或 Iterable 拆分成具体对象后，依次发送出来。

  - 3). 由这两个方法创建的Observable对象的特点是：所有Observer一旦订阅这个Observable就会立即调用onNext()方法并传入Observable.just()/from()的参数，而后因为Observable没有数据可以发送了，onComplete()方法会被调用。

  ~~~ Java
  Observable<List<String>> listObservable = Observable.just(getColorList());
  ~~~

  > 注意，如果just()中传入的是耗时方法，该方法会被立即执行并阻塞UI线程。这里的getColorList()是一个不耗时的方法.

    下一步，我们写一个Observer来观察Observable。

~~~ Java
listObservable.subscribe(new Observer<List<String>>() {

    @Override
    public void onCompleted() { }

    @Override
    public void onError(Throwable e) { }

    @Override
    public void onNext(List<String> colors) {
        mSimpleStringAdapter.setStrings(colors);
    }
});
~~~
  在这个例子中我们不关心Observable何时完成数据的传输，所以我们不用在onComplete()方法里写代码。而且在这里不会有异常抛出，所以我们也不用管onError()方法

#### **2.Observable.fromCallable()**

  先看代码：

~~~ Java
Observable<List<String>> tvShowObservable = Observable.fromCallable(new Callable<List<String>>() {

      @Override
      public List<String> call() {
          return mRestClient.getFavoriteTvShows();
      }
});
~~~

使用Observable.fromCallable()方法有两点好处：

  - 1).获取要发送的数据的代码只会在有Observer订阅之后执行。

  - 2).获取数据的代码（指的是call()方法）可以在子线程中执行。对比just()中传入的方法只能运行在主线程。

    这两点好处有时可能非常重要。

现在让我们订阅这个Observable。

~~~ Java
mTvShowSubscription = tvShowObservable
    .subscribeOn(Schedulers.io())
    .observeOn(AndroidSchedulers.mainThread())
    .subscribe(new Observer<List<String>>() {

        @Override
        public void onCompleted() { }

        @Override
        public void onError(Throwable e) { }

        @Override
        public void onNext(List<String> tvShows){
            displayTvShows(tvShows);
        }
});
~~~

  上面代码逐一介绍：

  - 1).subscribeOn() 指定事件发生的线程

    在默认情况下Observable的所有代码，都会在执行subscribe()方法的线程中运行。而通过subscribeOn()方法，这些代码可以在其他线程中执行。在上面的例子中，我们让代码在"IO Scheduler"中执行（Schedulers.io()）。现在我们可以只把Scheduler当做一个可以工作的子线程。

  - 2).observeOn() 会指定onNext()方法发生的线程

    通过在observeOn()方法中指定另一个Scheduler来完成onNext()的内容，在这里也就是AndroidSchedules.mainThread()所返回的Scheduler(UI线程的Scheduler)。

  - 3).subscribe() Callable只会在有在Observable调用subscribe()后执行。

    Observable.subscribe(Subscriber) 的内部实现是这样的（仅核心代码）：

~~~ Java  
// 注意：这不是 subscribe() 的源码，而是将源码中与性能、兼容性、扩展性有关的代码剔除后的核心代码。
// 如果需要看源码，可以去 RxJava 的 GitHub 仓库下载。
public Subscription subscribe(Subscriber subscriber) {
    subscriber.onStart();
    onSubscribe.call(subscriber);
    return subscriber;
}
~~~
  可以看到，subscriber()做了3件事：

>   ①.调用Subscriber.onStart()。这个方法在前面已经介绍过，是一个可选的准备方法。
   ②.调用Observable中的OnSubscribe.call(Subscriber)。在这里，事件发送的逻辑开始运行。从这也可以看出，在RxJava中， Observable并不是在创建的时候就立即开始发送事件，而是在它被订阅的时候，即当 subscribe()方法执行的时候。
   ③.将传入的Subscriber作为Subscription返回。这是为了方便unsubscribe().

  - 4).mTvShowSubscription

    每当Observer订阅Observable时就会生成一个Subscription对象。一个Subscription代表了一个Observer与Observable之间的连接。有时我们需要操作这个连接，这里拿在Activity的onDestroy()方法中的代码举个例子：

~~~ Java  
if (mTvShowSubscription != null && !mTvShowSubscription.isUnsubscribed()) {
      mTvShowSubscription.unsubscribe(); //取消订阅
}
~~~
   unsubscribe()方法告诉Observable它所发送的数据不再被Observer所接收。在调用unsubscribe()方法后，我们创建的Observer就不再会收到数据了，以免Observable异步加载数据时发生意外。

#### **3.使用Single**

Single是Observable的精简版，一种特殊的只发射单个值的Observable，几乎和Observable一模一样，但其回调方法不是onComplete()/onNext()/onError()，而是onSuccess()/onError()。

  我们现在把刚才写过的Observable用Single重写一遍。首先我们要创建一个Single:

~~~ Java
Single<List<String>> tvShowSingle = Single.fromCallable(new Callable<List<String>>() {
    @Override
    public List<String> call() throws Exception {
        mRestClient.getFavoriteTvShows();
    }
});
~~~

  然后订阅一下

~~~ Java
mTvShowSubscription = tvShowSingle
    .subscribeOn(Schedulers.io())
    .observeOn(AndroidSchedulers.mainThread())
    .subscribe(new SingleSubscriber<List<String>>() {

        @Override
        public void onSuccess(List<String> tvShows) {
            displayTvShows(tvShows);
        }

        @Override
        public void onError(Throwable error) {
            displayErrorMessage();
        }
});
~~~


但这次我们不再使用Observer，而是使用一个叫SingleSubscriber的类。这个类和Observer非常像，只不过它只有上述两个方法：onSuccess()和onError()。SingleSubscriber之于Single就如Observer之于Observable。

订阅一个Single的同时也会自动创建一个Subscription对象。这里的Subscription和上面的例子没有区别，一定要在onDestroy()中解除订阅。

### 5.线程控制——Scheduler

  RxJava已经内置了几个 Scheduler ，它们已经适合大多数的使用场景：

> - 1.Schedulers.immediate(): 直接在当前线程运行，相当于不指定线程。这是默认的 Scheduler。
- 2.Schedulers.newThread(): 总是启用新线程，并在新线程执行操作。
- 3.Schedulers.io(): I/O 操作（读写文件、读写数据库、网络信息交互等）所使用的 Scheduler。行为模式和 newThread() 差不多，区别在于 io() 的内部实现是是用一个无数量上限的线程池，可以重用空闲的线程，因此多数情况下 io() 比 newThread() 更有效率。不要把计算工作放在 io() 中，可以避免创建不必要的线程。
- 4.Schedulers.computation(): 计算所使用的 Scheduler。这个计算指的是 CPU 密集型计算，即不会被 I/O 等操作限制性能的操作，例如图形的计算。这个 Scheduler 使用的固定的线程池，大小为 CPU 核数。不要把 I/O 操作放在 computation() 中，否则 I/O 操作的等待时间会浪费 CPU。
- 5.另外， Android 还有一个专用的 AndroidSchedulers.mainThread()，它指定的操作将在 Android 主线程运行。

### 6.特殊的情况

1.Subject Observable和Observer的复合体，也是二者的桥梁

  Subjects = Observable + Observer，Subject继承自Observable实现了Observer

  Rxjava提供的四种Subject:

>    ①PublishSubject ： subject的基础子类。
    ②BehaviorSubject : 会首先向它的订阅者发送截止订阅前最新的一个数据，然后正常发送订阅后的数据流。
    ③ReplaySubject ： 会缓存它所订阅的所有数据，向所有订阅它的观察者重发。
    ④AsyncSubject ： 只会发布最后一个数据给已经订阅的每一个观察者。
