---
layout: post
title: android 画中画功能总结
category: accumulation
tags:
    - picture-in-picture
keywords: picture-in-picture, PIP
banner: https://cdn.conorlee.top/Flying%20Fox.jpg
thumbnail: https://cdn.conorlee.top/Flying%20Fox.jpg
toc: true
---

首先，Google官方给的画中画api比较简单，想要实现一个画中画需要很多其他交互，本文就以画中画涉及到的具体操作来总结如何实现画中画
<!--more-->
官方文档： https://developer.android.google.cn/guide/topics/ui/picture-in-picture 

### 简单实现

#### 修改清单文件，让activity支持画中画
~~~ java
<activity
    android:name="xxx.VideoActivity"
    android:configChanges="orientation|keyboardHidden|screenSize|keyboard|screenLayout|smallestScreenSize"
    android:launchMode="singleTop"
    android:taskAffinity=".VideoActivity"
    android:supportsPictureInPicture="true"
    android:stateNotNeeded="false"
    android:autoRemoveFromRecents="true"
    android:excludeFromRecents="true" />
~~~

- 1.设置 `android:supportsPictureInPicture` 属性为 true
- 2.configChanges 添加 `smallestScreenSize`，避免进入画中画导致Activity重建
- 3.指定新的`taskAffinity`，可以在小窗模式下打开新的页面，而不是在小窗中打开新页面
- 4.可以添加stateNotNeeded、autoRemoveFromRecents、excludeFromRecents等，使画中画页面不显示在最近的应用列表中

> taskAffinity 需要配合singleTask或singleInstance使用。这里指定为singleTop是有一个小技巧，即可以通过是否添加 intent.addFlags(FLAG_ACTIVITY_NEW_TASK) 标记`动态控制`是否在新任务栈中打开VideoActivity

注意：如果VideoActivity不添加taskAffinity属性，开启画中画的情况下，那么新启动的Activity有 `singleTask`或`FLAG_ACTIVITY_NEW_TASK`标记，新的Activity会展示在画中画中

#### 进入画中画

- 1.主动触发
~~~ java
getActivity().enterPictureInPictureMode();
~~~

- 2.离开页面
~~~ java
@Override
public void onUserLeaveHint () {
    if (iWantToBeInPipModeNow()) {
        enterPictureInPictureMode();
    }
}
~~~

官方只有这两步，但是使用中还会有其他要求，继续往下看：

### 其他交互操作

#### 进入画中画的同时，打开另一个页面
// 需要延时，因为画中画只能打开在前台的Activity，不加延时则画中画最上层页面是 VideoDescActivity
~~~ java
 new Handler().postDelayed(new Runnable() {
    @Override
    public void run() {
        if (null == mActivity) {
            return;
        }
        Intent intent = new Intent(mActivity, VideoDescActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        mActivity.startActivity(intent);
    }
}, 300);
~~~

#### 点击画中画回到VideoActivity：不用特别处理

#### 点击VideoDescActivity上元素，回到VideoActivity
重新启动VideoActivity即可
~~~ java
Intent intent = new Intent(mActivity, VideoActivity.class);
intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
mActivity.startActivity(intent);
~~~

#### 点击VideoDescActivity上元素，关闭画中画

~~~ java
// 使用本地广播，通知 VideoActivity 收到广播后直接 finish
Intent intent = new Intent();
intent.setAction(PIP_ACTION_FILTER_ENTER); // 需要 VideoActivity 注册这个action
intent.putExtra("closeOrReopen", true);
LocalBroadcastManager.getInstance(CartoonGlobalContext.getAppContext()).sendBroadcast(intent);
~~~

#### 监听系统关闭画中画
重写Activity onPictureInPictureModeChanged：
~~~ java
onPictureInPictureModeChanged(boolean isInPictureInPictureMode, Configuration newConfig) {
    if (!isInPictureInPictureMode) {
        // 这里是关闭
    }
}
~~~

#### VideoActivity打开新的页面，比如TimerActivity
~~~ java
Intent intent = new Intent(mActivity, TimerActivity.class);
mActivity.startActivity(intent);
~~~
> 这里必须使用mActivity，不能是Application，否则 TimerActivity 会显示在默认taskAffinity属性的 activity 上。

#### 关闭TimerActivity后，回到VideoActivity

~~~ java
// 重新启动 VideoActivity，并且设置 intent 属性如下：
Intent intent1 = new Intent(this, VideoActivity.class);
intent1.setAction(Intent.ACTION_MAIN);
intent1.addCategory(Intent.CATEGORY_LAUNCHER);
startActivity(intent1);
~~~

#### VideoActivity关闭后，切回到默认task

画中画切到手机桌面上后，将画中画切换为全屏，这时候退出VideoActivity会回到手机桌面，如果需要返回到App的默认activity栈，则需要添加下面的代码：

~~~ java
public static void navToLauncherTask(Context appContext) {
    if (null == appContext || Build.VERSION.SDK_INT < Build.VERSION_CODES.LOLLIPOP) {
        return;
    }
    ActivityManager activityManager = (ActivityManager) appContext.getSystemService(Context.ACTIVITY_SERVICE);
    // iterate app tasks available and navigate to launcher task (browse task)
    final List<ActivityManager.AppTask> appTasks = activityManager.getAppTasks();
    for (ActivityManager.AppTask task : appTasks) {
        final Intent baseIntent = task.getTaskInfo().baseIntent;
        final Set<String> categories = baseIntent.getCategories();
        if (categories != null && categories.contains(Intent.CATEGORY_LAUNCHER)) {
            task.moveToFront();
            return;
        }
    }
}
~~~
> task 也有 startActivity的方法，感兴趣的可以看一下

### 注意问题
- 在支持画中画的activity中，并且开启过画中画，getContext()方法返回值是`ContextThemeWrapper`，用这个跳转页面（getContext().startActivity），会有栈reparent问题，可以使用XXXActivity.this 或者 view.getContext()代替


### REF

https://developer.android.google.cn/guide/topics/ui/picture-in-picture

https://proandroiddev.com/task-management-for-picture-in-picture-mode-on-android-o-882103271cad