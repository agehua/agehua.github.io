---
layout: post
title:  跨平台桌面程序开发
category: accumulation
tags:
    - Alfred Workflow
    - Electron
    - JavaScript
keywords: Electron, JavaScript
banner: http://obxk8w81b.bkt.clouddn.com/Digger%203.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Digger%203.jpg
toc: true
---

## 跨平台桌面程序开发

今天搜索Alfred 英文词典应用时发现了这个工具：
[Youdao Alfred Workflow](https://github.com/kaiye/workflows-youdao/)

作者在这篇文章：[学英语和写工具](https://github.com/kaiye/kaiye.github.com/issues/5)，里面介绍了Alfred插件开发，同时也介绍了一下跨平台插件开发的方式：

<!--more-->
下面这段摘自这篇文章：
> 从趋势上来看，基于 HTML5 API 和 Node.js 的跨平台插件开发是未来的主流方向。  
   - Chrome App 。从 Github 上下载一些 chrome app sample 示例，简单浏览一下教程（墙内的、墙外的）和官方 API 文档 ，不出半天时间就能开发出一款插件。
   - npm 命令行工具。npm 模块除了在 node 环境下作为依赖模块被引用以外，还可以直接作为命令行工具使用。通过 package.json bin 字段定义，在全局安装时即可自动注册为全局命令行。例如，这款用于生成字体的 makefont 命令行模块。
   - [NW.js](https://nwjs.io/) 。将 webkit 和 node 集成打包使用的跨平台方案。当前 Mac 版的「夺命追魂钉」用的就是这个方案。node + webkit 方案最终将会取代 Adobe AIR ，这是 HTML5 对 Flash 的胜利。
   - [Electron](http://electron.atom.io/) 。微软新出的 Visual Studio Code 编辑器和 Github 的 Atom 编辑器使用的内核引擎，与 NW.js 的区别参看这里 。
   
     对于程序员来说，这是一个非常好的全栈实践之路。


[Visual Studio Code 配置指南](https://github.com/kaiye/kaiye.github.com/issues/14)

Visual Studio Code和Atom竟然是用Electron开发出来的，顿时觉得这种技术好强大，下决心要学习一下这个技术。

### 关于 Electron

Electron的官方网址是：https://electronjs.org/
> 如果你可以建一个网站，你就可以建一个桌面应用程序。 Electron 是一个使用 JavaScript, HTML 和 CSS 等 Web 技术创建原生程序的框架，它负责比较难搞的部分，你只需把精力放在你的应用的核心上即可。

Electron（最初名为Atom Shell[3]）是GitHub开发的一个开源框架。它允许使用Node.js（作为后端）和Chromium（作为前端）完成桌面GUI应用程序的开发。Electron现已被多个开源Web应用程序用于前端与后端的开发，著名项目包括GitHub的Atom和微软的Visual Studio Code。

入门教程有：
[Electron W3C中文文档](https://www.w3cschool.cn/electronmanual/)
[Github: electron-cn-docs](https://github.com/amhoho/electron-cn-docs)

由于Electron基于Node.js，所以入门第一关应当稍微熟悉Node.js的文档.

### Electron

**主进程**

在 Electron 里，运行 package.json 里 main 脚本的进程被称为主进程。在主进程运行的脚本可以以创建 web 页面的形式展示 GUI。
**渲染进程**

由于 Electron 使用 Chromium 来展示页面，所以 Chromium 的多进程结构也被充分利用。每个 Electron 的页面都在运行着自己的进程，这样的进程我们称之为渲染进程。
在一般浏览器中，网页通常会在沙盒环境下运行，并且不允许访问原生资源。然而，Electron 用户拥有在网页中调用 io.js 的 APIs 的能力，可以与底层操作系统直接交互。

**主进程与渲染进程的区别**

主进程使用 BrowserWindow 实例创建网页。每个 BrowserWindow 实例都在自己的渲染进程里运行着一个网页。当一个 BrowserWindow 实例被销毁后，相应的渲染进程也会被终止。
主进程管理所有页面和与之对应的渲染进程。每个渲染进程都是相互独立的，并且只关心他们自己的网页。
由于在网页里管理原生 GUI 资源是非常危险而且容易造成资源泄露，所以在网页面调用 GUI 相关的 APIs 是不被允许的。如果你想在网页里使用 GUI 操作，其对应的渲染进程必须与主进程进行通讯，请求主进程进行相关的 GUI 操作。
在 Electron，我们提供用于在主进程与渲染进程之间通讯的 ipc 模块。并且也有一个远程进程调用风格的通讯模块 remote。


