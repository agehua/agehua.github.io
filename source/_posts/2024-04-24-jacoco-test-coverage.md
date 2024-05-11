---
layout: post
title: 代码覆盖率统计——集成Jacoco
category: accumulation
tags:
  - Jacoco
keywords: Jacoco
banner: https://cdn.conorlee.top/Irises.jpg
thumbnail: https://cdn.conorlee.top/Irises.jpg
toc: true
---

### 代码覆盖率
代码覆盖率是软件测试中一个重要的评价指标。主要是指程序运行过程中，被执行到的代码在总代码中的占比程度，现在有很多插件可以实现这个功能，应用比较广的就是jacoco。

代码覆盖率从测试方式上有两个方向可以进行：
- 单元测试（白盒测试）
- 功能测试（黑盒测试）

下面在Android工程中，分别用Jacoco来实现这两个方向的代码覆盖率统计：

<!--more-->

### 单元测试
单元测试代码覆盖率
对于单元测试（白盒测试），gradle编译工具已经集成了JaCoCo插件，只需要添加这个插件，然后在工具栏 Run “YourUnitTestCase” with Coverage 即可，执行完用例之后，在IDE右侧会自动显示覆盖率报告，也可以单独建一个gradle task来生成jacoco覆盖率报告。

> 需要编写单元测试代码

使用下面的gradle task来生成单元测试的报告
~~~ java
// 源代码路径
def coverageSourceDirs = [
        '../app/src/main/java'
]
task jacocoTestReport(type: JacocoReport) {
    group = "Reporting"
    description = "Generate Jacoco coverage reports after running tests."
    reports {
        xml.enabled = true
        html.enabled = true
    }
    // 字节码路径
    classDirectories.from = fileTree(
            dir: './build/intermediates/javac/debug',
            excludes: ['**/R*.class',
                       '**/*$InjectAdapter.class',
                       '**/*$ModuleAdapter.class',
                       '**/*$ViewInjector*.class'
            ])
    sourceDirectories.from = files(coverageSourceDirs)
    // 覆盖率数据路径，这个数据是执行单测时自动生成的
    executionData.from = files("$buildDir/outputs/unit_test_code_coverage/debugUnitTest/testDebugUnitTest.exec")

    doFirst {
        new File("$buildDir/intermediates/javac/").eachFileRecurse { file ->
            if (file.name.contains('$$')) {
                file.renameTo(file.path.replace('$$', '$'))
            }
        }
    }
}
~~~
### 黑盒测试代码覆盖率
黑盒测试，也叫功能测试，也就是把app装在手机上，人工（或自动化）去操作UI界面。这种测试的代码覆盖率，用JaCoCo也可以统计到。

常见获取覆盖率的方法分两种，一种是在源码中，以打桩的形式，收集覆盖率数据，针对性较强，但是需要深入源码，另一种是改写应用入口，通过instrument启动应用，记录应用执行期间全局的代码覆盖率。

大致分为三个步骤：

使用gradle编译apk、aar、jar时，添加jacoco插件（记得只在buildType为debug时开启），完成插桩。
apk/aar/jar运行期间，在指定时机（比如OnDestroy时或者用户点击按钮）利用反射机制调用 jacoco api 拿到覆盖率统计数据，然后保存到手机，如果做了覆盖率平台，可以上传到平台。
把覆盖率数据从手机pull下来，在gradle里面添加一个生成报告的任务（参考上面单元测试代码覆盖率报告生成）。如果是做成了平台，也可以调用 jacoco api 根据覆盖率数据、源码、字节码生成报告并展示。
下面请看例子：

调用下面代码来生成对应报告
~~~ java
fun generateEcFile(isNew: Boolean, generateCallback: ((Boolean) -> Unit)? = null) {
        var out: OutputStream? = null
        val dir = FacadeHelper.getApplicationContext().getExternalFilesDir(null).toString() + "/shareData/"
        val target = dir + File.separator + "coverage.ec"
        val mCoverageFilePath = File(target)
        try {
            val targetDir = File(dir)
            if (!targetDir.exists()) {
                targetDir.mkdirs()
            }
            if (isNew && mCoverageFilePath.exists()) {
                JaLog.d(TAG, "清除旧的ec文件")
                mCoverageFilePath.delete()
            }
            if (!mCoverageFilePath.exists()) {
                mCoverageFilePath.createNewFile()
            }
            out = FileOutputStream(mCoverageFilePath.path, true)
            val agent = Class.forName("org.jacoco.agent.rt.RT")
                .getMethod("getAgent")
                .invoke(null)
            if (agent != null) {
                out.write(
                    agent.javaClass.getMethod("getExecutionData", Boolean::class.javaPrimitiveType)
                        .invoke(agent, false) as ByteArray
                )
            }
            generateCallback?.invoke(true)
        } catch (e: Exception) {
            JaLog.d(TAG, e.toString())
            generateCallback?.invoke(false)
        } finally {
            try {
                out?.close()
            } catch (e: IOException) {
                JaLog.d(TAG, e.toString())
                generateCallback?.invoke(false)
            }
        }
    }
