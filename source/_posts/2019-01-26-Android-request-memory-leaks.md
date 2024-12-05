---
layout: post
title: Android避免网络请求造成内存泄漏
category: accumulation
tags:
    - Memory leaks
    - net request
keywords: Memory leaks, net request
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Evening%20The%20End%20of%20the%20Day%20after%20Millet.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Evening%20The%20End%20of%20the%20Day%20after%20Millet.jpg
toc: true
---

现在的网络请求框架很多也很完善，但是使用中不注意还是会有内存泄漏的现象。为了避免内存泄漏，大概有两种方式：

- 1.在Activity结束时取消请求
- 2.在异步回调的时候，通过Activity.isFinishing()方法判断Activity是否已经被销毁

但是最近在工作中，发现一个问题：同一个请求，会在多个页面被调用到，这就导致相同的代码包括发送请求，成功或失败回调，数据解析等会出现好几处。
如果我想把这个网络请求单独抽离出来，放到一个类里，不依赖具体的Activity，现在的网络框架回调一般都会运行在主线程，如果未及时释放资源，很容易造成内存泄漏。

<!--more-->

针对这种方式，可以采用EventBus，来发送包含解析后的数据到对应的Activity，在Activity onDestroy里取消EventBus监听。但是如果使用postSticky发送事件，会导致事件接收混乱。

为了真的实现这种需求，参考网上的一篇文章：[如何在Activity/Fragment结束时处理异步回调？](https://iluhcm.com/2017/03/05/handle-asynchronous-callbacks-when-activity-finishes/)

### 基于Lifeful接口的异步回调框架
#### Lifeful接口设计
我们定义Lifeful，一个不依赖于Context、也不依赖于PATH的接口。

~~~ Java
/**
 * 判断生命周期是否已经结束的一个接口。
 */
public interface Lifeful {
    /**
     * 判断某一个组件生命周期是否已经走到最后。一般用于异步回调时判断Activity或Fragment生命周期是否已经结束。
     *
     * @return
     */
    boolean isAlive();
}
~~~
实际上，我们只需要让具有生命周期的类(一般是Activity或Fragment)实现这个接口，然后再通过这个接口来判断这个实现类是否还存在，就可以与Context解耦了。

接下来定义一个接口生成器，通过弱引用包装Lifeful接口的实现类，并返回所需要的相关信息。
~~~ Java
/**
 * 生命周期具体对象生成器。
 */
public interface LifefulGenerator<Callback> {

    Callback getCallback();
    WeakReference<Lifeful> getLifefulWeakReference();
    boolean isLifefulNull();
}
~~~
提供一个该接口的默认实现：
~~~ Java
/**
 * 默认生命周期管理包装生成器。
 */
public class DefaultLifefulGenerator<Callback> implements LifefulGenerator<Callback> {
    private WeakReference<Lifeful> mLifefulWeakReference;
    private boolean mLifefulIsNull;
    private Callback mCallback;
    public DefaultLifefulGenerator(Callback callback, Lifeful lifeful) {
        mCallback = callback;
        mLifefulWeakReference = new WeakReference<>(lifeful);
        mLifefulIsNull = lifeful == null;
    }
    @Override
    public Callback getCallback() {
        return mCallback;
    }
    public WeakReference<Lifeful> getLifefulWeakReference() {
        return mLifefulWeakReference;
    }
    @Override
    public boolean isLifefulNull() {
        return mLifefulIsNull;
    }
}
~~~
接着通过一个静态方法判断是否对象的生命周期：
~~~ Java
/**
 * 生命周期相关帮助类。
 */
public class LifefulUtils {
    private static final String TAG = LifefulUtils.class.getSimpleName();
    public static <T> boolean shouldGoHome(LifefulGenerator<T> lifefulGenerator, boolean objectIsNull) {
        WeakReference<Lifeful> lifefulWeakReference = lifefulGenerator.getLifefulWeakReference();
        if (lifefulWeakReference == null) {
            Log.e(TAG, "Go home, lifefulWeakReference == null");
            return true;
        }
        Lifeful lifeful = lifefulWeakReference.get();
        if (null == lifeful && !objectIsNull) {
            return true;
        }

        if (null != lifeful && !lifeful.isAlive()) {
            lifefulGenerator.setCallback(null); // 取消持有的callback
            return true;
        }
        return false;
    }
    public static <T> boolean shouldGoHome(LifefulGenerator<T> lifefulGenerator) {
        if (null == lifefulGenerator) {
            Log.e(TAG, "Go home, null == lifefulGenerator");
            return true;
        } if (null == lifefulGenerator.getCallback()) {
            Log.e(TAG, "Go home, null == lifefulGenerator.getCallback()");
            return true;
        }
        return shouldGoHome(lifefulGenerator, lifefulGenerator.isLifefulNull());
    }
}
~~~
#### 具有生命周期的Runnable
具体到跟线程打交道的异步类，只有Runnable(Thread也是其子类)，因此只需要处理Runnable就可以了。我们可以通过Wrapper包装器模式，在处理真正的Runnable类之前，先通过Lifeful接口判断对象是否还存在，如果不存在则直接返回。对于Runnable：
~~~ Java
/**
 * 与周期相关的异步线程回调类。
 */
public class LifefulRunnable implements Runnable {
    private LifefulGenerator<Runnable> mLifefulGenerator;
    public LifefulRunnable(Runnable runnable, Lifeful lifeful) {
        mLifefulGenerator = new DefaultLifefulGenerator<>(runnable, lifeful);
    }
    @Override
    public void run() {
        if (LifefulUtils.shouldGoHome(mLifefulGenerator)) {
            return;
        }
        mLifefulGenerator.getCallback().run();
    }
}
~~~
#### Lifeful的实现类
最后说一下Lifeful类的实现类，主要包括Activity和Fragment，
~~~ Java
public class BaseActivity extends Activity implements Lifeful {
	
    @Override
    public boolean isAlive() {
        return activityIsAlive();
    }
	public boolean activityIsAlive() {
		if (currentActivity == null) return false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            return !(currentActivity.isDestroyed() || currentActivity.isFinishing());
        } else {
            return !currentActivity.isFinishing();
        }
	}
}
public class BaseFragment extends Fragment implements Lifeful {
	
    @Override
    public boolean isAlive() {
        return activityIsAlive();
    }
    
	public boolean activityIsAlive() {
		Activity currentActivity = getActivity();
		if (currentActivity == null) return false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.JELLY_BEAN_MR1) {
            return !(currentActivity.isDestroyed() || currentActivity.isFinishing());
        } else {
            return !currentActivity.isFinishing();
        }
	}
}
~~~

除了这两个类以外，别的类如果有生命周期，或者包含生命周期的引用，也可以实现Lifeful接口(如View，可以通过onAttachedToWindow()和onDetachedToWindow())。

包含生命周期的异步调用
对于需要用到异步的地方，调用也很方便。

~~~ Java
// ThreadCore是一个用于线程调度的ThreadPoolExecutor封装类，也用于主线程和工作线程之间的切换
ThreadCore.getInstance().postOnMainLooper(new LifefulRunnable(new Runnable() {
    @Override
    public void run() {
        // 实现真正的逻辑。
    }
}, this));
~~~