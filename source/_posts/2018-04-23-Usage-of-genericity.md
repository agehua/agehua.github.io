---
layout: post
title: 泛型的应用
category: Java
tags:
    - genericity
keywords: Java, genericity
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Enclosed%20Wheat%20Field%20with%20Peasant.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Enclosed%20Wheat%20Field%20with%20Peasant.jpg
toc: true
---
我在之前转载过一篇Java泛型的介绍：[JAVA 泛型通配符 T，E，K，V 区别，T 以及 Class<T>，Class<?> 的区别](/2016/11/21/Difference_between-T-E-K-V/)

最近在开发中遇到一个场景，想简化流程，突然发现可以使用泛型。
这个场景就是，在手机上实现分页加载的功能时，把通用的数据映射成对象，然后使用泛型来替代具体业务类，从而封装出一个对应分页加载的类。

<!--more-->
有点绕，往下看：

### 泛型的应用
在手机上实现分页加载功能，一般都会去服务器请求一个list，但除了list之外，可能还有其他通用的消息内容，比如下面的这个Json内容：

~~~ Json
{
    "list": [{
		"description": "b",
		"editTime": "12/04/2018",
		"expireTime": "20/04/2018 16:30",
		"id": 105,
		"lang": "en",
		"sendTimeShort": "12/04/2018",
		"status": "3",
		"title": "a1",
		"unread": "1"
	},
	...
	{
		"description": "d20",
		"editTime": "10/04/2018",
		"expireTime": "11/04/2018 15:19",
		"id": 78,
		"lang": "en",
		"sendTimeShort": "10/04/2018",
		"status": "3",
		"title": "push notification",
		"unread": "1"
	}],
	"pageNo": 1,
	"pageSize": 10,
	"totalCount": 19,
	"totalPages": 2
}
~~~

可以发现上面的json串的末尾，是一些跟**具体业务无关**的**固定格式的键值对**，这些键值对可以是分页需要的信息，也可以是交易验等证信息。

上面除了通用信息，还有一个list，这个list就跟具体业务有关了，可以用泛型 `<T> `来代替。

针对上面的json，写出一个对象，代码如下：

~~~ Java
public class FCMModel<T extends Parcelable> implements Parcelable {

    private int pageNo; //对应上面json里的键值对
    private int totalPages;
    private int pageSize;
    private int totalCount;

    private ArrayList<T> list;
    private static String DATA_KEY = "bean_key";//这个值在序列化 ArrayList 的时候用到

    //省略参数的 get 和 set 方法...

    public List<T> getList() {
        return list;
    }

    public void setList(ArrayList<T> list) {
        this.list = list;
    }

    //继承 Parcelable 必须的方法
    public static final Creator<FCMModel> CREATOR = new Creator<FCMModel>() {
        @Override
        public FCMModel createFromParcel(Parcel in) {
            return new FCMModel(in);
        }

        @Override
        public FCMModel[] newArray(int size) {
            return new FCMModel[size];
        }
    };

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeInt(this.pageNo);
        dest.writeInt(this.totalPages);
        dest.writeInt(this.pageSize);
        dest.writeInt(this.totalCount);
        Bundle bundle = new Bundle();
        //注意ArrayList的序列化方法
        bundle.putParcelableArrayList(DATA_KEY, list);
        dest.writeBundle(bundle);
    }

    public FCMModel() {
    }

    protected FCMModel(Parcel in) {
        this.pageNo = in.readInt();
        this.totalPages = in.readInt();
        this.pageSize = in.readInt();
        this.totalCount = in.readInt();
        this.list = in.readBundle().getParcelableArrayList(DATA_KEY);
    }


    public static class NotificationListBean implements Parcelable {
        //这个类只是一个业务类，除了实现Parcelable 接口外没有特殊方法，这里全部省略 ...
    }
}
~~~
在上面的代码中，FCMModel就是抽象出来的类，它封装了分页功能中的通用数据，开发者可以只简单实现业务类就行了，是不是方便了一点点，哈哈。

