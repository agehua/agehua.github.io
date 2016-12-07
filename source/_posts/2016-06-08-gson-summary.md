---
layout: post
title: Gson解析使用总结
category: accumulation
tags: accumulation
keywords: gson, json
description: Gson解析使用总结
banner: http://obxk8w81b.bkt.clouddn.com/Child%20with%20Orange.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Child%20with%20Orange.jpg
---


### 1.Gson解析总结，就两种情况
Gson 是Google提供的用来在Java对象和JSON数据之间进行映射的Java类库。可以将一个JSON字符串转成一个Java对象，或者将Java对象转成一个JSON字符串。
使用Gson来解析时，总结一下，就两句话：

- 1.**遇到“{”字符，表示单个对象，直接用XXXBean.class类去映射**
- 2.**遇到“[”字符，表示对象数组，要用XXXBean[].class或list<XXXBean>去映射**

<!--more-->

### 2.第一种情况，举例

- json字符串是：

~~~ javascript
{
    "items": [
        {
            "key": "H60-L12__1464938219953__346589_483",
            "hash": "Fip_In4BhB5syoZ28W3l_eb6rBDU",
            "fsize": 348120,
            "mimeType": "image/jpeg",
            "putTime": 14649382507765902
        },
        {
            "key": "H60-L12__1465352164202__397253_2946",
            "hash": "Fs-UwiosdckN9kVY01GrOYC-b7a9",
            "fsize": 938413,
            "mimeType": "image/jpeg",
            "putTime": 14653521712332144
        },
        {
            "key": "H60-L12__1465352548728__322577_4114",
            "hash": "Frga_QiMJVb9caiuwMlJABCsu1rc",
            "fsize": 506921,
            "mimeType": "image/jpeg",
            "putTime": 14653525989907168
        }
    ]
}
~~~


- 分析

最开始是一个“{”字符，所以需要用gson.fromJson(jsonstring, XXXBean.class)来解析。同时，XXXBean中只有一个字段items。
items里面是一个“[”字符，表示items里面是数组，可以用list去映射。


- 解析时，

最后对应的Gson对象就是：

~~~ Java
public class XXXBean {

    public List<XXXItem> items;

    public List<XXXItem> getItems() {
        return items;
    }

    public void setItems(List<XXXItem> items) {
        this.items = items;
    }

    public static class XXXItem {
        private String key;
        private String hash;
        private String fsize;
        private String mimeType;
        private String putTime;

        ...
	}
}

Gson gson = new Gson();
List<XXXBean.XXXItem> list = gson.fromJson(jsonstring,XXXBean.class).getItems();
~~~


### 3.第二种情况，举例

json字符串以“[”开头。

例如，json==[{"id":1,"name":"李坤","birthDay":"Jun 22, 2012 8:28:52 AM"},{"id":2,"name":"曹贵生","birthDay":"Jun 22, 2012 8:28:52 AM"},{"id":3,"name":"柳波","birthDay":"Jun 22, 2012 8:28:52 AM"}]

- 解析时，需要使用list来接收。

~~~ Java
 List<Student> retList = gson.fromJson(jsonstring2,  new TypeToken<List<Student>>(){}.getType());
~~~


- 也可以这样

~~~ Java
Student[] students= gson.fromJson(jsonstring2,new Student[].class);
~~~

- 对应list为什么要使用TypeToken？

	TypeToken是Gson提供的，来实现对泛型的支持