~~~

#### 支持多Module
源码在下面，主要实现步骤：
- 1.在App module中定义一个jacoco.gradle，引入jacoco插件
- 2.定义一个 `copyTask`，将class文件和source code文件复制到指定目录（这里复制到了 **/build/custom**下）
- 3.在 `copyTask`中遍历项目所有的 project，对每个 project执行复制操作
    - 复制文件方法参考： [Gradle Copy Task](https://docs.gradle.org/current/dsl/org.gradle.api.tasks.Copy.html)
- 4.`copyTask`每次在 `package${buildType}` 任务后执行，这样打好包后，文件也已复制完毕
    - 这里支持多种`buildType`构建，比如debug、staging等。但不支持release
- 5.从App拿到覆盖率数据，放到指定目录后，执行 `jacocoTestReport`，将在 **build/reports/** 目录下生成Html报告
- 6.最后，在app.gradle中定义开关，根据开关状态判断是否引入 jacoco.gradle

> 如果是由服务端生成最终报告，需要上传覆盖率数据和class文件。因为所有class文件都在一个目录，可以轻易将class文件打包为一个jar，将jar包传给服务端

~~~ java
// jacoco.gradle文件
apply plugin: 'jacoco'

//Jacoco 版本，本篇创建时最新版本
jacoco {
    toolVersion = "0.8.11"
}

//源代码存放路径
def sourceCodeDirs = "$buildDir/customJavaSources"
def javaClassDirs = "$buildDir/customJavaClasses"

def currentBuildType = null

tasks.register('copyTask') {
    group = "JacocoReport"


    def originalSourceDir = {
        Set<Project> projects = project.rootProject.subprojects
        List<String> tmpDir = new ArrayList<>(projects.size())
        projects.forEach {
//            println("当前project is ${it.name}")
            tmpDir.add("$it.projectDir/src/main/java")
        }
        tmpDir
    }

    def originalJavaClassDir =  {
        Set<Project> projects = project.rootProject.subprojects
        List<String> tmpDir = new ArrayList<>(projects.size())
        projects.forEach {
            def subBuildType = currentBuildType.toLowerCase()
            if (it.name != "app") {
                try {
                    def androidConfig = it.extensions.getByName('android')
                    def buildTypeExists = androidConfig.buildTypes.any { it.name.equalsIgnoreCase(subBuildType) }
                    if (!buildTypeExists) {
                        subBuildType = 'debug'
                    }
//                    println("当前 project(${it.name}) 的config ${androidConfig}，buildtype 是${subBuildType}")
                } catch (Exception e) {
                    subBuildType = 'debug'
//                    println("当前 project(${it.name}) 没有 'android' 扩展名: ${e.message}")
                }
            }

            tmpDir.add("$it.projectDir/build/intermediates/javac/${subBuildType}/classes")
        }
        tmpDir
    }

    def originalKotlinClassDir = {
        Set<Project> projects = project.rootProject.subprojects
        List<String> tmpDir = new ArrayList<>(projects.size())
        projects.forEach {
            def subBuildType = currentBuildType.toLowerCase()
            if (it.name != "app") {
                try {
                    def androidConfig = it.extensions.getByName('android')
                    def buildTypeExists = androidConfig.buildTypes.any { it.name.equalsIgnoreCase(subBuildType) }
                    if (!buildTypeExists) {
                        subBuildType = 'debug'
                    }
//                    println("当前 project(${it.name}) 的config ${androidConfig}，buildtype 是${subBuildType}")
                } catch (Exception e) {
                    subBuildType = 'debug'
//                    println("当前 project(${it.name}) 没有 'android' 扩展名: ${e.message}")
                }
            }
//            tmpDir.add("$it.projectDir/build/tmp/kotlin-classes/debug")
            // https://issuetracker.google.com/issues/161300933#comment13
            tmpDir.add("$it.projectDir/build/intermediates/asm_instrumented_project_classes/${subBuildType}")
        }
        tmpDir
    }

    doLast {
        if (!Boolean.parseBoolean(enableJacoco)) {
            println("开关未打开，不复制文件")
            return
        }
        println("开始删除文件")
        delete sourceCodeDirs
        delete javaClassDirs

        println("开始复制文件")
        // https://docs.gradle.org/current/dsl/org.gradle.api.tasks.Copy.html
        copy {
            from originalSourceDir
            // 目标目录
            into sourceCodeDirs
            duplicatesStrategy = DuplicatesStrategy.INCLUDE
            exclude '**/com/blelib/**', "**/com/alibaba/**", "**/com/bumptech/**", "**/com/chad/**",
                    '**/*_HiltComponents**'
            println("复制java源文件开始")
        }
        copy {
            from originalJavaClassDir
            // 目标目录
            into javaClassDirs
            duplicatesStrategy = DuplicatesStrategy.INCLUDE
            exclude '**/com/blelib/**', "**/com/alibaba/**", "**/com/bumptech/**", "**/com/chad/**",
                    '**/*_HiltComponents**'
            println("复制java class开始")
        }
        copy {
            from originalKotlinClassDir
            // 目标目录
            into javaClassDirs
            duplicatesStrategy = DuplicatesStrategy.INCLUDE
            exclude '**/com/blelib/**', "**/com/alibaba/**", "**/com/bumptech/**", "**/com/chad/**",
                    '**/*_HiltComponents**'
            println("复制Kotlin class开始")
        }

        println("复制文件完成")
    }
}

