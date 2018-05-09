---
layout: post
title:  Android面试知识点整理
category: accumulation
tags:
  - ANDROID
  - Interview Knowledge
keywords: Android,面试题
banner: http://obxk8w81b.bkt.clouddn.com/Cottages%20Reminiscence%20of%20the%20North.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Cottages%20Reminiscence%20of%20the%20North.jpg
---


只整理android面试中涉及到的知识点。不断补充中。。。

#### 1.Android事件传递机制
下面内容摘选自《Android开发艺术探索》

- (1) 当一个点击事件发生之后，传递过程遵循如下顺序：**Activity -> Window -> View**。
如果一个view的onTouchEvent方法返回false，那么它的父容器的onTouchEvent方法将会被调用，依此类推，如果所有的元素都不处理这个事件，那么这个事件将会最终传递给Activity处理(调用Activity的onTouchEvent方法)。

<!--more-->
- (2) 正常情况下，一个事件序列只能被一个view拦截并消耗，因为一旦某个元素拦截了某个事件，那么同一个事件序列内的所有事件都会直接交给它处理，并且该元素的**onInterceptTouchEvent**方法不会再被调用了。
- (3) 某个view一旦开始处理事件，如果它不消耗ACTION_DOWN事件，那么**同一事件序列的其他事件都不会再交给它来处理**，并且事件将重新交给它的**父容器去处理**(调用父容器的onTouchEvent方法)；如果它消耗ACTION_DOWN事件，但是不消耗其他类型事件，那么这个点击事件会消失，父容器的onTouchEvent方法不会被调用，**当前view依然可以收到后续的事件**，但是这些事件最后都会传递给Activity处理。
- (4) ViewGroup默认不拦截任何事件，因为它的**onInterceptTouchEvent**方法默认返回false。view没有onInterceptTouchEvent方法，一旦有点击事件传递给它，那么它的onTouchEvent方法就会被调用。
- (5) View的**onTouchEvent**默认都会消耗事件(返回true)，除非它是不可点击的(clickable和longClickable都为false)。view的longClickable默认是false的，clickable则不一定，Button默认是true，而TextView默认是false。
- (6) **View的enable属性不影响onTouchEvent的默认返回值**。哪怕一个view是disable状态，只要它的clickable或者longClickable有一个是true，那么它的onTouchEvent就会返回true。
- (7) 事件传递过程总是**先传递给父元素**，然后再由**父元素分发给子view**，通过**requestDisallowInterceptTouchEvent**方法可以在**子元素中干预父元素的事件分发过程**，但是**ACTION_DOWN事件除外**，即当面对ACTION_DOWN事件时，ViewGroup总是会调用自己的onInterceptTouchEvent方法来询问自己是否要拦截事件。
ViewGroup的dispatchTouchEvent方法中有一个标志位**FLAG_DISALLOW_INTERCEPT**，这个标志位就是通过**子view调用requestDisallowInterceptTouchEvent方法**来设置的，一旦设置为true，那么ViewGroup不会拦截该事件。
- (8) 以上结论均可以在书中的源码解析部分得到解释。Window的实现类为PhoneWindow，获取Activity的contentView的方法

~~~ Java
((ViewGroup)getWindow().getDecorView().findViewById(android.R.id.content)).getChildAt(0);
~~~

#### 2.OnTouchListener、onTouchEvent、OnClickListener优先级顺序
如果给一个view设置了OnTouchListener，那么OnTouchListener中的onTouch方法会被回调。这时事件如何处理还要看**onTouch**的返回值，如果返回false，那么当前view的**onTouchEvent方法**会被调用；如果返回true，那么onTouchEvent方法将不会被调用。
在onTouchEvent方法中，如果当前view设置了OnClickListener，那么它的onClick方法会被调用，所以OnClickListener的优先级最低。

#### 3.AsyncTask的方法介绍
- 1. onPreExecute()
这个方法会在后台任务开始执行之间调用，用于进行一些界面上的初始化操作，比如显示一个进度条对话框等。

