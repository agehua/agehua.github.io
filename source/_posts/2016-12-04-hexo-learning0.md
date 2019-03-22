---
layout: post
title:  Hexo 模板学习——EJS介绍
category: accumulation
tags:
  - Hexo
  - EJS
keywords: EJS介绍
banner: http://cdn.conorlee.top/Bulb%20Fields.jpg
thumbnail: http://cdn.conorlee.top/Bulb%20Fields.jpg
toc: true
---

### 什么是EJS？

EJS是JavaScript模板库，用来从JSON数据中生成HTML字符串

### EJS的语法和功能：
#### 1、缓存功能，能够缓存已经解析好的html模版

#### 2、&lt;% code %&gt;用于执行其中javascript代码。
~~~ javascript
<% alert('hello world') %>
~~~
#### 3、&lt;%= code =%&gt;会对code进行html转义；

<!--more-->

~~~ javascript
<h1><%=title %></h1>                    注：会把title里面存的值给显示出来在h1中。
<p><%= 'hello world' %></p>             注：会把hello world显示在h1中。
<h1><%= '<b>hello world</b>' %></h1>    注：会把hello world变粗，然后显示在h1中。
~~~ javascript
#### 4、<%- code %>将不会进行转义；，这一行代码不会执行，像是被注释了一样，然后显示原来的html。也不会影响整个页面的执行。

~~~ javascript
<h1><%-title %>asd</h1>          最后显示asd，及显示原网页
<p><%# 'hello world' %>asd</p>   最后显示asd，及显示原网页
~~~
#### 5、支持自定义标签，比如'&lt;%'可以使用'{{'，'%&gt;'用'}}'代替；

    ejs 里，默认的闭合标记是 <%  .. %>，我们也可以定义自己的标签。例如：

~~~ javascript
app.set("view options",{                                                                                  
   "open":"{{",                                                                                  
   "close":"}}"
});
~~~
#### 6、提供一些辅助函数，用于模版中使用

- 1)、first，返回数组的第一个元素；
- 2)、last，返回数组的最后一个元素；
- 3)、capitalize，返回首字母大写的字符串；
- 4)、downcase，返回字符串的小写；
- 5)、upcase，返回字符串的大写；
- 6)、sort，排序（Object.create(obj).sort()？）；
- 7)、sort_by:'prop'，按照指定的prop属性进行升序排序；
- 8)、size，返回长度，即length属性，不一定非是数组才行；
- 9)、plus:n，加上n，将转化为Number进行运算；
- 10)、minus:n，减去n，将转化为Number进行运算；
- 11)、times:n，乘以n，将转化为Number进行运算；
- 12)、divided_by:n，除以n，将转化为Number进行运算；
- 13)、join:'val'，将数组用'val'最为分隔符，进行合并成一个字符串；
- 14)、truncate:n，截取前n个字符，超过长度时，将返回一个副本
- 15)、truncate_words:n，取得字符串中的前n个word，word以空格进行分割；
- 16)、replace:pattern,substitution，字符串替换，substitution不提供将删除匹配的子串；
- 17)、prepend:val，如果操作数为数组，则进行合并；为字符串则添加val在前面；
- 18)、append:val，如果操作数为数组，则进行合并；为字符串则添加val在后面；
- 19)、map:'prop'，返回对象数组中属性为prop的值组成的数组；
- 20)、reverse，翻转数组或字符串；
- 21)、get:'prop'，取得属性为'prop'的值；
- 22)、json，转化为json格式字符串

**利用&lt;%- include filename %&gt;加载其他页面模版；**

#### ejs我的总结：

ejs 写法：

- 1.普通传入并使用变量：

~~~ JavaScript
<%= title %>
~~~

- 2.普通for执行js代码（for中间的代码一定可以执行到）：

~~~ JavaScript
<% for(var i=0; i<headerNavbar.length; i++) {%>
    <li><a href="/reg"><%= headerNavbar[i].name %></a></li>
<% } %>
~~~

- 3.特殊if语句的js代码（if中间的额代码不一定可以执行到）：

~~~ JavaScript
 <% if(active=='index'){%>
class="active"
 <% }%>
~~~

