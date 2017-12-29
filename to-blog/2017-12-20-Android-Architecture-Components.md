---
layout: post
title: Android Architecture Components介绍
category: accumulation
tags:
    - Android Architecture Components
    - Python
keywords: Python
toc: true
---

## Android Architecture Components(AAC)
Android Architecture Components是谷歌官方推出的一个库集合，帮助开发者构建一个健壮、可测试、易维护的android APP

现在版本是`Now 1.0 stable`

### 架构原则
如果不能用应用组件去存储应用数据和状态，那应该怎样去设计应用的架构呢？

首先，通常的架构原则有几个重点：

第一个重点是在应用中的关注点分离。
    比如在一个 Activity 或者 Fragment 中写所有的代码这明显是错误的。任何与 UI 或者交互无关的代码都不应该存在这些类中。保证他们尽可能的职责单一化将会使我们避免很多生命周期相关的问题。Android 系统可能会随时由于用户的行为或者系统状态（比如剩余内存过低）而销毁你的应用组件。所以应该最小化应用组件之间的依赖以提供一个健壮的体验。

第二个重点是我们应该采用数据模型驱动 UI 的方式，最好是一个可持久化的模型。

持久化被建议的原因有两个：

- 用户不会因为系统销毁我们的应用而导致丢失数据。
- 我们的应用可以在网络状况不好甚至断网的情况下继续工作。

这里说的模型其实也是一种组件，他们就是专门负责为我们的应用处理和存储数据的。他们完全独立于 Views 和其他应用中的组件，所以他们不存在生命周期相关的问题。保证 UI 部分的代码足够简单，没有业务逻辑，使代码更容易去管理。

### AAC 介绍
AAC 的核心是: Lifecycle, LiveData, ViewModel 以及 Room, 通过它可以非常优雅的让数据与界面交互,并做一些持久化的东西,高度解耦,自动管理生命周期, 而且不用担心内存泄漏的问题.

![Android Architecture Components](/images/blogimages/2017/Architecture_Components.png)

下面就一一介绍它们

### Lifecycle
Lifecycle 主要用 State 和 Event 这两个枚举类来表达 Activity/Fragment 的生命周期和状态. LifecycleRegistry 是它的主要实现类,用于分发生命周期的变化.

- Event
    The lifecycle events that are dispatched from the framework and the Lifecycle class. These events map to the callback events in activities and fragments.
- State
    The current state of the component tracked by the Lifecycle object.

State 和 Event 之间的关系用下图表示:

![](/images/blogimages/2017/lifecycle-states.png)

Think of the states as nodes of a graph and events as the edges between these nodes.

### LiveData
LiveData 主要用于接受/更新数据, Lifecycle 的具体实现会将生命周期的变化传给 LiveData,LiveData 通过状态判断要不要刷新数据,假设要刷新数据,则通过 LifecycleObserver 将得到数据的变分发给对应的监听者(Activity/Fragment).

### ViewModel
用于管理数据,它持有 LiveData. 处理数据持久化,存取等具体逻辑, 相当于 MVP 中的 Presenter.

![](/images/blogimages/2017/viewmodel-lifecycle.png)

### Room
通过注解将数据持久化(数据库管理)

![](/images/blogimages/2017/room_architecture.png)



## 简单使用
### 配置
根目录gradle文件中添加Google Maven Repository

```
allprojects {
    repositories {
        jcenter()
        maven { url 'https://maven.google.com' }
    }
}
```


如使用Lifecycle, LiveData、ViewModel，添加如下依赖。

```
compile "android.arch.lifecycle:runtime:1.0.0-alpha1"
compile "android.arch.lifecycle:extensions:1.0.0-alpha1"
annotationProcessor "android.arch.lifecycle:compiler:1.0.0-alpha1"
```

如使用Room功能，添加如下依赖。

```
compile "android.arch.persistence.room:runtime:1.0.0-alpha1"
annotationProcessor "android.arch.persistence.room:compiler:1.0.0-alpha1"

// For testing Room migrations, add:
testCompile "android.arch.persistence.room:testing:1.0.0-alpha1"

// For Room RxJava support, add:
compile "android.arch.persistence.room:rxjava2:1.0.0-alpha1"

```

## AAC 的特点
- 数据驱动型编程:变化的永远是数据,界面无需更改.
    我们通常的编程类型是:业务驱动型,即通过编写业务逻辑代码来实现,代码通常放在 Activity, 使用 MVP 之后放在 Presenter.
    数据驱动型与业务驱动型最大的不同是：数据驱动型认为通常的变化大多是在数据层面上的变化,且它将数据的变化放在了数据层,将 UI的变化放在的 UI, 分离了变化,遵循"单一职责,一个类只有一个变化"的设计原则.而业务驱动型的编程是根据业务的变化,修改获取到的数据,即获取到的数据还需要再根据业务需求处理一遍.

- 感知生命周期,防止内存泄漏<br>原理与 Glide 一样,通过给 Activity 添加一个碎片来监听生命周期的变化

- 高度解耦<br>数据,界面高度分离, library 分离,可仅使用其中一个功能,例如不使用数据库功能

- 数据持久化<br>数据 ViewModel 不与 UI 的生命周期挂钩,不会因为界面的重建而销毁


### 参考

https://developer.android.google.cn/topic/libraries/architecture/guide.html

[基于Android Architecture Components的应用架构指南](http://cdc.tencent.com/2017/06/29/%E5%9F%BA%E4%BA%8Eandroid-architecture-components%E7%9A%84%E5%BA%94%E7%94%A8%E6%9E%B6%E6%9E%84%E6%8C%87%E5%8D%97/)

[浅谈 Android Architecture Components 使用和原理](https://github.com/siyehua/Android-Architecture-Components-Demo)