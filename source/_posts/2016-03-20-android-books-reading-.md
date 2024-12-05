---
layout: post
title: 《android开发艺术探索》读书笔记
category: accumulation
tags:
  - reading
  - ANDROID
keywords: 《android开发艺术探索》,读书笔记
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Auvers%20Town%20Hall%20on%2014%20July%201890.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Auvers%20Town%20Hall%20on%2014%20July%201890.jpg
---


### 第1章 Activity的生命周期和启动模式

本节和《Android群英传》中的第8章Activity和Activity调用栈分析有关系，建议先阅读该章的总结

第1章 Activity的生命周期和启动模式

#### 1.1 Activity生命周期全面分析

##### 1.1.1 典型情况下生命周期分析

- (1)一般情况下，当当前Activity从不可见重新变为可见状态时，onRestart方法就会被调用。

- (2)当用户打开新的Activity或者切换到桌面的时候，回调如下：onPause -> onStop，但是如果新Activity采用了透明主题，那么onStop方法不会被回调。当用户再次回到原来的Activity时，回调如下：onRestart -> onStart -> onResume。

<!--more-->

- (3)onStart和onStop对应，它们是从Activity是否可见这个角度来回调的；onPause和onResume方法对应，它们是从Activity是否位于前台这个角度来回调的。

- (4)从Activity A进入到Activity B，回调顺序是onPause(A) -> onCreate(B) -> onStart(B) -> onResume(B) -> onStop(A)，所以不能在onPause方法中做重量级的操作。

1.1.2 异常情况下生命周期分析

- (1)onSaveInstanceState方法只会出现在Activity被异常终止的情况下，它的调用时机是在onStop之前，它和onPause方法没有既定的时序关系，可能在它之前，也可能在它之后。

    当Activity被重新创建的时候，onRestoreInstanceState会被回调，它的调用时机是onStart之后。

    系统只会在Activity即将被销毁并且有机会重新显示的情况下才会去调用onSaveInstanceState方法。

    当Activity在异常情况下需要重新创建时，系统会默认为我们保存当前Activity的视图结构，并且在Activity重启后为我们恢复这些数据，比如文本框中用户输入的数据、listview滚动的位置等，这些view相关的状态系统都会默认为我们恢复。具体针对某一个view系统能为我们恢复哪些数据可以查看view的源码中的onSaveInstanceState和onRestoreInstanceState方法。

- (2)Activity按优先级的分类

    前台Activity；可见但非前台Activity；后台Activity

- (3)android:configChanges="xxx"属性，常用的主要有下面三个选项：

    local：设备的本地位置发生了变化，一般指切换了系统语言；

    keyboardHidden：键盘的可访问性发生了变化，比如用户调出了键盘；

    orientation：屏幕方向发生了变化，比如旋转了手机屏幕。

    配置了android:configChanges="xxx"属性之后，Activity就不会在对应变化发生时重新创建，而是调用Activity的onConfigurationChanged方法。

#### 1.2 Activity的启动模式

##### 1.2.1 启动模式

- (1)当任务栈中没有任何Activity的时候，系统就会回收这个任务栈。

- (2)从非Activity类型的Context(例如ApplicationContext、Service等)中以standard模式启动新的Activity是不行的，因为这类context并没有任务栈，所以需要为待启动Activity指定FLAG_ACTIVITY_NEW_TASK标志位。

- (3)任务栈分为前台任务栈和后台任务栈，后台任务栈中的Activity位于暂停状态，用户可以通过切换将后台任务栈再次调到前台。

- (4)参数TaskAffinity用来指定Activity所需要的任务栈，意为任务相关性。

    默认情况下，所有Activity所需的任务栈的名字为应用的包名。TaskAffinity属性主要和singleTask启动模式或者allowTaskReparenting属性配对使用，在其他情况下没有意义。

    当TaskAffinity和singleTask启动模式配对使用的时候，它是具有该模式的Activity的目前任务栈的名字，待启动的Activity会运行在名字和TaskAffinity相同的任务栈中；

    当TaskAffinity和allowTaskReparenting结合的时候，当一个应用A启动了应用B的某个Activity C后，如果Activity C的allowTaskReparenting属性设置为true的话，那么当应用B被启动后，系统会发现Activity C所需的任务栈存在了，就将Activity C从A的任务栈中转移到B的任务栈中。

