---
layout: post
title: java多线程和并发面试问答
category: accumulation
tags:
  - Java
  - multi-thread
  - Interview Knowledge
keywords: java, 多线程, 并发
description:
banner: http://obxk8w81b.bkt.clouddn.com/Daubigny%20s%20Garden%203.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Daubigny%20s%20Garden%203.jpg
---

本文基于**酷勤网关于java多线程和并发面试题的文**章，进行了少量的整理和补充。[原文在这](http://www.kuqin.com/shuoit/20140708/341091.html)。

------

以下是正文：

多线程和并发问题是Java技术面试中面试官比较喜欢问的问题之一。在这里，从面试的角度列出了大部分重要的问题，但是你仍然应该牢固的掌握Java多线程基础知识来对应日后碰到的问题。

<!--more-->

# Java多线程面试问题

### 1. 什么是进程和线程？
进程是具有一定独立功能的程序关于某个数据集合上的一次运行活动，进程是系统进行资源分配和调度的一个独立单位。线程是进程的一个实体，是CPU调度和分派的基本单位，它是比进程更小的能独立运行的基本单位。

线程自己基本上不拥有系统资源,只拥有一点在运行中必不可少的资源（如程序计数器，一组寄存器和栈），但是它可与同属一个进程的其他的线程共享进程所拥有的全部资源。一个线程可以创建和撤销另一个线程；同一个进程中的多个线程之间可以并发执行。相对进程而言，线程是一个更加接近于执行体的概念，它可以与同进程中的其他线程共享数据，但拥有自己的栈空间，拥有独立的执行序列。在串行程序基础上引入线程和进程是为了提高程序的并发度，从而提高程序运行效率和响应时间。

### 2. 进程和线程之间有什么不同？
一个进程是一个独立(self contained)的运行环境，它可以被看作一个程序或者一个应用。而线程是在进程中执行的一个任务。

- 简而言之,一个程序至少有一个进程,一个进程至少有一个线程。
- 线程的划分尺度小于进程，使得多线程程序的并发性高。
- 进程在执行过程中拥有独立的内存单元，而多个线程共享内存，极大地提高了程序的运行效率。
- 线程在执行过程中与进程还是有区别的。每个独立的线程有一个程序运行的入口、顺序执行序列和程序的出口。但线程不能够独立执行，必须依存在应用程序中，由应用程序提供多个线程执行控制。
- 从逻辑角度来看，多线程的意义在于一个应用程序中，有多个执行部分可以同时执行。但操作系统并没有将多个线程看做多个独立的应用，来实现进程的调度和管理以及资源分配。这就是进程和线程的重要区别。

> 形象的讲，进程就是一个项目组，每个程序员就是里面的线程呀！当然一个程序员也可以叫做一个项目组，对应的就是一个进程只有一个线程。公司里面的任务是分配给项目组级别的，干活的就是其中的程序员。总的意思就是，进程和线程没有什么区别。
吐槽的话：我的意思就是进程干不过来了，那就多开几个线程呀！from [JacobK](https://www.zhihu.com/question/21535820/answer/19120563)


### 3. 多线程编程的好处是什么？
- 在进程内创建、终止线程比创建、终止进程要快；
- 同一进程内的线程间切换比进程间的切换要快,尤其是用户级线程间的切换。

在多线程程序中，多个线程被并发的执行以提高程序的效率，CPU不会因为某个线程需要等待资源而进入空闲状态。多个线程共享堆内存(heap memory)，因此创建多个线程去执行一些任务会比创建多个进程更好。举个例子，Servlets比CGI更好，是因为Servlets支持多线程而CGI不支持。

### 4. 用户线程和守护线程有什么区别？
当我们在Java程序中创建一个线程，它就被称为用户线程。一个守护线程是在后台执行并且不会阻止JVM终止的线程。当没有用户线程在运行的时候，JVM关闭程序并且退出（与守护线程是否在运行没有关系）。一个守护线程创建的子线程依然是守护线程。**守护线程应用背景**：后台线程，比如可以收集某些系统状态的线程，发送email的线程，等不希望影响JVM的事情。

### 5. 我们如何创建一个线程？
有两种创建线程的方法：一是实现Runnable接口，然后将它传递给Thread的构造函数，创建一个Thread对象；二是直接继承Thread类。[Read more...](http://www.journaldev.com/1016/java-thread-example-extending-thread-class-and-implementing-runnable-interface)

### 6. 有哪些不同的线程生命周期？
当我们在Java程序中新建一个线程时，它的状态是New。当我们调用线程的start()方法时，状态被改变为Runnable。线程调度器会为Runnable线程池中的线程分配CPU时间并且讲它们的状态改变为Running。其他的线程状态还有Waiting，Blocked和Dead。[Read more...](http://www.journaldev.com/1044/life-cycle-of-thread-understanding-thread-states-in-java)

- **新建（new Thread）**：当创建Thread类的一个实例（对象）时，此线程进入新建状态（未被启动）。例如：`Thread  t1=new Thread()`;
- **就绪（runnable）**：线程已经被启动，正在等待被分配给CPU时间片，也就是说此时线程正在就绪队列中排队等候得到CPU资源。例如：`t1.start()`;
- **运行（running）**：线程获得CPU资源正在执行任务（run()方法），此时除非此线程自动放弃CPU资源或者有优先级更高的线程进入，线程将一直运行到结束。
- **死亡（dead）**：当线程执行完毕或被其它线程杀死，线程就进入死亡状态，这时线程不可能再进入就绪状态等待执行。
- **自然终止**：正常运行run()方法后终止
- **异常终止**：调用stop()方法让一个线程终止运行
- **堵塞（blocked）**：由于某种原因导致正在运行的线程让出CPU并暂停自己的执行，即进入堵塞状态。
- **正在睡眠**：用sleep(long t) 方法可使线程进入睡眠方式。一个睡眠着的线程在指定的时间过去可进入就绪状态。
- **正在等待**：调用wait()方法。（调用motify()方法回到就绪状态）
- **被另一个线程所阻塞**：调用suspend()方法。（调用resume()方法恢复）

### 7. 可以直接调用Thread类的run()方法么？
可以，但是如果我们调用了Thread的run()方法，它的行为就会和普通的方法一样，为了在新的线程中执行我们的代码，必须使用Thread。start()方法。

### 8. 如何让正在运行的线程暂停一段时间？
我们可以使用Thread类的Sleep()方法让线程暂停一段时间。需要注意的是，这并不会让线程终止，一旦从休眠中唤醒线程，线程的状态将会被改变为Runnable，并且根据线程调度，它将得到执行。

### 9. 你对线程优先级的理解是什么？
线程的优先级越高，那么就可以分占相对多的CPU时间片。每个进程都有相应的优先级，**线程优先级决定它何时运行和占用CPU时间**。最终的优先级共分32级。是从0到31的数值，称为 **基本优先级别**。OS调度的是线程，**真正具有优先级的是线程**，而进程优先级是作为一个优先级Class存在。一个线程创建的时候，会继承进程的优先级。 外线程优先级可以用SetThreadPriority来进行微调。常用的桌面系统，都是分时操调度，根据线程的优先级来分配调度时间。我们可以定义线程的优先级，但是这并不能保证高优先级的线程会在低优先级的线程前执行。

### 10. 什么是线程调度器(Thread Scheduler)和时间分片(Time Slicing)？
线程调度器是一个操作系统服务，它负责为Runnable状态的线程分配CPU时间。一旦我们创建一个线程并启动它，它的执行便依赖于线程调度器的实现。时间分片是指将可用的CPU时间分配给可用的Runnable线程的过程。分配CPU时间可以基于线程优先级或者线程等待的时间。线程调度并不受到Java虚拟机控制，所以由应用程序来控制它是更好的选择（也就是说不要让你的程序依赖于线程的优先级）。

### 11. 在多线程中，什么是上下文切换(context-switching)？
上下文切换是存储和恢复CPU状态的过程，它使得线程执行能够从中断点恢复执行。上下文切换是多任务操作系统和多线程环境的基本特征。

### 12. 你如何确保main()方法所在的线程是Java程序最后结束的线程？
我们可以使用Thread类的joint()方法来确保所有程序创建的线程在main()方法退出前结束。[Read more...](http://www.journaldev.com/1024/java-thread-join-example-with-explanation)
> 线程实例的方法join()方法可以使得一个线程在另一个线程结束后再执行。如果join()方法在一个线程实例上调用，当前运行着的线程将阻塞直到这个线程实例完成了执行。在join()方法内设定超时，使得join()方法的影响在特定超时后无效。当超时时，主方法和任务线程申请运行的时候是平等的。然而，当涉及sleep时，join()方法依靠操作系统计时，所以你不应该假定join()方法将会等待你指定的时间。

### 13.线程之间是如何通信的？
线程间通信有三种方式：

a. 使用全局变量。进程中的线程间内存共享，这是比较常用的通信方式和交互方式。主要由于多个线程可能更改全局变量，因此全局变量最好声明为violate

b. 使用消息实现通信。在Windows程序设计中，每一个线程都可以拥有自己的消息队列（UI线程默认自带消息队列和消息循环，工作线程需要手动实现消息循环），因此可以采用消息进行线程间通信sendMessage,postMessage。

```
1)定义消息#define WM_THREAD_SENDMSG=WM_USER+20;  
2)添加消息函数声明afx_msg int OnTSendmsg();
3)添加消息映射ON_MESSAGE(WM_THREAD_SENDMSG,OnTSM)
4)添加OnTSM()的实现函数；
5)在线程函数中添加PostMessage消息Post函数
```

c. 使用事件CEvent类实现线程间通信。Event对象有两种状态：有信号和无信号，线程可以监视处于有信号状态的事件，以便在适当的时候执行对事件的操作。

```
1)创建一个CEvent类的对象：CEvent threadStart;它默认处在未通信状态；
2)threadStart.SetEvent();使其处于通信状态；
3)调用WaitForSingleObject()来监视CEvent对象
```

当线程间是可以共享资源时，线程间通信是协调它们的重要的手段。Object类中wait()notify()notifyAll()方法可以用于线程间通信关于资源的锁的状态。[Read more](http://www.journaldev.com/1037/java-thread-wait-notify-and-notifyall-example)

### 14. 进程间通信方式及特点？
进程是转入内存并准备执行的程序，每个程序都有私有的虚拟地址空间，由代码，数据以及它可利用的系统资源(如文件，管道)组成。多进程/多线程是windows操作系统的一个基本特征。Linux系统一般都统称为进程。

由于不同的进程运行在各自不同的内存空间中，其中一个进程对于变量的修改另一方是无法感知的，因此，进程之间的消息传递不能通过变量或其他数据结构直接进行，只能通过进程间通信来完成。进程间通信是指**不同进程间进行数据共享和数据交换**。

进程间通信方式：**文件和记录锁定，管道，有名管道，FIFO，信号量，信号，消息队列，共享内存，套接字**。[^14] [Read Read Read...](http://blog.csdn.net/chenhuajie123/article/details/9315477)


### 15. 为什么线程通信的方法wait(), notify()和notifyAll()被定义在Object类里？
Java的每个对象中都有一个锁(monitor，也可以成为监视器) 并且wait()，notify()等方法用于等待对象的锁或者通知其他线程对象的监视器可用。在Java的线程中并没有可供任何对象使用的锁和同步器。这就是为什么这些方法是Object类的一部分，这样Java的每一个类都有用于线程间通信的基本方法

### 16. 为什么wait(), notify()和notifyAll()必须在同步方法或者同步块中被调用？
当一个线程需要调用对象的wait()方法的时候，这个线程必须拥有该对象的锁，接着它就会释放这个对象锁并进入等待状态直到其他线程调用这个对象上的notify()方法。同样的，当一个线程需要调用对象的notify()方法时，它会释放这个对象的锁，以便其他在等待的线程就可以得到这个对象锁。由于所有的这些方法都需要线程持有对象的锁，这样就只能通过同步来实现，所以他们只能在同步方法或者同步块中被调用。

### 17. 为什么Thread类的sleep()和yield()方法是静态的？
Thread类的sleep()和yield()方法将在当前正在执行的线程上运行。所以在其他处于等待状态的线程上调用这些方法是没有意义的。这就是为什么这些方法是静态的。它们可以在当前正在执行的线程中工作，并避免程序员错误的认为可以在其他非运行线程调用这些方法。

### 18. 如何确保线程安全？
在Java中可以有很多方法来保证线程安全——同步，使用原子类(atomic concurrent classes)，实现并发锁，使用volatile关键字，使用不变类和线程安全类。[Read more](http://www.journaldev.com/1061/java-synchronization-and-thread-safety-tutorial-with-examples)

### 19. volatile关键字在Java中有什么作用？
当我们使用volatile关键字去修饰变量的时候，所以线程都会直接读取该变量并且不缓存它。这就确保了线程读取到的变量是同内存中是一致的。

### 20. 同步方法和同步块，哪个是更好的选择？
同步块是更好的选择，因为它不会锁住整个对象（当然你也可以让它锁住整个对象）。同步方法会锁住整个对象，哪怕这个类中有多个不相关联的同步块，这通常会导致他们停止执行并需要等待获得这个对象上的锁。

### 21. 如何创建守护线程？
使用Thread类的setDaemon(true)方法可以将线程设置为守护线程，需要注意的是，需要在调用start()方法前调用这个方法，否则会抛出IllegalThreadStateException异常。

### 22. 什么是ThreadLocal?
ThreadLocal用于创建线程的本地变量，我们知道一个对象的所有线程会共享它的全局变量，所以这些变量不是线程安全的，我们可以使用同步技术。但是当我们不想使用同步的时候，我们可以选择ThreadLocal变量。

每个线程都会拥有他们自己的Thread变量，它们可以使用get()set()方法去获取他们的默认值或者在线程内部改变他们的值。ThreadLocal实例通常是希望它们同线程状态关联起来是private static属性。[Read more](http://www.journaldev.com/1076/java-threadlocal-example-to-create-thread-local-variables)。

### 23. 什么是死锁(Deadlock)？如何避免死锁？
死锁是指两个以上的线程永远阻塞的情况，这种情况产生至少需要两个以上的线程和两个以上的资源。死锁的四个必要条件：
- **互斥（Mutual exclusion）**：存在这样一种资源，它在某个时刻只能被分配给一个执行绪（也称为线程）使用；
- **持有（Hold and wait）**：当请求的资源已被占用从而导致执行绪阻塞时，资源占用者不但无需释放该资源，而且还可以继续请求更多资源；
- **不可剥夺（No preemption）**：执行绪获得到的互斥资源不可被强行剥夺，换句话说，只有资源占用者自己才能释放资源；
- **环形等待（Circular wait）**：若干执行绪以不同的次序获取互斥资源，从而形成环形等待的局面，想象在由多个执行绪组成的环形链中，每个执行绪都在等待下一个执行绪释放它持有的资源。

在系统中已经出现死锁后，应该及时检测到死锁的发生，并采取适当的措施来解除死锁。目前处理死锁的方法可归结为[四种](http://blog.csdn.net/joejames/article/details/37960873)

### 24. 什么是线程池？如何创建一个Java线程池？
一个线程池管理了一组工作线程，同时它还包括了一个用于放置等待执行的任务的队列。

java.util.concurrent.Executors提供了一个java.util.concurrent.Executor接口的实现用于创建线程池。[如何创建和使用线程池](http://www.journaldev.com/1069/java-thread-pool-example-using-executors-and-threadpoolexecutor)

# Java并发面试问题

### 1. 什么是原子操作？在Java Concurrency API中有哪些原子类(atomic classes)？
原子操作是指一个不受其他操作影响的操作任务单元。原子操作是在多线程环境下避免数据不一致必须的手段。

int++并不是一个原子操作，所以当一个线程读取它的值并加1时，另外一个线程有可能会读到之前的值，这就会引发错误。

为了解决这个问题，必须保证增加操作是原子的，在JDK1.5之前我们可以使用同步技术来做到这一点。到JDK1.5，java.util.concurrent.atomic包提供了int和long类型的装类，它们可以自动的保证对于他们的操作是原子的并且不需要使用同步。[Read more](http://www.journaldev.com/1069/java-thread-pool-example-using-executors-and-threadpoolexecutor)。

### 2. Java Concurrency API中的Lock接口(Lock interface)是什么？对比同步它有什么优势？
Lock接口比同步方法和同步块提供了更具扩展性的锁操作。他们允许更灵活的结构，可以具有完全不同的性质，并且可以支持多个相关类的条件对象。它的优势有：
- 可以使锁更公平
- 可以使线程在等待锁的时候响应中断
- 可以让线程尝试获取锁，并在无法获取锁的时候立即返回或者等待一段时间
- 可以在不同的范围，以不同的顺序获取和释放锁

### 3. 什么是Executors框架？
Executor框架同java.util.concurrent.Executor 接口在Java5中被引入。Executor框架是一个根据一组执行策略调用，调度，执行和控制的异步任务的框架。

无限制的创建线程会引起应用程序内存溢出。所以创建一个线程池是个更好的的解决方案，因为可以限制线程的数量并且可以回收再利用这些线程。利用Executors框架可以非常方便的创建一个线程池。[Read more](http://www.journaldev.com/1069/java-thread-pool-example-using-executors-and-threadpoolexecutor)

### 4. 什么是阻塞队列？如何使用阻塞队列来实现生产者-消费者模型？
java.util.concurrent.BlockingQueue的特性是：当队列是空的时，从队列中获取或删除元素的操作将会被阻塞，或者当队列是满时，往队列里添加元素的操作会被阻塞。

阻塞队列不接受空值，当你尝试向队列中添加空值的时候，它会抛出NullPointerException。

阻塞队列的实现都是线程安全的，所有的查询方法都是原子的并且使用了内部锁或者其他形式的并发控制。

BlockingQueue接口是java collections框架的一部分，它主要用于实现生产者-消费者问题。[使用阻塞队列实现生产者-消费者问题](http://www.journaldev.com/1034/java-blockingqueue-example-implementing-producer-consumer-problem)。

### 5. 什么是并发容器的实现？
Java集合类都是快速失败的，这就意味着当集合被改变且一个线程在使用迭代器遍历集合的时候，迭代器的next()方法将抛出ConcurrentModificationException异常。

并发容器支持并发的遍历和并发的更新。

主要的类有ConcurrentHashMap, CopyOnWriteArrayList 和CopyOnWriteArraySet。

### 6. Executors类是什么？
Executors为Executor，ExecutorService，ScheduledExecutorService，ThreadFactory和Callable类提供了一些工具方法。

Executors可以用于方便的创建线程池。

# 好文推荐

### [秒杀多线程面试题系列](http://blog.csdn.net/column/details/killthreadseries.html)

### [进程通信方式及特点](http://blog.csdn.net/chenhuajie123/article/details/9315477)

-----

[^1-3]: [进程与线程及其区别](http://blog.chinaunix.net/uid-21411227-id-1826748.html)
[^14]: [进程通信方式及特点](http://blog.csdn.net/chenhuajie123/article/details/9315477)
