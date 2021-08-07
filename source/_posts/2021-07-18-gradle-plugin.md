---
layout: post
title: 自定义gradle插件
category: accumulation
tags:
    - gradle
keywords: Android,gradle
banner: https://cdn.conorlee.top/Gardens%20on%20Montmartre%20and%20the%20Blute-Fin%20Windmill.jpg
thumbnail: https://cdn.conorlee.top/Gardens%20on%20Montmartre%20and%20the%20Blute-Fin%20Windmill.jpg
toc: true
---
### gradle和gradlew的区别

如果配置好了 gradle 环境变量后,就可以在cmd中使用gradle命令了
在cmd中输入： gradle -v
<!--more-->
输出如下:
~~~ c
------------------------------------------------------------
Gradle 5.6.4
------------------------------------------------------------
~~~
但是有时候没有配置环境变量，却在Android项目根目录使用 gradlew 命令
~~~ c
./gradlew -v 
~~~
输出的版本和gradle-wrapper.properties文件中的版本一致

其实 gradlew是 gradle wrapper 的简写。Gradle wrapper 是Gradle的核心特性，能够让机器在没有安装Gradle运行时的情况下运行Gradle构建，也可以让构建脚本运行在一个指定的Gradle版本上。
使用Wrapper被认为是最佳实践，这样在不同的机器上面，构建的版本就能够保证统一。使用了包装器的Gradle脚本非常适合作为自动化发布的一部分，比如持续集成。

我们每个Android项目的根目录下都有一个 gradlew的可执行文件，./gradlew 正是访问的这个文件

### Gradle构建块
每个Gradle构建都包含三个基本构建块：project、task、property。每个构建至少包含一个project，进而包含一个或者多个task。project和task暴露的属性可以用来控制构建。

Gradle使用的是领域驱动设计（DDD）的原理为其自己的领域构建软件建模。因此，在Gradle API中有相应的类来表示project和task。

Gradle API中有相应的类来表示project和task。这一点是要明确的，Gradle中的脚本实际上是转为代码来执行的。

#### 项目Project
在Gradle术语中，一个项目（project）代表一个正在构建的组件，比如一个JAR文件，或一个想要完成的目标，如部署应用程序。
当构建进程启动后，Gradle基于build.gradle中的配置实例化org.gradle.api.Project接口，并且能够通过project变量使其隐式可用。

关于 Project 的介绍可以看这里: https://www.jianshu.com/p/434eba22561e
#### 任务Task
任务动作 task action，定义了一个当任务执行时最小的工作单元。
任务依赖 task dependency，很多时候运行一个task之前需要运行另一个task。
Gradle task对应的API是org.gradle.api.Task 接口。

关于 task更多的介绍可以看这里 https://www.jianshu.com/p/60bf794cdf91

#### 属性 Property
每个Project和Task实例都提供了可以通过getter和setter方法访问的属性。一个属性可能是一个任务的描述或者项目的版本。
你也可以定义自己的属性。Gradle允许用户通过扩展属性自定义一些标量

### Gradle 插件分类
Gradle插件分为两类：

#### 脚本插件，是一个普通的Gradle构建脚本，它可以被导入到其它的构建脚本中。
对象插件，需要实现org.gradle.api.Plugin接口。
1 使用脚本插件
假设我们有一个脚本名为 sayhello.gradle ，它里面有一个task：
~~~ groovy
task sayHello << {
    println 'hello world'
}
~~~
然后我们在build.gradle 中依赖这个外部脚本，调用Project的apply方法，apply方法调用时传入from属性，它的值可以是任何类型的URL，比如HTTP地址。
~~~ groovy
apply from: 'sayhello.gradle'
~~~
此时我们运行gradle sayHello 命令，可以看到sayHello 这个task执行了：
~~~ groovy
:sayHello
hello world
~~~
通过apply from的方式引入脚本插件，就好像那个脚本插件中的内容是写在当前的build.gradle中一样。它是使用比较简单，这里就不多做介绍了。

#### 对象插件
我们之前讲了自定义的task，它的实现逻辑是一种可维护、可测试的解决方案。通过打包成jar文件，task可以在独立的项目中被重用。然而，该方式仍有一些限制。
优点：

- 定制逻辑在类中是自包含的，并且可以通过增强型task配置
- 通过将task属性用注解标志可以支持声明式增量构建
- 自定义的task可以测试
缺点：

- 自定义的task仅仅暴露独立的工作单元。所提供的额外的公式化代码、约定和生命周期的整合并不是很直接
- 自定义的task仅仅能通过增强型task来配置。通过自定义的DSL，缺乏有表达性的扩展机制
- 其他插件的功能不容易使用或扩展

而使用对象插件的方式，可以给你最大的灵活性去封装高复杂度的逻辑，并且提供一种强大的扩展机制可以在构建脚本中定制它的行为。比如编译Java代码，我们会通过一句话来引入Java插件：
~~~ grovvy
apply plugin :'java'
~~~
然后我们还可以配置很多属性来控制编译逻辑。这个Java插件就是一个对象插件。

### 对象插件的实现方式
对于实现一个对象插件，有4个基本元素是非常重要的。

- 在放置插件实现的位置方面Gradle给你完全的灵活性。代码可以放在构建脚本中或者buildSrc目录下，也可以作为一个独立的工程被开发并且以jar文件方式发布。
- 每个插件都需要提供一个实现类，它代表着插件的入口点。插件可以用任何JVM语言编写并编译成字节码。
- 应用到项目中的插件可以通过暴露出来的扩展对象进行定制。如果用户想要在构建脚本中覆盖插件的默认配置时，这一点特别有用。
- 插件描述符是一个属性文件，它包含了关于插件的元信息。通常，它包含有插件的简短名字和插件实现类的映射。

