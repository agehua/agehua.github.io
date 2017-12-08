---
layout: post
title:  android新特性新知识点总结
category: accumulation
tags:
  - ANDROID
  - new features
  - Basic Knowledge
keywords: 新特性, 新知识点,总结
banner: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Chestnut%20Branches.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Chestnut%20Branches.jpg
---


### 一、mipmap 目录和drawable 目录有什么区别
Nexus 6 有 493 ppi，它刚好在 xxhdpi和xxxhdpi之间，所以显示的时候需要对xxxhdpi的资源进行缩小，如果你用了mipmap-xxxhdpi,那么这里会对sclae有一个优化，性能更好，占用内存更少。所以现在官方推荐使用mipmap：

### 二、setTranslucentStatus()方法
在Android4.4之后使用沉浸式状态栏，需要用到这个方法

<!--more-->

~~~ Java
public class MainActivity extends Activity
{
    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        //首先检测当前的版本是否是api>=19的
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT)
        {
            setTranslucentStatus(true);
        }

        SystemBarTintManager tintManager = new SystemBarTintManager(this);
        tintManager.setStatusBarTintEnabled(true);
        tintManager.setStatusBarTintColor(Color.parseColor("#FFC1E0"));
    }

    @TargetApi(19)
    private void setTranslucentStatus(boolean on)
    {
        Window win = getWindow();
        WindowManager.LayoutParams winParams = win.getAttributes();
        final int bits = WindowManager.LayoutParams.FLAG_TRANSLUCENT_STATUS;
        if (on)
        {
            winParams.flags |= bits;
        }
        else
        {
            winParams.flags &= ~bits;
        }
        win.setAttributes(winParams);
    }
}
~~~

布局设置

~~~ Javascript%
<LinearLayout xmlns:android="http://schemas.android.com/apk/res/android"
              android:layout_width="match_parent"
              android:layout_height="match_parent"
              <!--这两行是必须设置的-->
              android:fitsSystemWindows="true"
              android:clipToPadding="true"

              android:orientation="vertical"
              android:background="#FFD9EC"
        >

    <TextView
            android:text="沉浸式状态栏"
            android:layout_width="match_parent"
            android:layout_height="50dp"
            android:textSize="23dp"
            android:layout_gravity="center_horizontal"
            android:gravity="center"
            android:background="#FFD9EC"
            />
    <TextView
            android:layout_width="match_parent"
            android:layout_height="match_parent"
            android:background="@android:color/darker_gray"/>

</LinearLayout>
~~~

### 三、获取Bitmap图片大小的代码

~~~ Java
public int getBitmapSize(Bitmap bitmap){
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT){     //API 19
        return bitmap.getAllocationByteCount();
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB_MR1){//API 12
        return bitmap.getByteCount();
    }
    return bitmap.getRowBytes() * bitmap.getHeight();                //earlier version
}
~~~


### 四、Activity横竖屏切换生命周期

总结：

* 1、不设置Activity的android:configChanges时，切屏会重新调用各个生命周期，切横屏时会执行一次，切竖屏时会执行两次

* 2、设置Activity的android:configChanges="orientation"时，切屏还是会重新调用各个生命周期，切横、竖屏时只会执行一次

* 3、设置Activity的android:configChanges="orientation\|keyboardHidden"时，切屏不会重新调用各个生命周期，只会执行onConfigurationChanged方法

验证：

1、新建一个Activity，并把各个生命周期打印出来

2、运行Activity，得到如下信息

~~~ Java
onCreate-->
onStart-->
onResume-->
~~~

3、按crtl+f12切换成横屏时

~~~ Java
onSaveInstanceState-->
onPause-->
onStop-->
onDestroy-->
onCreate-->
onStart-->
onRestoreInstanceState-->
onResume-->
~~~
4、再按crtl+f12切换成竖屏时，发现打印了两次相同的log

~~~ Java
onSaveInstanceState-->
onPause-->
onStop-->
onDestroy-->
onCreate-->
onStart-->
onRestoreInstanceState-->
onResume-->
onSaveInstanceState-->
onPause-->
onStop-->
onDestroy-->
onCreate-->
onStart-->
onRestoreInstanceState-->
onResume-->
~~~
5、修改AndroidManifest.xml，把该Activity添加 android:configChanges="orientation"，执行步骤3

~~~ Java
onSaveInstanceState-->
onPause-->
onStop-->
onDestroy-->
onCreate-->
onStart-->
onRestoreInstanceState-->
onResume-->
~~~
6、再执行步骤4，发现不会再打印相同信息，但多打印了一行onConfigChanged

~~~ Java
onSaveInstanceState-->
onPause-->
onStop-->
onDestroy-->
onCreate-->
onStart-->
onRestoreInstanceState-->
onResume-->
onConfigurationChanged-->
~~~
7、把步骤5的android:configChanges="orientation" 改成android:configChanges="orientation\|keyboardHidden"，执行步骤3，就只打印onConfigChanged

~~~ Java
onConfigurationChanged-->
~~~
8、执行步骤4

~~~ Java
onConfigurationChanged-->
onConfigurationChanged-->
~~~

总结一下整个Activity的生命周期

  - 1.补充一点，当前Activity产生事件弹出Toast和AlertDialog的时候Activity的生命周期不会有改变
  - 2.Activity运行时按下HOME键(跟被完全覆盖是一样的)：onSaveInstanceState --> onPause --> onStop       onRestart -->onStart--->onResume
  - 3.Activity未被完全覆盖只是失去焦点：onPause--->onResume
