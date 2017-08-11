---
layout: post
title:  gradle 学习2——生成指定文件名的apk
category: accumulation
tags: gradle
keywords: Android,gradle
banner: http://obxk8w81b.bkt.clouddn.com/Cart%20with%20Red%20and%20White%20Ox.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Cart%20with%20Red%20and%20White%20Ox.jpg
---

上一篇[Gradle学习]()了解了Gradle的基本结构和依赖关系，这篇文章介绍下怎么在Android上使用Gradle生成指定包名的APK

### 需求场景
在我们Android开发基本进入测试阶段后，会根据后台的不同环境，打包出对应不同环境（如SIT、UAT、PRD等）的APK安装包。

每次去手动修改Java类改变后台环境太麻烦还容易忘记修改。打包成功后，还需要手动将默认的包名app-release.apk改成我们需要的包名，也是非常麻烦，有没有直接指定环境，生成指定文件名的安装包呢？

方法是有的，就是利用gradle的分渠道打包特性，下面上代码分别解释。

> 主要在这两个个地方修改：
- buildType（构建类型）
- productFlavors （不同定制的产品）

<!--more-->
#### buildType（构建类型）介绍

默认情况下，Android Plugin会自动给项目设置同时构建应用程序的debug和release版本。 两个版本之间的不同主要围绕着能否在一个安全设备上调试，以及APK如何签名。

可以创建一个新的构建类型就是简单的在buildType标签下添加一个新的元素，并且可以使用initWith()或者直接使用闭包来配置它。

以下是一些可能使用到的属性和默认值：

|*Property name	*|*Default values for debug	*|*Default values for release / other*|
|:--------:|:-------:|:--------:|
|debuggable	            | true	|false |
|jniDebugBuild	        | false	|false|
|renderscriptDebugBuild	| false	|false|
|renderscriptOptimLevel	|   3	|3|
|applicationIdSuffix	|  null	|null|
|versionNameSuffix	    |  null	|null|
|signingConfig	  |android.signingConfigs.debug	|null|
|zipAlign	             |false	|true|
|runProguard	         |false	|false|
|proguardFile	     |N/A (set only)	|N/A (set only)|
|proguardFiles	     |N/A (set only)	|N/A (set only)|

上面介绍摘选自[Gralde plugin User Guide 中文版](https://avatarqing.gitbooks.io/gradlepluginuserguidechineseverision/content/basic_project/build_types.html)


#### Product flavors（不同定制的产品）介绍
一个product flavor定义了从项目中构建了一个应用的自定义版本。一个单一的项目可以同时定义多个不同的flavor来改变应用的输出。

这个新的设计概念是为了解决不同的版本之间的差异非常小的情况。虽然最项目终生成了多个定制的版本，但是它们本质上都是同一个应用，那么这种做法可能是比使用库项目更好的实现方式。

Product flavor需要在productFlavors这个DSL容器中声明：

~~~ Java
android {
    ....

    productFlavors {
        flavor1 {
            ...
        }

        flavor2 {
            ...
        }
    }
}
~~~
这里创建了两个flavor，名为flavor1和flavor2。

> 注意：flavor的命名不能与已存在的Build Type或者androidTest这个sourceSet有冲突。

#### Build Type + Product Flavor = Build Variant（构建类型+定制产品=构建变种版本）
正如前面章节所提到的，每一个Build Type都会生成一个新的APK。

Product Flavor同样也会做这些事情：项目的输出将会拼接所有可能的Build Type和Product Flavor（如果有Flavor定义存在的话）的组合。

每一种组合（包含Build Type和Product Flavor）就是一个Build Variant（构建变种版本）。

例如，在上面的Flavor声明例子中与默认的debug和release两个Build Type将会生成4个Build Variant：

- Flavor1 - debug
- Flavor1 - release
- Flavor2 - debug
- Flavor2 - release

项目中如果没有定义flavor同样也会有Build Variant，只是使用的是默认的flavor和配置。default(默认)的flavor/config是没有名字的，所以生成的Build Variant列表看起来就跟Build Type列表一样。

### 具体使用
我们的需求就要用到**Build Type**和**Product Flavor**
我在项目中**Build Type**的使用，先看代码：

~~~ Java
buildTypes {
        release {
            buildConfigField "boolean", "LEO_DEBUG", "false"
            minifyEnabled true
            proguardFiles getDefaultProguardFile('proguard-android.txt'), 'proguard-rules.txt'
            debuggable false
            jniDebuggable false
        }
        debug {
            buildConfigField "int", "buildType", "2"
            buildConfigField "boolean", "LEO_DEBUG", "true"
        }
    }
~~~
和**Product Flavor**的使用

~~~ Java
productFlavors{
	kaifa{
		buildConfigField "int", "buildType", "1"
	}
	ceshi {
		buildConfigField "int", "buildType", "2"
	}
	SIT {
		buildConfigField "int", "buildType", "3"
	}
	UAT {
		buildConfigField "int", "buildType", "4"
	}
	PRD {
		buildConfigField "int", "buildType", "5"
	}
}
~~~
每一个构建过的productFlavors都会在**app->build->generated->source->buildConfig**目录下创建自己对应的一个flavor目录，如下图：

![Gradle build structure](http://oui2w5whj.bkt.clouddn.com/blogimages/2016/gradle_structure2.png)

而，buildConfigField会在指定的目录，**flavor->release/debug->包名**，下生成一个BuildConfig.java文件，我的代码在BuildConfig.java中定义了一个int类型的buildType和boolean类型的LEO_DEBUG。要使用这两个字段只需要:

~~~ Java
if (BuildConfig.LEO_DEBUG){
		//用来控制本地Log日志
}
if (Environment ==BuildConfig.buildType){
		//用来控制开发环境
}
~~~

#### gradle中修改apk生成名字的方法
这个方法是定义在productFlavors同一层级的

~~~ Java
android.applicationVariants.all { variant ->
        variant.outputs.each { output ->
            def outputFile = output.outputFile
            if (outputFile != null && outputFile.name.endsWith('.apk')) {
                //这里修改apk文件名
                def flavorname =variant.productFlavors[0].name
                if (flavorname.equals('kaifa'))
                    flavorname = 'debug'
                else if (flavorname.equals('ceshi'))
                    flavorname = 'release'

                def fileName = "app-${flavorname}-${defaultConfig.versionName}-${releaseTime() }.apk"
                output.outputFile = new File(outputFile.parent, fileName)
            }
        }
    }
~~~

这个方法要定义在最外层
~~~ Java
def releaseTime() {
    return new Date().format("MMdd", TimeZone.getTimeZone("UTC"))
}
~~~

最后生成的apk名字