- (5)singleTask模式的具体分析：

    当一个具有singleTask启动模式的Activity请求启动之后，系统首先会寻找是否存在A想要的任务栈，

    如果不存在，就重新创建一个任务栈，然后创建Activity的实例把它放到栈中；如果存在Activity所需的任务栈，这时候要看栈中是否有Activity实例存在.

    如果有，那么系统就会把该Activity实例调到栈顶，并调用它的onNewIntent方法(它之上的Activity会被迫出栈，所以singleTask模式具有FLAG_ACTIVITY_CLEAR_TOP效果)；如果Activity实例不存在，那么就创建Activity实例并把它压入栈中。

- (6)设置启动模式既可以使用xml属性android:launchMode，也可以使用代码intent.addFlags()。区别在于限定范围不同，前者无法直接为Activity设置FLAG_ACTIVITY_CLEAR_TOP标识，而后者无法为Activity指定singleInstance模式。

##### 1.2.2 Activity的Flags

FLAG_ACTIVITY_NEW_TASK,FLAG_ACTIVITY_SINGLE_TOP,FLAG_ACTIVITY_CLEAR_TOP

FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS：具有这个标记的Activity不会出现在历史Activity列表中，当某些情况下我们不希望用户通过历史列表回到我们的Activity的时候这个标记比较有用，它等同于属性设置android:excludeFromRecents="true"。

#### 1.3 IntentFilter的匹配规则

- (1)IntentFilter中的过滤信息有action、category、data，为了匹配过滤列表，需要同时匹配过滤列表中的action、category、data信息，否则匹配失败。

    一个过滤列表中的action、category、data可以有多个，所有的action、category、data分别构成不同类别，同一类别的信息共同约束当前类别的匹配过程。只有一个Intent同时匹配action类别、category类别和data类别才算完全匹配，只有完全匹配才能成功启动目标Activity。此外，一个Activity中可以有多个intent-filter，一个Intent只要能匹配任何一组intenf-filter即可成功启动对应的Activity。
~~~ Javascript
<intent-filter>
    <action android:name="com.ryg.charpter_1.c" />
    <action android:name="com.ryg.charpter_1.d" />

    <category android:name="com.ryg.category.c" />
    <category android:name="com.ryg.category.d" />
    <category android:name="android.intent.category.DEFAULT" />

    <data android:mimeType="text/plain" />
</intent-filter>
~~~  

- (2)action匹配规则

    只要Intent中的action能够和过滤规则中的任何一个action相同即可匹配成功，action匹配区分大小写。

- (3)category匹配规则

    Intent中如果有category那么所有的category都必须和过滤规则中的其中一个category相同，如果没有category的话那么就是默认的category，即android.intent.category.DEFAULT，所以为了Activity能够接收隐式调用，配置多个category的时候必须加上默认的category。

- (4)data匹配规则

    data的结构很复杂，语法大致如下：