### stylus介绍
部分内容转载自：[stylus入门使用方法](https://segmentfault.com/a/1190000002712872)

stylus项目地址：http://stylus-lang.com/

Stylus 是一个CSS的预处理框架，2010年产生，来自Node.js社区，主要用来给Node项目进行CSS预处理支持，所以 Stylus 是一种新型语言，可以创建健壮的、动态的、富有表现力的CSS。比较年轻，其本质上做的事情与 SASS/LESS 等类似，应该是有很多借鉴，所以近似脚本的方式去写CSS代码。

Stylus功能上更为强壮，和js联系更加紧密（EXPRESSIVE, DYNAMIC, ROBUST CSS）

#### Stylus安装

使用node包管理器，全局安装
~~~ JavaScript
$ npm install stylus -g
~~~

#### 生成CSS
建立一个stylusExample/，再在里面建立 src 目录专门存放 stylus 文件，在里面建立 example.styl 文件。然后在 stylusExample 目录下面执行下面命令
~~~ JavaScript
$ stylus --compress src/
~~~

输出compiled src/example.css ，这个时候表示你生成成功了，带上--compress参数表示你生成压缩的CSS文件。
~~~ JavaScript
$ stylus --css css/example.css css/out.styl CSS转换成styl
$ stylus help box-shadow CSS属性的帮助
$ stylus --css test.css 输出基本名一致的.styl文件
~~~

具体语法和应用可以参考: [Stylus中文参考文档](http://www.zhangxinxu.com/jq/stylus/)

### CSS3 column多列布局介绍
CSS3提供了个新属性columns用于多列布局。在这之前，有些大家习以为常的排版，要用CSS动态实现其实是比较困难的。如竖版报纸

Columns属性最大的问题还是浏览器兼容性问题
摘选自：[CSS3 column多列布局介绍](http://cdn2.jianshu.io/p/87d1862f88c6)

### 栅格与响应式设计
栅格样式库一般是这样做的：将页面划分为若干等宽的列（column），然后推荐你通过等宽列来创建响应式的页面区块。

虽然看起来都是这样的思路，但不同的栅格样式库，在做法上却是各有各的点子。下面，本文将介绍几个比较有代表性的栅格样式库，讲述它们的简要原理和用法
#### Bootstrap中的栅格:
[Bootstrap](http://getbootstrap.com/)把它的栅格放在CSS这个分类下，并称它为Gird system。默认分为12列。

要理解Bootstrap中的栅格，最好从掌握正确的使用方法开始。这其中有2个要点。
- 第1个要点是容器（container），行（row）和列（column）之间的层级关系。一个正确的写法示例如下：
~~~ JavaScript
<div class="container">
    <div class="row">
        <div class="col-md-6"></div>
        <div class="col-md-6"></div>
    </div>
</div>
~~~
- 第2个要点，是不同的断点类型的意义及其搭配。

  Bootstrap栅格的column对应的类名形如.col-xx-y。y是数字，表示该元素的宽度占据12列中的多少列。而xx只有特定的几个值可供选择，分别是xs、sm、md、lg，它们就是断点类型。

  在Bootstrap栅格的设计中，断点的意义是，当视口（viewport）宽度小于断点时，column将竖直堆叠（display: block的默认表现），而当视口宽度大于或等于断点时，column将水平排列（float的效果）。按照xs、sm、md、lg的顺序，断点像素值依次增大，其中xs表示极小，即认为视口宽度永远不小于xs断点，column将始终水平浮动。

#### Foundation中的栅格
Foundation栅格叫做Grid，它和Bootstrap栅格的设计十分近似，只是在类名和结构上有所差异。Foundation栅格同样默认12列。

- 行与列
类比之前Bootstrap栅格的例子，Foundation栅格的一个正确的写法示例如下：
~~~ JavaScript
<div class="row">
    <div class="medium-6 columns"></div>
    <div class="medium-6 columns"></div>
</div>
~~~
Foundation栅格的行用.row表示，而列由至少两个类名组成，一是.columns或.column（2种写法完全相同，单纯为了支持语法偏好）表明这是列元素，二是.medium-6这种用于表示断点类型和对应宽度。在默认情况下，Foundation栅格的断点类型从小到大依次是small、medium、large，其中small类似Bootstrap栅格的xs，也是指任意屏幕尺寸下都水平排列。

Foundation栅格没有container，只需要row和column，因此显得比Bootstrap栅格更简单一些。其中row定义了最大宽度（可以认为承担了container的部分功能），column定义了0.9375rem的水平内边距。如果要嵌套，仍然是column内续接row，再继续接column。

组合使用多个断点类型，其方法也和Bootstrap栅格相同。需要注意的是，Foundation栅格的断点值是用的em而不是px，对应的，它们转换后的像素值也有别于Bootstrap栅格。

#### Block Grid

作为栅格系统的补充，Foundation还提供了另外一个叫做Block Grid的栅格。不过，它并不是一个超出传统栅格的新东西，而只是一个针对特定栅格应用场景的方法糖。

> 摘选自：[有关css栅格系统的故事](http://acgtofe.com/posts/2015/07/a-story-of-grid)