//初始化Jacoco Task
tasks.register('jacocoInit') {
    group = "JacocoReport"
    File file = new File("$buildDir/outputs/code-coverage/")
    if (!file.exists()) {
        file.mkdir()
    }
}

/**
 * 单元测试，生成报告task
 */
tasks.register('jacocoTestReport', JacocoReport) {
    group = "JacocoReport"
    description = "Generate Jacoco coverage reports after running tests."

    reports {
        xml.enabled = true
        html.enabled = true
    }
    def excludes = [
            '**_HiltComponents_**',
            '**/*databinding*/**',
            '**/*BindingImpl*',
            '**/*hilt_aggregated_deps*/**'
    ]
    classDirectories.from = files([fileTree(dir: javaClassDirs, excludes: excludes)])
    sourceDirectories.from = files([sourceCodeDirs])
//    sourceDirectories.from = files([fileTree(dir: sourceCodeDirs, excludes: excludes)]) // 这行报错
    // 将 coverage.ec文件提前复制到这里：adb pull /storage/emulated/0/Android/data/com.xxx.xxx/files/shareData/coverage.ec
    executionData.from = files("$buildDir/outputs/code-coverage/coverage.ec")
}

afterEvaluate {
    def taskNames = gradle.startParameter.taskNames

    taskNames.each { taskName ->
        println "Current taskName: $taskName"
        if (taskName.toLowerCase().contains("debug")) {
            currentBuildType = "Debug"
        } else if (taskName.toLowerCase().contains("staging")) {
            currentBuildType = "Staging"
        }
    }

    if (currentBuildType != null) {
        println "Current Build Type: $currentBuildType"
    } else {
        println "Could not determine current build type."
        return
    }

    def jacocoTestReport = tasks.findByName('copyTask')
    def merge = tasks.findByName("package${currentBuildType}")
    if (null != merge) {
        merge.finalizedBy(jacocoTestReport)
//        jacocoTestReport.dependsOn(merge)
    } else {
        println("任务未找到")
    }
}

~~~

~~~ java
// app.gradle，省略其他代码

// 是否开启代码分析
if (Boolean.parseBoolean(enableJacoco)) {
    apply from: 'jacoco.gradle'
}
~~~

#### Hilt支持
正常的kotlin生成的class文件路径：build/tmp/kotlin-classes/debug，如果使用这个路径，`@AndroidEntryPoint` 修饰的类的覆盖率测试一直是0。
需要替换为 build/intermediates/asm_instrumented_project_classes/debug 这个路径，参考：
https://issuetracker.google.com/issues/161300933#comment13

#### 拿到手机上的测试覆盖数据
执行adb命令就可以：
~~~ java
adb pull /storage/emulated/0/Android/data/com.xxx.xxx/files/shareData/coverage.ec
~~~

### 增量代码覆盖率
- 计算增量的最小单位是 方法，即使方法中有一行变更，那也算作方法变更
- 配合git，拿到相关变更方法
- 需要修改Jacoco的代码，因为Jacoco的方法注入为全量注入。

大概架构图
![test-coverage架构](/images/blogimages/2024/jacoco-test-flow.jpeg)



