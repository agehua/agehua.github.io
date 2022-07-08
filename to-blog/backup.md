

### android 存储介绍
外部存储目录external与内部存储目录internal

1. 内部存储目录：打开文件管理器，最外层的data目录即为内部存储目录，内部都是app的包名，存储着应用程序相关的数据，例如 data/data/包名/(shared_prefs、database、files、cache)，这里面的文件对未root用户不可见，当用户卸载App时，存储在这里的数据也会被销毁。
2. 外部存储目录：和内部存储目录data同级的其他目录基本都为外部存储目录，外部存储目录分为私有目录和公有目录，私有目录存储在android文件夹下，各应用数据存储在对应以包为名的目录下，这里的数据不同应用不可共享。与android目录同级的目录为公有目录，这些目录可以由我们自己创建。

Context 类提供了基本文件和目录处理方法（存储的文件仅供应用内部使用，此法足以）来处理获取并操作内部存储空间下应用私有目录文件的方法。
（1）File getFilesDir() 获取/data/data/<包名>/files目录。
（2）FileInputStream openFileInput(String name)：打开现有文件进行读取
（3）FileOutputStream openFileOutPutStream(String name,int mode)： 打开文件进行写入，如果不存在就创建它。
（4）File getDir(String name,int mode) 获取/data/data/<包名>目录的子目录（不存在就创建它)。
（5）String[] fileList() *获取主文件目录下的文件列表，可与其它方法配合使用，如openFileInput(String)>。
（6）File getCacheDir() 获取/data/data/<包名>/cache目录，应及时清理该目录，并节约使用。

3. 应用内部存储空间中的应用私有目录和外部存储空间中的应用私有目录和区别

- （1）context.getFilesDir() 内部存储data/data/包名/files目录；
- （2）context.getCacheDir() 内部存储data/data/包名/cache目录；
- （3）Environment.getExternalStorageDirectory() 外部存储根目录 Environment.getExternalStoragePublicDirectory("")； 外部存储 /storage/emulated/0
- （4）外部存储公有目录context.getExternalFilesDir() 外部存储私有目录 storage/sdcard/Android/data/包名/files。一般存储长时间保存的数据。 
- （5）context.getExternalCacheDir() 外部存储私有目录 storage/sdcard/Android/data/包名/cache。一般存储临时缓存数据。


从 7.0 开始，Android SDK 中的 StrictMode策略禁止开发人员在应用外部公开 file:// URI，如果我们在使用 file://URI 时忽视了这两条规定，将导致用户在 7.0 及更高版本系统的设备中使用到相关功能时，出现 FileUriExposedException 异常，导致应用出现崩溃闪退问题。而这两个过程的替代解决方案便是使用 FileProvider。FileProvider帮助我们将访问受限的 file:// URI 转化为可以授权共享的 content:// URI。

FileProvider的使用：
a. 声明FileProvider为ContentProvider,并给予一定权限(及指定一个位置用来保存文件) 在AndroidManifest.xml中添加一个FileProvider声明。


#### 1.介绍下java的内存模型

  方法区、pc寄存器、栈和堆，哪些是线程安全的，哪些是非线程安全的？
方法区和堆，是所有线程共享的，非线程安全
 
#### 2.介绍下java gc的原理和处理方式
- 假如我现在有个对象比较大，然后eden区放不下了，jvm会怎么办？
如果对象大小超过

- 如果说老年代也放不下呢？

老年代也帆布下时，会触发full gc，当执行Full GC后空间仍然不足，报错：java.lang.OutOfMemoryError: Java heap space

- 年轻代和老年代的gc会卡住主线程吗？
都会

老年代使用并发标记清除（CMS）或标记-整理算法
STW总会发生 不管是新生代还是老年代 就算是CMS也有STW的时候
CMS gc时 垃圾收集线程与用户线程一段时间内同时工作(交替执行)

从年轻代空间(包括Eden和Survivor 区域)回收内存被称为 Minor GC
从老年代GC称为Major GC
FullGC

执行 Minor GC(年轻代GC) 的时候，JVM 会检查老年代中最大连续可用空间是否大于了当前新生代所有对象的总大小
如果大于，则直接执行 Minor GC(年轻代GC)（这个时候执行是没有风险的）
如果小于，JVM 会检查是否开启了空间分配担保机制，如果没有开启则直接改为执行Full GC
如果开启担保机制，则 JVM 会检查老年代中最大连续可用空间是否大于历次晋升到老年代中的平均大小，如果小于则执行改为执行Full GC
如果大于则会执行 Minor GC(年轻代GC)，如果 Minor GC(年轻代GC) 执行失败则会执行 Full GC

**出现 Full GC 的时候经常伴随至少一次的Minor GC** ,但不绝对。Major GC的速度一般会比Minor GC慢10倍以上

- 老年代的gc和年轻代的gc是一样的吗？还是说有什么区别？
年轻代使用`标记 - 整理（标记 - 复制）`算法
老年代使用有单线程的`标记 - 清除`和并发的 `CMS (并发标记清除)`
因为老年代对象存活概率高，使用涉及到copy的算法效率会低