- 2. doInBackground(Params...)
这个方法中的所有代码都会在子线程中运行，我们应该在这里去处理所有的耗时任务。任务一旦完成就可以通过return语句来将任务的执行结果进行返回，如果AsyncTask的第三个泛型参数指定的是Void，就可以不返回任务执行结果。注意，在这个方法中是不可以进行UI操作的，如果需要更新UI元素，比如说反馈当前任务的执行进度，可以调用publishProgress(Progress...)方法来完成。

- 3. onProgressUpdate(Progress...)
当在后台任务中调用了publishProgress(Progress...)方法后，这个方法就很快会被调用，方法中携带的参数就是在后台任务中传递过来的。在这个方法中可以对UI进行操作，利用参数中的数值就可以对界面元素进行相应的更新。

- 4. onPostExecute(Result)
当后台任务执行完毕并通过return语句进行返回时，这个方法就很快会被调用。返回的数据会作为参数传递到此方法中，可以利用返回的数据来进行一些UI操作，比如说提醒任务执行的结果，以及关闭掉进度条对话框等。

#### 4.项目中Handler怎么使用？

#### 5.项目中图片的适配问题怎么解决？

- dpi 
每英寸点数，全称dots per inch。用来表示屏幕密度，即屏幕物理区域中的像素量。高密度屏幕比低密度屏幕在给定物理区域的像素要多。
- dp 
即dip，全称device independent pixel。设备独立像素，是一种虚拟像素单位，用于以密度无关方式表示布局维度或位置，以确保在不同密度的屏幕上正常显示UI。在160dpi的设备上，1dp=1px。
- density 
设备的逻辑密度，是dip的缩放因子。以160dpi的屏幕为基线，density=dpi/160。
    ~~~ Java
    getResources().getDisplayMetrics().density
    ~~~
- sp 
缩放独立像素，全称scale independent pixel。类似于dp，一般用于设置字体大小，可以根据用户设置的字体大小偏好来缩放。

总结一下图片查找过程：优先匹配最适合的图片→查找密度高的目录（升序）→查找密度低的目录（降序)

也可以使用.9图片

#### 6.Android存储敏感信息的方式有？
使用SharedPreferences, getSharedPreferences 指定为 MODE_PRIVATE
运用SQLite数据库
保存到 SDCard: FileOutputStream fos = this.openFileOutput("oauth_1.out",Context.MODE_WORLD_READABLE); 
Keystore 也可以保存密钥
写入SO 文件
结合 ContentProvider 来保存信息
上传到服务器由服务器保存
采用多进程，放在单独的进程中保存

#### 7.自定义广播
- 继承自BroadcastReceiver
- 重写onReceive(Context,Intent)
      Intent.getAction
- 动态注册（Context.registerReceiver()），静态注册（使用IntentFilter指定action）


#### 8.加分项
- JNI开发
- 性能优化
- 优秀作品

#### 9.Sqlite数据库更新并保留升级前的数据
我们知道在SQLiteOpenHelper的构造方法:

~~~ Java
super(Context context, String name, SQLiteDatabase.CursorFactory factory, int version)
~~~
中最后一个参数表示数据库的版本号.当新的版本号大于当前的version时会调用方法:

~~~ Java
onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion)
~~~
所以我们的重点是在该方法中实现SQLite数据库版本升级的管理

对于保留升级前的数据，有两种解决办法：

**SQLite提供了ALTER TABLE命令**，允许用户重命名或添加新的字段到已有表中，但是不能从表中删除字段。并且只能在表的末尾添加字段，比如，为Subscription添加两个字段：

- ALTER TABLE Subscription ADD COLUMN Activation BLOB;
- ALTER TABLE Subscription ADD COLUMN Key BLOB;

**注释**：Sqlite支持BLOB(二进制大对象)数据类型

**保留数据删除原表创建新表**，具体思路是：

- 1：将表A重新命名：例如重新命名为：temp_A
- 2：创建新表A
- 3：将temp_A中的数据【也就是更新前的数据】插入到新表A

具体操作如下：

~~~ Java
//重命名原来的数据表
public static final String TEMP_SQL_CREATE_TABLE_SUBSCRIBE = "alter table "
            + A + " rename to temp_A";

//然后把备份表temp_A中的数据copy到新创建的数据库表A中，这个表A没发生结构上的变化
public static final String INSERT_SUBSCRIBE = "select 'insert into A (code,name,username,tablename)
                        values ("code","name","cnki","tablename")' as insertSQL from temp_A";

