### 为gitbook生成 summary文件
github地址：https://github.com/imfly/gitbook-summary

### 安装
~~~ Java
npm install -g gitbook-summary
~~~
简单使用：
~~~
cd /path/to/your/book/
book sm
~~~

### 导出文档
#### 导出为PDF

SUMMARY.md文件的同级目录执行gitbook pdf .命令进行导出PDF文件

> 如果想要自定义生成的pdf文件名称，可以使用gitbook pdf . ./xxxx.pdf命令。

#### 导出为epub
在SUMMARY.md文件的同级目录执行 **gitbook epub .** 命令进行导出epub文件

#### 导出为mobi
在SUMMARY.md文件的同级目录执行 **gitbook mobi .** 命令进行导出mobi文件