~~~ Javascript
<data android:scheme="string"
	android:host="string"
	android:port="string"
	android:path="string"`
	android:pathPattern="string"
	android:pathPrefix="string"
	android:mimeType="string" />
~~~
    主要由mimeType和URI组成，其中mimeType代表媒体类型，而URI的结构也复杂，大致如下：
~~~ Javascript
<scheme>://<host>:<port>/[<path>]|[<pathPrefix>]|[pathPattern]
~~~
    例如content://com.example.project:200/folder/subfolder/etc

  - scheme、host、port分别表示URI的模式、主机名和端口号，其中如果scheme或者host未指定那么URI就无效。

  - path、pathPattern、pathPrefix都是表示路径信息，其中path表示完整的路径信息，pathPrefix表示路径的前缀信息；pathPattern表示完整的路径，但是它里面包含了通配符(\*)。

  data匹配规则：Intent中必须含有data数据，并且data数据能够完全匹配过滤规则中的某一个data。

  URI有默认的scheme！

  如果过滤规则中的mimeType指定为image/\*或者text/\*等这种类型的话，那么即使过滤规则中没有指定URI，URI有默认的scheme是content和file！如果过滤规则中指定了scheme的话那就不是默认的scheme了。

~~~ Java
//URI有默认值
<intent-filter>
    <data android:mimeType="image/*"/>
  ...
</intent-filter>
~~~

~~~ Java
//URI默认值被覆盖
<intent-filter>
    <data android:mimeType="image/*" android:scheme="http" .../>
    ...
</intent-filter>
~~~

  如果要为Intent指定完整的data，必须要调用setDataAndType方法！

  不能先调用setData然后调用setType，因为这两个方法会彼此清除对方的值。
~~~ Java
intent.setDataAndType(Uri.parse("file://abc"), "image/png");
~~~
  data的下面两种写法作用是一样的：

~~~ Java
<intent-filter>
    <data android:scheme="file" android:host="www.github.com"/>
</intent-filter>

<intent-filter>
    <data android:scheme="file"/>
    <data android:host="www.github.com"/>
</intent-filter>
~~~
  如何判断是否有Activity能够匹配我们的隐式Intent？

  - (1)PackageManager的resolveActivity方法或者Intent的resolveActivity方法：如果找不到就会返回null

  - (2)PackageManager的queryIntentActivities方法：它返回所有成功匹配的Activity信息
针对Service和BroadcastReceiver等组件，PackageManager同样提供了类似的方法去获取成功匹配的组件信息，例如queryIntentServices、queryBroadcastReceivers等方法

  - 有一类action和category比较重要，它们在一起用来标明这是一个入口Activity，并且会出现在系统的应用列表中。
~~~ Java
<intent-filter>
    <action android:name="android.intent.action.MAIN" />
    <category android:name="android.intent.category.LAUNCHER" />
</intent-filter>
~~~

### 第2章 IPC机制

#### 2.1 Android IPC简介

(1)任何一个操作系统都需要有相应的IPC机制，Linux上可以通过命名通道、共享内存、信号量等来进行进程间通信。Android系统不仅可以使用了Binder机制来实现IPC，还可以使用Socket实现任意两个终端之间的通信。

#### 2.2 Android中的多进程模式

- (1)通过给四大组件指定android:process属性就可以开启多进程模式.

    默认进程的进程名是包名packageName，进程名以:开头的进程属于当前应用的私有进程，其他应用的组件不可以和它跑在同一个进程中，而进程名不以:开头的进程属于全局进程，其他应用通过ShareUID方法可以和它跑在同一个进程中。

    android:process=":xyz" //进程名是 packageName:xyz

    android:process="aaa.bbb.ccc" //进程名是 aaa.bbb.ccc

- (2)Android系统会为每个应用分配一个唯一的UID，具有相同UID的应用才能共享数据。

    两个应用通过ShareUID跑在同一个进程中是有要求的，需要这两个应用有相同的ShareUID并且签名相同才可以。 在这种情况下，它们可以相互访问对方的私有数据，比如data目录、组件信息等，不管它们是否跑在同一个进程中。如果它们跑在同一个进程中，还可以共享内存数据，它们看起来就像是一个应用的两个部分。
- (3)android系统会为每个进程分配一个独立的虚拟机，不同的虚拟机在内存分配上有不同的地址空间，所以不同的虚拟机中访问同一个类的对象会产生多个副本。

- (4)使用多进程容易造成以下几个问题：

  - 1.静态成员和单例模式完全失效；
  - 2.线程同步机制完全失效：无论锁对象还是锁全局对象都无法保证线程同步；
  - 3.SharedPreferences的可靠性下降：SharedPreferences不支持并发读写；
  - 4.Application会多次创建：当一个组件跑在一个新的进程的时候，系统要在创建新的进程的同时分配独立的虚拟机，应用会重新启动一次，也就会创建新的Application。运行在同一个进程中的组件是属于同一个虚拟机和同一个Application。

    同一个应用的不同组件，如果它们运行在不同进程中，那么和它们分别属于两个应用没有本质区别。

#### 2.3 IPC基础概念介绍

- (1)Serializable接口是Java中为对象提供标准的序列化和反序列化操作的接口，而Parcelable接口是Android提供的序列化方式的接口。

- (2)serialVersionUId是一串long型数字，主要是用来辅助序列化和反序列化的，原则上序列化后的数据中的serialVersionUId只有和当前类的serialVersionUId相同才能够正常地被反序列化。

  serialVersionUId的详细工作机制：序列化的时候系统会把当前类的serialVersionUId写入序列化的文件中，当反序列化的时候系统会去检测文件中的serialVersionUId，看它是否和当前类的serialVersionUId一致，如果一致就说明序列化的类的版本和当前类的版本是相同的，这个时候可以成功反序列化；否则说明版本不一致无法正常反序列化。一般来说，我们应该手动指定serialVersionUId的值。

  - 1.静态成员变量属于类不属于对象，所以不参与序列化过程；

  - 2.声明为transient的成员变量不参与序列化过程。

- (3)Parcelable接口内部包装了可序列化的数据，可以在Binder中自由传输，Parcelable主要用在内存序列化上，可以直接序列化的有Intent、Bundle、Bitmap以及List和Map等等，下面是一个实现了Parcelable接口的示例

~~~ Java
public class Book implements Parcelable {
    public int bookId;
    public String bookName;
    public Book() {
    }

    public Book(int bookId, String bookName) {
        this.bookId = bookId;
        this.bookName = bookName;
    }

    //“内容描述”，如果含有文件描述符返回1，否则返回0，几乎所有情况下都是返回0
    public int describeContents() {
        return 0;
    }

    //实现序列化操作，flags标识只有0和1，1表示标识当前对象需要作为返回值返回，不能立即释放资源，几乎所有情况都为0
    public void writeToParcel(Parcel out, int flags) {
        out.writeInt(bookId);
        out.writeString(bookName);
    }

    //实现反序列化操作
    public static final Parcelable.Creator<Book> CREATOR = new Parcelable.Creator<Book>() {
        //从序列化后的对象中创建原始对象
        public Book createFromParcel(Parcel in) {
            return new Book(in);
        }
        public Book[] newArray(int size) {//创建指定长度的原始对象数组
            return new Book[size];
        }
    };

    private Book(Parcel in) {
        bookId = in.readInt();
        bookName = in.readString();
    }

}
~~~

- (4)Binder是Android中的一个类，它实现了IBinder接口。

  从IPC角度看，Binder是Android中一种跨进程通信的方式；Binder还可以理解为虚拟的物理设备，它的设备驱动是/dev/binder；

  从Framework层角度看，Binder是ServiceManager连接各种Manager和相应的ManagerService的桥梁；

  从Android应用层来说，Binder是客户端和服务端进行通信的媒介，当bindService的时候，服务端会返回一个包含了服务端业务调用的Binder对象，通过这个Binder对象，客户端就可以获取服务端提供的服务或者数据，这里的服务包括普通服务和基于AIDL的服务。


  在Android开发中，Binder主要用在Service中，包括AIDL和Messenger，其中普通Service中的Binder不涉及进程间通信，较为简单；而Messenger的底层其实是AIDL，正是Binder的核心工作机制。

- (5)aidl工具根据aidl文件自动生成的java接口的解析：

  首先，它声明了几个接口方法，同时还声明了几个整型的id用于标识这些方法，id用于标识在transact过程中客户端所请求的到底是哪个方法；

  接着，它声明了一个内部类Stub，这个Stub就是一个Binder类，当客户端和服务端都位于同一个进程时，方法调用不会走跨进程的transact过程，而当两者位于不同进程时，方法调用需要走transact过程，这个逻辑由Stub内部的代理类Proxy来完成。

  所以，这个接口的核心就是它的内部类Stub和Stub内部的代理类Proxy。 下面分析其中的方法：

  - 1.asInterface(android.os.IBinder obj)：用于将服务端的Binder对象转换成客户端所需的AIDL接口类型的对象，这种转换过程是区分进程的，如果客户端和服务端是在同一个进程中，那么这个方法返回的是服务端的Stub对象本身，否则返回的是系统封装的Stub.Proxy对象。
  - 2.asBinder：返回当前Binder对象。
  - 3.onTransact：这个方法运行在服务端中的Binder线程池中，当客户端发起跨进程请求时，远程请求会通过系统底层封装后交由此方法来处理。

    这个方法的原型是public Boolean onTransact(int code, Parcelable data, Parcelable reply, int flags)

    服务端通过code可以知道客户端请求的目标方法，接着从data中取出所需的参数，然后执行目标方法，执行完毕之后，将结果写入到reply中。如果此方法返回false，说明客户端的请求失败，利用这个特性可以做权限验证(即验证是否有权限调用该服务)。

  - 4.Proxy#[Method]：代理类中的接口方法，这些方法运行在客户端，当客户端远程调用此方法时，它的内部实现是：

    首先创建该方法所需要的参数，然后把方法的参数信息写入到_data中，接着调用transact方法来发起RPC请求，同时当前线程挂起；然后服务端的onTransact方法会被调用，直到RPC过程返回后，当前线程继续执行，并从_reply中取出RPC过程的返回结果，最后返回_reply中的数据。

  如果搞清楚了自动生成的接口文件的结构和作用之后，其实是可以不用通过AIDL而直接实现Binder的，[主席写的示例代码](https://github.com/singwhatiwanna/android-art-res/blob/master/Chapter_2/src/com/ryg/chapter_2/manualbinder/BookManagerImpl.java)

- (6)Binder的两个重要方法linkToDeath和unlinkToDeath

  Binder运行在服务端，如果由于某种原因服务端异常终止了的话会导致客户端的远程调用失败，所以Binder提供了两个配对的方法linkToDeath和unlinkToDeath，通过linkToDeath方法可以给Binder设置一个死亡代理，当Binder死亡的时候客户端就会收到通知，然后就可以重新发起连接请求从而恢复连接了。

  如何给Binder设置死亡代理呢？

  - 1.声明一个DeathRecipient对象，DeathRecipient是一个接口，其内部只有一个方法bindeDied，实现这个方法就可以在Binder死亡的时候收到通知了。

~~~ Java
private IBinder.DeathRecipient mDeathRecipient = new IBinder.DeathRecipient() {
    @Override
    public void binderDied() {
        if (mRemoteBookManager == null) return;
        mRemoteBookManager.asBinder().unlinkToDeath(mDeathRecipient, 0);
        mRemoteBookManager = null;
        // TODO:这里重新绑定远程Service
    }
};
~~~
  - 2.在客户端绑定远程服务成功之后，给binder设置死亡代理

~~~ Java
mRemoteBookManager.asBinder().linkToDeath(mDeathRecipient, 0);
~~~

#### 2.4 Android中的IPC方式

- (1)使用Bundle：Bundle实现了Parcelable接口，Activity、Service和Receiver都支持在Intent中传递Bundle数据。

- (2)使用文件共享：这种方式简单，适合在对数据同步要求不高的进程之间进行通信，并且要妥善处理并发读写的问题。

  SharedPreferences是一个特例，虽然它也是文件的一种，但是由于系统对它的读写有一定的缓存策略，即在内存中会有一份SharedPreferences文件的缓存，因此在多进程模式下，系统对它的读写就变得不可靠，当面对高并发读写访问的时候，有很大几率会丢失数据，因此，不建议在进程间通信中使用SharedPreferences。

- (3)使用Messenger：Messenger是一种轻量级的IPC方案，它的底层实现就是AIDL。Messenger是以串行的方式处理请求的，即服务端只能一个个处理，不存在并发执行的情形，详细的示例见原书。

- (4)使用AIDL

  大致流程：首先建一个Service和一个AIDL接口，接着创建一个类继承自AIDL接口中的Stub类并实现Stub类中的抽象方法，在Service的onBind方法中返回这个类的对象，然后客户端就可以绑定服务端Service，建立连接后就可以访问远程服务端的方法了。

  - 1.AIDL支持的数据类型：基本数据类型、String和CharSequence、ArrayList、HashMap、Parcelable以及AIDL；
  - 2.某些类即使和AIDL文件在同一个包中也要显式import进来；
  - 3.AIDL中除了基本数据类，其他类型的参数都要标上方向：in、out或者inout；
  - 4.AIDL接口中支持方法，不支持声明静态变量；
  - 5.为了方便AIDL的开发，建议把所有和AIDL相关的类和文件全部放入同一个包中，这样做的好处是，当客户端是另一个应用的时候，可以直接把整个包复制到客户端工程中。
  - 6.RemoteCallbackList是系统专门提供的用于删除跨进程Listener的接口。RemoteCallbackList是一个泛型，支持管理任意的AIDL接口，因为所有的AIDL接口都继承自IInterface接口。

- (5)使用ContentProvider

  - 1.ContentProvider主要以表格的形式来组织数据，并且可以包含多个表；
  - 2.ContentProvider还支持文件数据，比如图片、视频等，系统提供的MediaStore就是文件类型的ContentProvider；
  - 3.ContentProvider对底层的数据存储方式没有任何要求，可以是SQLite、文件，甚至是内存中的一个对象都行；
  - 4.要观察ContentProvider中的数据变化情况，可以通过ContentResolver的registerContentObserver方法来注册观察者；

- (6)使用Socket

  Socket是网络通信中“套接字”的概念，分为流式套接字和用户数据包套接字两种，分别对应网络的传输控制层的TCP和UDP协议。

#### 2.5 Binder连接池

- (1)当项目规模很大的时候，创建很多个Service是不对的做法，因为service是系统资源，太多的service会使得应用看起来很重，所以最好是将所有的AIDL放在同一个Service中去管理。

  整个工作机制是：每个业务模块创建自己的AIDL接口并实现此接口，这个时候不同业务模块之间是不能有耦合的，所有实现细节我们要单独开来，然后向服务端提供自己的唯一标识和其对应的Binder对象；对于服务端来说，只需要一个Service，服务端提供一个queryBinder接口，这个接口能够根据业务模块的特征来返回相应的Binder对象给它们，不同的业务模块拿到所需的Binder对象后就可以进行远程方法调用了。

  Binder连接池的主要作用就是将每个业务模块的Binder请求统一转发到远程Service去执行，从而避免了重复创建Service的过程。

- (2)作者实现的Binder连接池BinderPool的实现源码，建议在AIDL开发工作中引入BinderPool机制。

#### 2.6 选用合适的IPC方式

![选择合适的IPC方式](https://raw.githubusercontent.com/agehua/blog-imags/img/lib-hexo-blog-img/blogimages/2016//androidart_ipc.png)



### 第9章 四大组件的工作过程
本篇摘选自[amurocrash的专栏](http://blog.csdn.net/amurocrash/article/details/48858353)

#### 9.1 四大组件的运行状态

- (1)四大组件中只有BroadcastReceiver既可以在AndroidManifest文件中注册，也可以在代码中注册，其他三个组件都必须在AndroidManifest文件中注册；ContentProvider的调用不需要借助Intent，其他三个组件都需要借助Intent。
- (2)Activity是一种展示型组件，用于向用户展示界面，可由显式或者隐式Intent来启动。
- (3)Service是一种计算型组件，用于在后台执行计算任务。尽管service是用于后台执行计算的，但是它本身是运行在主线程中的，因此耗时的后台计算仍然需要在单独的线程中去完成。Service组件有两种状态：启动状态和绑定状态。当service处于绑定状态时，外界可以很方便的和service进行通信，而在启动状态中是不可与外界通信的。
- (4)BroadcastReceiver是一种消息型组件，用于在不同的组件乃至不同的应用之间传递消息，它工作在系统内部。广播有两种注册方式：静态注册和动态注册。静态注册是在AndroidManifest中注册，在应用安装的时候会被系统解析，这种广播不需要应用启动就可以收到相应的广播。动态注册需要通过Context.registerReceiver()来注册，这种广播需要应用启动才能注册并接收广播。BroadcastReceiver组件一般来说不需要停止，它也没有停止的概念。
- (5)ContentProvider是一种数据共享型组件，用于向其他组件乃至其他应用共享数据。ContentProvider中的insert、delete、update、query方法需要处理好线程同步，因为这几个方法是在Binder线程池中被调用的，另外ContentProvider组件也不需要手动停止。

#### 9.2 Activity的工作过程

##### (1)Activity启动的大致流程
![Activity启动的大致流程](https://raw.githubusercontent.com/agehua/blog-imags/img/lib-hexo-blog-img/blogimages/2016//androidart_activity.png)

(2)ApplicationThread是ActivityThread的一个内部类，它继承自ApplicationThreadNative，而ApplicationThreadNative继承自Binder并实现了IApplicationThread接口，ApplicationThreadNative的作用其实就和系统为AIDL文件生成的类是一样的。
(3)ActivityManagerService(AMS)继承自ActivityManagerNative，而ActivityManagerNative继承自Binder并实现了IActivityManager这个Binder接口，因此AMS也是一个Binder。
(4)一个应用只有一个Application对象，它的创建也是通过Instrumentation来完成的，这个过程和Activity对象的创建过程一样，都是通过类加载器来实现的。
(5)ContextImpl是Context的具体实现，ContextImpl是通过Activity的attach方法来和Activity建立关联的，在attach方法中Activity还会完成Window的创建并建立自己和Window的关联，这样当window接收到外部输入事件后就可以将事件传递给Activity。 [这里可能有误，应该是Activity将事件传递给window]

#### 9.3 Service的工作过程

##### (1)Service有两种状态：
启动状态和绑定状态，两种状态是可以共存的。
**启动过程：**
![Service1](https://raw.githubusercontent.com/agehua/blog-imags/img/lib-hexo-blog-img/blogimages/2016//androidart_service1.png)

**绑定过程：**
![Service2](https://raw.githubusercontent.com/agehua/blog-imags/img/lib-hexo-blog-img/blogimages/2016//androidart_service2.png)

#### 9.4 BroadcastReceiver的工作过程
##### (1)BroadcastReceiver的工作过程包括广播注册过程、广播发送和接收过程。

**注册过程**：静态注册的时候是由PackageManagerService来完成整个注册过程，下面是动态注册的过程
![broadcastreceiver1](https://raw.githubusercontent.com/agehua/blog-imags/img/lib-hexo-blog-img/blogimages/2016//androidart_broadcastreceiver1.png)
**发送和接收**：
![broadcastreceiver2](https://raw.githubusercontent.com/agehua/blog-imags/img/lib-hexo-blog-img/blogimages/2016//androidart_broadcastreceiver2.png)
##### (2)广播的发送有几种类型：
普通广播、有序广播和粘性广播，有序广播和粘性广播与普通广播相比具有不同的特性，但是发送和接收过程是类似的。
##### (3)一个应用处于停止状态分为两种情况：
一是应用安装后未运行；二是应用被手动或者其他应用强停了。从Android 3.1开始，处于停止状态的应用无法接受到开机广播。

#### 9.5 ContentProvider的工作过程

##### (1)当ContentProvider所在的进程启动的时候，它会同时被启动并被发布到AMS中，这个时候它的onCreate要先去Application的onCreate执行。
##### (2)ContentProvider的启动过程：
- 1.当一个应用启动时，入口方法是ActivityThread的main方法，其中创建ActivityThread的实例并创建主线程的消息队列；
- 2.ActivityThread的attach方法中会远程调用ActivityManagerService的attachApplication，并将ApplicationThread提供给AMS，ApplicationThread主要用于ActivityThread和AMS之间的通信；
- 3.ActivityManagerService的attachApplication会调用ApplicationThread的bindApplication方法，这个方法会通过H切换到ActivityThread中去执行，即调用handleBindApplication方法；
- 4.handleBindApplication方法会创建Application对象并加载ContentProvider，注意是先加载ContentProvider，然后调用Application的onCreate方法。

##### (3)ContentProvider的android:multiprocess属性决定它是否是单实例，默认值是false，也就是默认是单实例。当设置为true时，每个调用者的进程中都存在一个ContentProvider对象。

##### (4)当调用ContentProvider的insert、delete、update、query方法中的任何一个时，如果ContentProvider所在的进程没有启动的话，那么就会触发ContentProvider的创建，并伴随着ContentProvider所在进程的启动。下图是ContentProvider的query操作的大致过程：
![ContentProvider](https://raw.githubusercontent.com/agehua/blog-imags/img/lib-hexo-blog-img/blogimages/2016//androidart_contentprovider.png)