#### 3.线程安全里面的乐观锁和悲观锁了解吗？
java里面有哪些方式可以实现乐观锁？ `CAS`

- 关于cas，如果我去读值的时候，比如我的预期值是1，别的线程把它改成0，然后第三个线程又把他改成了1，这个时候我去读这个值，虽然读出来还是1，但实际上它已经变了，这种怎么处理？

关于ABA问题，Java并发包为了解决这个问题，提供了一个带有标记的原子引用类“AtomicStampedReference”，它可以通过控制变量值的版本来保证CAS的正确性。因此，在使用CAS前要考虑清楚“ABA”问题是否会影响程序并发的正确性，如果需要解决ABA问题，改用传统的互斥同步可能会比原子类更高效。

#### 4.synchronize和volatile有什么区别？
当一个变量被声明为 volatile 时：

- 线程在【读取】共享变量时，会先清空本地内存变量值，再从主内存获取最新值
- 线程在【写入】共享变量时，不会把值缓存在寄存器或其他地方（就是刚刚说的所谓的「工作内存」），而是会把值刷新回主内存

**volatile 不会阻塞线程**

synchronized 是排他的，线程排队就要有切换，这个切换就好比上面的例子，要完成切换，还得记准线程上一次的操作，很累CPU大脑，这就是通常说的上下文切换会带来很大开销

volatile 就不一样了，它是非阻塞的方式，所以在解决共享变量可见性问题的时候，volatile 就是 synchronized 的弱同步体现了

volatile 除了解决可见性问题，还能解决编译优化重排序问题

如果我想实现原子性，有什么办法吗？
- 使用synchronized关键字
- 使用ReentrantLock
- 使用AtomicInterger

#### 5.泛型擦除了解吗？
Java泛型，它允许我们在定义类和接口时使用类型参数。
Java 泛型是在编译级别实现的。编译器生成的字节码不包含运行时执行的泛型类型信息。

- 运行时类型检查。
因为泛型实际参数会被擦除，`List<String>` 会被擦除为 `List`，所以当通过一些手段（强制转换，raw type 等）将其他类型的值放入这个 List 的时候并不会出错，直到实际访问时才会发生问题。

怎样将泛型类在转换为具体类型呢，可以看下Gson是如何实现的：

~~~ java
public <T> T fromJson(String json, Class<T> classOfT) throws JsonSyntaxException {
    Object object = fromJson(json, (Type) classOfT);
    return Primitives.wrap(classOfT).cast(object);
}
~~~

#### 6.介绍下hashMap
- 那jdk1.8的时候链表会转化成红黑树，那它会一直是红黑树吗？那它后面还会转化成链表吗？

  当链表中的元素个数大于8(此时 node有9个)，并且数组的长度大于等于64时才会将链表转为红黑树

会转化为链表

#### 7.activity, application，context这三个有什么区别和联系吗？
  那他们三个之间的继承关系是什么样的？
<https://cloud.tencent.com/developer/article/1628570>

#### 8.ActivityA 跳到ActivityB, 按照生命周期顺序，他们分别会执行哪些函数呢？

#### 9.setContentView,了解它的设置原理吗？
  我们调用setContentView之后，我们怎么把contentView里面的内容显示再屏幕上呢？它是怎么实现的？
  你刚才说监听这个vSync信号，那它监听这个信号是怎么监听的？

10.invalidate和requestLayout有什么区别？

11.比如说我的activity，启动了很多个fragment，然后他们一个个叠在activity上面。
   如果说我按back键的时候，希望fragment一个个移除，这种怎么实现？
   
   show/hide add/replace
   addBackToStack

12.handler有两种消息，一个是post的消息，一个是postDelay的消息，这两个在实现上有什么区别吗？
   那比如我post一个消息，然后postDelay一个2s的消息，你意思是说，我先插入一个post消息，
   然后等2s之后再插入那个delay的消息吗？
   比如说我当前线程处于休眠状态，然后来了个即时消息，它怎么做到唤醒线程去处理的？
   比如说我线程正在经历这个2s的休眠，那比如说我1.5秒的时候过来一个消息，那这时候handler是怎么处理的？

13.介绍下事件分发机制，比如说我有个down事件，那它是怎么传递的？
   那比如说我子view消耗了down事件了，return true了，那我的move事件分发的时候，它会直接走到我的子view吗？
   还是说会在走一遍刚刚的流程？
   那你意思说说，如果我的子view处理了down事件，那我的父viewGroup就不能拦截我的move事件了嘛？

### 算法题学习方向
算法 - Algorithms

- 排序算法：快速排序、归并排序、计数排序
- 搜索算法：回溯、递归、剪枝技巧
- 图论：最短路、最小生成树、网络流建模
- 动态规划：背包问题、最长子序列、计数问题
- 基础技巧：分治、倍增、二分、贪心

数据结构 - Data Structures

- 数组与链表：单 / 双向链表、跳舞链
- 栈与队列
- 树与图：最近公共祖先、并查集
- 哈希表
- 堆：大 / 小根堆、可并堆
- 字符串：字典树、后缀树

