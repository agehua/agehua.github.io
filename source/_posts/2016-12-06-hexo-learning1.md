---
layout: post
title:  Hexo 模板学习——Hexo 结构
category: accumulation
tags: Hexo
keywords: Hexo结构
banner: http://obxk8w81b.bkt.clouddn.com/Boy%20Cutting%20Grass%20with%20a%20Sickle.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Boy%20Cutting%20Grass%20with%20a%20Sickle.jpg
---

### 起步
Hexo会读取根目录下的_config.yml里面的theme属性, 从而采用对应的主题。而主题都是放在themes/目录下面的，然后你会发现他里面有个landscape的目录，这个就是默认主题啦。

接着，我们查看themes/landscape/目录，以及根据文档docs-themes，我们很容易得出：一个主题其实由4部分组成。

- _config.yml: 主题的配置文件
- source/: 放我们的CSS文件以及图片
- layout/: 模板文件
- scripts: 放JavaScript文件，他们会自动加载

根据文档[docs-templates](https://hexo.io/docs/templates.html)，下面表格中每个页面都有一个可用的模板，可以没有这些模板，但至少要有一个index模板。

<!--more-->

|Template|Page|	Fallback|
|:--------:|:-------:|:--------:|
|index	|Home page | |
|post	|Posts	|index|
|page|	Pages |index|
|archive	|Archives|	index|
|category	|Category archives	|archive|
|tag	|Tag archives|	archive|



每次当我们在浏览器访问时，Hexo都会去解析sources目录下对应的模板文件。不同的URL对应不同的文件，所以才有了不同的页面。那么，我们怎么知道哪个URL对应哪个页面呢？(下面我们以EJS为例)

无论URL是什么，Hexo先读取layout.ejs，然后里面的body变量会替换成上面表格里的模板：(Fallback的意思是如果访问/archives时，我们的archives.ejs不存在的话，就会返回index.ejs)


### 布局
#### 编写布局文件(layout.ejs)

模板文件在layout文件夹下，文件名对应Hexo中的模板名，有index,post,page,archive,category,tag几种，对于普通的header + content + footer的页面结构，header和footer往往是可以复用的，因此我们可以使用layout.ejs进行布局，动态的内容使用body变量去动态渲染，所以我的layout.ejs大概长这样:

~~~ JavaScript
<!doctype html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, user-scalable=no"/>
    <title><%= config.title %></title>
    <%- css('css/style') %>
</head>
<body>
    <%- partial('_partial/header') %>
    <div class="main">
        <%- body %>
    </div>
    <%- partial('_partial/footer') %>
    <%- js('js/index.js') %>
</body>
</html>
~~~

partial,js和css是Hexo提供的辅助函数，后面再说。

#### 其他模板文件

每一个模板文件对应的是一种布局，当你使用hexo new <title>的时候，其实忽略了一个参数，完整的命令是hexo new [layout] <title>，这个layout就决定了文章使用何种方式布局，比如创建一个自己简介的About页面，hexo new page "about"其实就是使用了page布局。每种布局对应到我们的模板文件上就是index.ejs(首页),post.ejs(文章),archive.ejs(归档),tag.ejs(标签归档),page.ejs(分页)。

##### index.ejs

首页一般是一些博文的摘要和一个分页器，通过Hexo的page变量拿到页面的数据渲染即可，这里我们不直接在index.ejs中写HTML结构，新建一个_partial/article.ejs，将文章数据传给子模板渲染，然后再额外传入一个参数{index: true}，对后面的post.ejs和page.ejs加以区分，让子模板能正确渲染。最后，index.ejs大致是这样的:

~~~ JavaScript
//index.ejs
<% page.posts.each(function(post, index){ %>
    <%- partial('_partial/article', {index: true, post: post}) %>
<% }) %>
<div class="pagination">
    <%- paginator({ total: Math.ceil(site.posts.length / config.per_page)}) %>
</div>
~~~
##### post.ejs

文章模板和首页差不多，只是对应的是一篇具体的文章，所以就把文章传入，再额外传入{index: false}告诉子模板不要按首页的方式去渲染就好了。就一行代码(因为都在子模板里 XD

~~~ JavaScript
//post.ejs
<%- partial('_partial/article', {index: false, post: page}) %>
~~~
##### page.ejs

我个人对Page模板其实是有点懵逼的，在我自己的实践中是添加about(hexo new page "about")页面后，访问/about会走分页布局，实际上这个页面对应的内容是/source/about里的index.md，也相当于对文章的渲染，因此我把Page模板也写成了和文章模板一样:

~~~ JavaScript
//page.ejs
<%- partial('_partial/article', {index: false, post: page}) %>
_partial/article.ejs
~~~
前面一共有三处共用了article模板，另外page和post的一样的，所以实际上只有两种情况:主页(index: true)和非主页(index: false)。对应的_partial/article.ejs里只要判断这个值就可以正确渲染了，基本结构如下：

~~~ JavaScript
//_partial/article.ejs
<% if(index){ %>
    //index logic...
<% }else{ %>
    //post or page logic...
<% } %>
~~~

##### tag.ejs

标签归档页内容很少，直接用Hexo的辅助函数list_tags生成一个标签的列表就ok了:

~~~ JavaScript
//tag.ejs
<%- list_tags() %>
~~~
归档页模板和首页差不多，归档页只需要展示文章标题和最后的分页器就好:

~~~ JavaScript
//archive.ejs
<div class="archive">
  <% var lastyear; %>
  <% page.posts.each(function(post){ %>
    <% var year = post.date.year() %>
    <% if(lastyear !== year){ %>
      <h4 class="year"><%= year %></h4>
      <% lastyear = year %>
    <% } %>
    <div class="archive_item">
      <a class="title" href="<%- url_for(post.path) %>"><%= post.title %></a>
      <span class="date"><%= post.date.format('YYYY-MM-DD') %></span>
    </div>
  <% }) %>
  <div class="pagination">
    <%- paginator({ total: Math.ceil(site.posts.length / config.per_page)}) %>
  </div>
</div>
~~~
至此，模板文件就写好了，对于category模板就放弃了，感觉比较鸡肋。。。

### 变量
其实在模板文件中我们已经看到了page.post,site.posts.length,config.per_page等等，页面的内容就是根据这些变量获取的，由Hexo提供，拿来直接用，Hexo提供了很多变量，但不是都很常用，一般就用到以下变量:

- site: 对应整个网站的变量，一般会用到site.posts.length制作分页器

- page: 对应当前页面的信息，例如我在index.ejs中使用page.posts获取了当前页面的所有文章而不是使用site.posts。

- config: 博客的配置信息，博客根目录下的_config.yml。

- theme: 主题的配置信息，对于主题根目录下的_config.yml。

#### 辅助函数(Helper)
制作一个分页器，我们需要知道文章的总数和每页展示的文章数，然后通过循环生成每个link标签，还要根据当前页面判断link标签的active状态，但是在Hexo中这些都不用我们自己来做了!Hexo提供了**paginator**这一辅助函数帮助我们生成分页器，只需要将文章总数site.posts.length和每页文章数config.per_page传入就可以生成了。

##### 其他的Helper:

- **list_tags([options])**: 快速生成标签列表

- **js(path/to/js), css(path/to/css)** 用来载入静态资源，path可以是字符串或数组(载入多个资源)，默认会去source文件夹下去找。

- **partial(path/to/partial)** 引用字模板，默认会去layout文件夹下找。

### 样式
知道了Hexo的渲染方式，我们就可以使用HTML标签+CSS样式个性化我们的主题了，推荐大家使用CSS预处理语言的一种来写样式，这样就可以通过预处理语言自身的特点让样式更灵活。

### 其他
#### 添加对多说和Disqus的支持

评论是很常用的功能，不如就直接在我们的主题里支持了，然后通过配置变量决定是否开启，评论区跟在文章内容下面，对于这种三方的代码块，最好也以partial的方式提取出来，方便移除或是替换。

~~~ JavaScript
//_partial/article.ejs
<section class='post-content'>
    <%- post.content %>
</section>
//评论部分，post.comments判断是否开启评论，config.duoshuo_shortname
和config.disqus_shortname来判断启用那种评论插件，这里优先判断了多说
<% if(post.comments){ %>
    <section id="comments">
    <% if (config.duoshuo_shortname){ %>
            <%- partial('_partial/duoshuo') %>
        <% }else if(config.disqus_shortname){ %>
            <%- partial('_partial/disqus') %>
        <% } %>
    </section>
<% } %>
~~~
再将多说和Disqus提供的js脚本代码放在**_partial/duoshuo.ejs和_partial/disqus.ejs**下就ok了~

#### 使用highlight.js提供代码高亮

highlight.js提供了多种语言的支持和多种皮肤，用法也很简单，载入文件后调用初始化方法，一切都帮你搞定，对于使用那种皮肤，喜好因人而异，我们干脆在主题的配置文件中做成配置项让用户自己选择:

~~~ JavaScript
//showonne/_config.yml

...other configs

# highlight.js
highlight_theme: zenburn
~~~
对应的layout.ejs中:

~~~ JavaScript
<link rel="stylesheet" href="//cdnjs.cloudflare.com/ajax/libs/highlight.js/9.4.0/styles/<%= theme.highlight_theme %>.min.css">
~~~
样式文件通过CDN引入，因为不同皮肤对应不同的文件名，所以十分灵活。

### 最后
当初是对应着landscape照葫芦画瓢写的，最近回头来发现一些不合理的地方，所以就又改了改，也对应着写了这么一篇总结，接下来准备再把样式划分一下，对于颜色这类样式通过变量的方式提取出来，也变得可配置，能让主题更灵活一些。

### 参考资源
