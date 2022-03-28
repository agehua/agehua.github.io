

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

