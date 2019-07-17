---
layout: post
title: 泛型的应用
category: Java
tags:
    - genericity
keywords: Java, genericity
banner: http://cdn.conorlee.top/Enclosed%20Wheat%20Field%20with%20Peasant.jpg
thumbnail: http://cdn.conorlee.top/Enclosed%20Wheat%20Field%20with%20Peasant.jpg
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