### 编写插件
编写一个插件最低的要求是提供`org.gradle.api.Plugin<T>`接口的一个实现类。该接口仅仅定义了一个简单的方法：`apply(T target)`。

现在我们来演示自定义一个插件，这个插件的作用是提供一个task来打印用户凭证。
我们前面已经提到，自定义对象插件和自定义task方式类似，代码可以放在构建脚本中或者buildSrc目录下，也可以作为一个独立的工程被开发并且以JAR文件方式发布，下面我们根据这三种方式来分别演示如何自定义和使用插件。

#### 放在构建脚本中
在build.gradle中，代码如下：
~~~ java
apply plugin:CredentialPlugin

userCredential{
    username='admin'
    password='000000'
}

class CredentialPlugin implements Plugin<Project>{
    @Override
    void apply(Project project){
        project.extensions.create('userCredential',CredentialExtension)
        project.tasks.create('printUserCredential') << {
             println "username is: " + project.userCredential.username
             println "password is: " + project.userCredential.password
        }
    }
}

//扩展模型
class CredentialExtension {
    String username
    String password
}
~~~

Gradle将语言结构化模型作为扩展，扩展可以被添加到许多Gradle对象中，如果一个类实现了org.gradle.api.plugins.ExtensionAware接口，比如Project或者Task，就认为它是扩展可知的。每个扩展都是一个数据模型，它是扩展的基础。这个模型可以是一个POJO或者Groovy Bean。

在上面的代码中，userCredential闭包中的内容，可以从构建脚本中给task提供所需要的属性值，这个userCredential就是我们暴露的一个DSL。
ExtensionAware对象有一个方法getExtensions()，该方法返回一个ExtensionContainer对象，ExtensionContainer对象可以通过create()方法来注册我们的扩展，也就是把我们配置的DSL和具体的类关联起来。在本例中就是把userCredential这个闭包和CredentialExtension这个类关联起来，这就是插件的扩展机制。

> **扩展对象vs额外属性** 被用来扩展一个对象的DSL的扩展是扩展可知的，一个已注册的扩展模型会暴露一些属性和方法，用来给构建脚本建立新的构建语言结构，这些属性名和方法在创建的时候已经定好。扩展模型的典型用例是插件。额外属性，是一些通过ext命名空间创建的简单变量，它们一般提供给用户空间也就是构建脚本使用，额外属性的属性名是可以任意指定的。请尽量避免在插件实现中使用额外属性。

4.2 放在buildSrc目录下
与自定义task一样，Groovy代码放在`buildSrc/src/main/groovy` 目录下，
~~~ java
package com.sososeen.credential

import org.gradle.api.Plugin
import org.gradle.api.Project

class CredentialPlugin implements Plugin<Project>{
    @Override
    void apply(Project project){
        ...
    }
}


package com.sososeen.credential
class CredentialExtension {
    ...
}
~~~
在与buildSrc同级的目录下，创建build.gradle脚本，引入这个plugin：
~~~ java
apply plugin:com.sososeen.credential.CredentialPlugin
userCredential{
    username='admin'
    password='000000'
}
~~~
然后执行 gradle printUserCredential 命令，可以看到打印结果：
~~~ java
:printUserCredential
username is: admin
password is: 000000
~~~
可以看到，我们引入这个插件的时候是把它的包名带类名写上了，这显得太长了，不好写。我们可以给这个插件一个有意义且精简的名字。在src/main/resources/META-INF/gradle-plugins目录下，我们可以创建一个属性文件来配置。比如创建一个credentials.properties，它就是一个插件描述符，暴露了插件名字是credentials。在这个文件中，将该插件类的全局类名赋值给键implemention-class，如下：
~~~ java
implementation-class = com.sososeen.credential.CredentialPlugin
~~~
然后应用插件就可以这样子：
~~~ java
apply plugin : 'credentials'
~~~
执行 gradle printUserCredential 命令，可以看到打印结果与之前一样。

以jar文件形式提供插件
这个步骤与自定义task打包为jar文件一样，新建一个项目，把buildSrc目录下的文件复制过来，同时，在该项目下创建一个build.gradle文件。
~~~ java
apply plugin: 'groovy' //应用这个插件来编译Groovy代码
apply plugin: 'maven'

version = '1.0'
group = 'com.sososeen09'
archivesBaseName = 'credential'

repositories {
    mavenCentral()
}

dependencies {
    // 使用Gradle中的API需要这个
    compile gradleApi()
}

uploadArchives {
    repositories {
        mavenDeployer {
            repository(url: "file:../lib")
        }
    }
}
~~~
执行gradle uploadArchives命令后就可以看到，在与当前项目同级的lib文件目录中生成了我们期望的jar文件。

我们再新建一个工程，这个工程中有一个脚本文件build.gradle:
~~~ java
apply plugin : 'credentials'
buildscript {
    repositories {
        maven {
            url 'file:../lib'
        }

    }

    dependencies {
        classpath 'com.sososeen09:credential:1.0'
    }
}

userCredential{
    username='admin'
    password='000000'
}
~~~
执行 gradle printUserCredential 命令，可以看到打印结果与之前一样。


### REF 

https://www.jianshu.com/p/158851436e82