### fastjson 泛型转换

> 注意：在使用 fastjson 对有泛型存在的实体类做序列化时，必须要用到 com.alibaba.fastjson.TypeReference 类，否则会抛出 java.lang.ClassCastException 异常。

我是参考的这篇文章：[fastJson泛型如何转换](https://blog.csdn.net/csdn_xpw/article/details/65022177)

针对本例子，应该这样使 fastjson 做转换：

~~~ Java
final static Type type = new TypeReference<FCMModel<FCMModel.NotificationListBean>>() {}.getType();
List<FCMModel.NotificationListBean> mBeanList= new ArrayList<>();

//...
mFCMModel = JSON.parseObject(result.getDataStr(), type);
mBeanList = mFCMModel.getList();
~~~

`这里的Type是 java.lang.reflect.Type `

### 获取泛型的类型参数

以kotlin语音为例，我们不能直接获取一个泛型的类型，比如：
~~~ java
val listType = ArrayList<String>::class.java // 不被允许
val mapType = Map<String, String>::class.java // 不被允许
~~~
如何获取一个泛型的具体类型呢，有两种方式：

#### 使用匿名内部类
先看下面的例子：
~~~ java
val list1 = ArrayList<String>()
val list2 = object: ArrayList<String>() {} // 使用object创建匿名内部类

println(list1.javaClass.genericSuperclass)
println(list2.javaClass.genericSuperclass) 

// 结果：
java.util.AbstractList<E>
java.util.ArrayList<java.lang.String>
~~~
list2成功拿到了泛型的具体类型，这是因为 **泛型类型擦除并不是真的将全部的类型信息都擦除，还是会将类型信息放在对应class的常量池中**

**匿名内部类在初始化的时候就会绑定父类或父接口的相应信息**，这样就能通过获取父类或父接口的泛型信息来实现我们的需求。

我们来设计一个能获取所有类型的信息的泛型类：

~~~ java
import java.lang.reflect.ParameterizedType
import java.lang.reflect.Type

open class GenericsToken<T> {
    var type : Type = Any::class.java
    init {
        // 在初始化的时候就能拿到父类或父接口的类型信息
        val superClass = this.javaClass.genericSuperclass
        type = (superClass as ParameterizedType).getActualTypeArguments()[0]
    }
}

// 使用：
val gt = object: GenericsToken<Map<String, String>>() {} // 一个匿名内部类
println(gt.type)

// 结果：
java.util.Map<java.lang.String, ? extends java.lang.String>
~~~

到这里再看下上面的fastjson是如何获取泛型类型的，是不是就可以为什么fastjson要这样写了？

同理，Gson也是用的相同的设计

#### 使用内联函数
其实Kotlin中的内联函数在编译的时候，编译器也会将相应函数的字节码插入到调用的地方，参数类型也会插入到字节码中，这样我们就可以获取参数的类型了。
~~~ java
inline fun <reified T> getType() { // 关键字 reified
    return T::class.java
}
~~~
这里的意思相当于在编译的会将具体的类型插入响应的字节码中，那么我们就能在运行时获取到对应参数的类型了。所以可以在Kotlin中改进Gson的使用方式：

~~~ java
inline fun <reified T : Any> Gson.fromJson(json: String) : T { // 对 Gson.fromJson进行扩展
    return Gson().fromJson(json, T::class.java)
}

// 使用
val json =" ... "
val stringList = Gson().fromJson<List<String>>(json) // 只需传入json一个参数
~~~

`reified` 关键字带来的特性在Android开发中也格外有用，比如修改 startActivity 方法：
~~~ java
inline fun <reified T : Activity> Activity.startActivity() {
    startActivity(Intent(this, T::class.java))
}

// 使用
startActivity<DetailActivity>()
~~~

更多关于 `reified` 关键字的介绍可以看这篇文章：<https://juejin.cn/post/6844903833596854279>