//删除备份表
public static final String DELETE_TEMP_SUBSCRIBE = "delete from temp_A ";
public static final String DROP_TEMP_SUBSCRIBE = "drop table if exists temp_A";

@Override
public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {

        for (int j = oldVersion; j <= newVersion; j++) {
            switch (j) {
            case 2:
　　　　　　　　　//创建临时表
                db.execSQL(TEMP_SQL_CREATE_TABLE_SUBSCRIBE);
　　 　　　　　　//执行OnCreate方法，这个方法中放的是表的初始化操作工作，比如创建新表之类的
                onCreate(db);
　　　　　　　　　//删除之前的表里面的默认数据
                for (int i = 0; i < arrWhereAct.length; i++) {
                    db.execSQL(DELETE_TEMP_SUBSCRIBE + arrWhereAct[i]);
                }

                //将临时表中的数据放入表A
　　　　　　　　　Cursor cursor = db.rawQuery(INSERT_SUBSCRIBE, null);
                if (cursor.moveToFirst()) {
                    do {
                        db.execSQL(cursor.getString(cursor
                                .getColumnIndex("insertSQL")));
                    } while (cursor.moveToNext());
                }

                cursor.close();
　　　　　　　　　//将临时表删除掉
                db.execSQL(DROP_TEMP_SUBSCRIBE);

                break;

            default:
                break;
            }
        }
}
~~~

> **注意**，为什么要在方法里写for循环，主要是考虑到跨版本升级，比如有的用户一直不升级版本，数据库版本号一直是1，而客户端最新版本其实对应的数据库版本已经是4了，那么我中途可能对数据库做了很多修改，通过这个for循环，可以迭代升级，不会发生错误。


#### 10.AIDL支持的数据类型
其实AIDL这门语言非常的简单，基本上它的语法和 Java 是一样的，只是在一些细微处有些许差别——毕竟它只是被创造出来简化Android程序员工作的，太复杂不好——所以在这里我就着重的说一下它和 Java 不一样的地方。主要有下面这些点：

**文件类型**：用AIDL书写的文件的后缀是 .aidl，而不是 .java。

**数据类型**：AIDL默认支持一些数据类型，在使用这些数据类型的时候是不需要导包的，但是除了这些类型之外的数据类型，在使用之前必须导包，就算目标文件与当前正在编写的 .aidl 文件在同一个包下——在 Java 中，这种情况是不需要导包的。比如，现在我们编写了两个文件，一个叫做Book.java ，另一个叫做 BookManager.aidl，它们都在 com.lypeer.aidldemo 包下 ，现在我们需要在 .aidl 文件里使用 Book 对象，那么我们就必须在 .aidl 文件里面写上 import com.lypeer.aidldemo.Book; 哪怕 .java 文件和 .aidl 文件就在一个包下。

   默认支持的数据类型包括：

- Java中的八种基本数据类型，包括 byte，short，int，long，float，double，boolean，char。
- String 类型。
- CharSequence类型。
- List类型：List中的所有元素必须是AIDL支持的类型之一，或者是一个其他AIDL生成的接口，或者是定义的parcelable（下文关于这个会有详解）。List可以使用泛型。
- Map类型：Map中的所有元素必须是AIDL支持的类型之一，或者是一个其他AIDL生成的接口，或者是定义的parcelable。Map是不支持泛型的。

**定向tag**：这是一个极易被忽略的点——这里的“被忽略”指的不是大家都不知道，而是很少人会正确的使用它。在我的理解里，定向tag是这样的：AIDL中的定向tag表示了在跨进程通信中数据的流向，其中**in表示数据只能由客户端流向服务端**，**out表示数据只能由服务端流向客户端**，**而inout则表示数据可在服务端与客户端之间双向流通**。其中，**数据流向是针对在客户端中的那个传入方法的对象而言的**。in为定向tag的话表现为服务端将会接收到一个那个对象的完整数据，但是客户端的那个对象不会因为服务端对传参的修改而发生变动；out的话表现为服务端将会接收到那个对象的的空对象，但是在服务端对接收到的空对象有任何修改之后客户端将会同步变动；inout为定向tag的情况下，服务端将会接收到客户端传来对象的完整信息，并且客户端将会同步服务端对该对象的任何变动。

另外，Java 中的基本类型和String，CharSequence的定向tag默认且只能是in。还有，请注意，请不要滥用定向tag，而是要根据需要选取合适的——要是不管三七二十一，全都一上来就用inout，等工程大了系统的开销就会大很多——因为排列整理参数的开销是很昂贵的。

**两种AIDL文件**：在我的理解里，所有的AIDL文件大致可以分为两类。**一类是用来定义parcelable对象**，以供其他AIDL文件使用AIDL中非默认支持的数据类型的。**一类是用来定义方法接口**，以供系统使用来完成跨进程通信的。可以看到，两类文件都是在“定义”些什么，而不涉及具体的实现，这就是为什么它叫做“Android接口定义语言”。
注：所有的非默认支持数据类型必须通过第一类AIDL文件定义才能被使用。

下面是两个例子，对于常见的AIDL文件都有所涉及：

~~~ Java
// Book.aidl
//第一类AIDL文件的例子
//这个文件的作用是引入了一个序列化对象 Book 供其他的AIDL文件使用
//注意：Book.aidl与Book.java的包名应当是一样的
package com.lypeer.ipcclient;

//注意parcelable是小写
parcelable Book;

// BookManager.aidl
//第二类AIDL文件的例子
package com.lypeer.ipcclient;
//导入所需要使用的非默认支持数据类型的包
import com.lypeer.ipcclient.Book;

interface BookManager {

    //所有的返回值前都不需要加任何东西，不管是什么数据类型
    List<Book> getBooks();
    Book getBook();
    int getBookCount();

    //传参时除了Java基本类型以及String，CharSequence之外的类型
    //都需要在前面加上定向tag，具体加什么量需而定
    void setBookPrice(in Book book , int price)
    void setBookName(in Book book , String name)
    void addBookIn(in Book book);
    void addBookOut(out Book book);
    void addBookInout(inout Book book);
}
~~~


#### 11.Android布局的优化方案

- 层级观察器(Hierarchy Viewer)：
- 使用layoutopt工具输出
- 重用布局文件：<include>
- 使用< merge />标签减少布局的嵌套层次；
- 仅在需要时才加载布局，ViewStub

#### 12.Android性能调优
- UI卡顿
- ANR异常
- 内存性能优化
- Android API使用：
        StringBuffer/String、HashMap/ArrayMap/SparseArray

性能调优参考这篇博客[Andoid应用开发性能优化完全分析](http://blog.csdn.net/yanbober/article/details/48394201)


####  使用 DialogFragment 创建对话框
使用DialogFragment至少需要实现onCreateView或者onCreateDIalog方法。onCreateView即使用定义的xml布局文件展示Dialog。onCreateDialog即利用AlertDialog或者Dialog创建出Dialog。
> 注：官方不推荐直接使用Dialog创建对话框。

没有布局的Fragment的作用
没有布局文件Fragment实际上是为了保存，当Activity重启时，保存大量数据准备的

在运行时配置发生变化时，在Fragment中保存有状态的对象
a) 继承Fragment，声明引用指向你的有状态的对象
b) 当Fragment创建时调用setRetainInstance(boolean)
c) 把Fragment实例添加到Activity中
d) 当Activity重新启动后，使用FragmentManager对Fragment进行恢复

~~~ Java
import android.app.Fragment;
import android.os.Bundle;

/** 
 * 保存对象的Fragment 
 *  
 * @author zhy 
 *  
 */  
public class OtherRetainedFragment extends Fragment  
{  
  
    // data object we want to retain  
    // 保存一个异步的任务  
    private MyAsyncTask data;  
  
    // this method is only called once for this fragment  
    @Override  
    public void onCreate(Bundle savedInstanceState)  
    {  
        super.onCreate(savedInstanceState);  
        // retain this fragment  
        setRetainInstance(true);  
    }  
  
    public void setData(MyAsyncTask data)  
    {  
        this.data = data;  
    }
    
    public MyAsyncTask getData()
    {
        return data;
    }
}
~~~


具体参考：[Android 屏幕旋转 处理 AsyncTask 和 ProgressDialog 的最佳方案](http://blog.csdn.net/lmj623565791/article/details/37936275)