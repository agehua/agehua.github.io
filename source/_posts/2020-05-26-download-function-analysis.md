---
layout: post
title: 分析下载功能源码和实现
category: accumulation
tags:
    - download
keywords: Download
banner: http://cdn.conorlee.top/Field%20with%20Two%20Rabbits.jpg
thumbnail: http://cdn.conorlee.top/Field%20with%20Two%20Rabbits.jpg
toc: true
---

### 安卓下载功能分析
最近看了一份实现下载功能的代码，为了方便学习和记忆，将下载功能拆分，根据功能模块分析源码逻辑，做了一份整理：

**目标**
- 1.支持多种下载api，如okhttp、HttpURLConnection
- 2.多线程、断点续传下载，线程池支持
- 3.下载管理：开始、暂停、继续、取消、重新开始
<!--more-->
#### 最基础功能下载
需要创建一个代理类，FileDownloadProxy 来判断需要使用哪种方式下载
以使用HttpURLConnection方式为例下载文件，创建一个 UrlConnectionFileDownload 类
~~~ Java
    HttpURLConnection connection = null;
    File downloadingFile = new File(bean.getDownloadingPath());
    long downloadSize = downloadingFile.length();
    try {
        connection = (HttpURLConnection) url.openConnection();
        connection.setRequestMethod("GET");
        connection.addRequestProperty("Connection", "Keep-Alive");
        connection.setConnectTimeout(30000);
        connection.setReadTimeout(30000);
        connection.setInstanceFollowRedirects(false);

        int responseCode = connection.getResponseCode();
        // 这里先忽略其他请求状态码，比如重定向302等
        switch (responseCode) {
            case HttpURLConnection.HTTP_OK:
                inputStream = urlConn.getInputStream();
            //...
        }
    } catch (Exception e) {
        return FileDownloadConstant.DOWNLOAD_ERROR;
    }
~~~
然后根据输入流去下载文件，后面在分析如何断点下载

#### 下载线程管理
下载放在一个单独的线程里，最好线程池管理线程.
线程池采用ThreadPoolExecutor，第六个参数可以指定一个ThreadFactory，创建指定的Runnable对象
~~~ Java
 public ThreadPoolExecutor(int corePoolSize,
                              int maximumPoolSize,
                              long keepAliveTime,
                              TimeUnit unit,
                              BlockingQueue<Runnable> workQueue,
                              ThreadFactory threadFactory) {
    this(corePoolSize, maximumPoolSize, keepAliveTime, unit, workQueue,
             threadFactory, defaultHandler);
}
~~~
实现一个Runnable类，XInfiniteRetryRunnable支持重试，并且能够通知前台线程执行状态

伪代码实现如下：
~~~ Java
int retryCount;
boolean isRunning;
int times = 5;

@Override
public final void run() {
    // 准备活动
    if (!onPreExecute(bean)) {
        if (!isRunning) {
            onCancelled(bean);
        } else {
            onPreExecuteError(bean);
        }
        return;
    }
    while (isRunning) {
        if (onExecute() || !isRunning) {
            // 执行成功，或外部中断，退出循环
            break;
        }
        // 添加重试
        while (isRunning && retryCount < times) {
            Thread.sleep(100 * retryCount);
            retryCount++;
        }
    }

    // 执行结束
    if (isRunning) {
        // 正常结束情况下，调用onPostExecute
        onExecuteFinish(bean);
    } else {
        // 被中断情况下，调用onCancelled
        onCancelled(bean);
    }
}

// 通知外界，用于执行耗时任务
boolean onExecute(T bean);
// 通知外界，正常执行成功
boolean onExecuteFinish(T bean);
// 在这里执行下载前数据的初始化操作
boolean onPreExecute(T bean)
void onCancelled(T bean);
void onPreExecuteError(T bean);
~~~
最后调用ExecutorService.submit() 提交下载线程

#### 下载状态管理
前端需要知道下载中的状态，并作出相应的处理
定义一个下载类对象，总结一下，大概有五个状态TODO,DOING,DONE,ERROR,DEFAULT（未初始化状态）
~~~ Java
public interface TaskBean extends Cloneable {
    public static final int STATUS_DEFAULT = -1;// 默认状态(未进执行队列)
    public static final int STATUS_TODO = 0;// 未执行状态
    public static final int STATUS_DOING = 1;// 正在执行状态
    public static final int STATUS_DONE = 2;// 已完成状态
    public static final int STATUS_ERROR = 3;// 错误状态

    // 定义一些属性方法
    String getId();
    int getStatus();
    void setStatus(int status);
    public long getCompleteSize();
    public void setCompleteSize(long completeSize);
}
~~~

##### 使用状态机处理状态转换
状态机有五个行为start,pause,abort,endSuccess,endError
子类继承时，重写五个行为的回调方法即可：onStart(),onPause(),onAbort(),onEndSuccess(),onEndError()

 * start = {@link TaskBean#STATUS_TODO} / {@link TaskBean#STATUS_ERROR} to {@link TaskBean#STATUS_DOING}
 * pause = {@link TaskBean#STATUS_DOING} to {@link TaskBean#STATUS_TODO} or {@link TaskBean#STATUS_DEFAULT}
 * abort = all status to {@link TaskBean#STATUS_DONE}
 * endSuccess ={@link TaskBean#STATUS_DOING} to {@link TaskBean#STATUS_DONE}
 * endError = {@link TaskBean#STATUS_DOING} to {@link TaskBean#STATUS_ERROR}

先定义一个接口，大概方法如下：
~~~ Java
public interface ITaskExecutor<B extends TaskBean> {
    // 这里的参数采用数据，而不是int值的方式，可以避免必须传入一个int值，preStatus可以为空
    int start(int... preStatus);
    int pause(int... postStatus);
    boolean abort();
    B getBean();
    String getId();
    void setStatus(int status);
    int getStatus();
    void setListener(ITaskListener<B> listener);
    ITaskListener<B> getListener(); // 通过一个listener将状态机状态通知出去
}
~~~
定义一个抽象类实现状态的检查和切换，下面代码以start状态为例：
~~~ Java
public abstract class StateMachineExecutor<B extends TaskBean> implements ITaskExecutor<B> {

    private volatile int mStatus; // 状态
    private B mBean; // 任务数据

    @Override
    public int start(int... preStatus) {
        // 检查任务状态
        int oldStatus = getStatus();
        if (oldStatus == TaskBean.STATUS_STARTING || oldStatus == TaskBean.STATUS_DOING) {
            return StateMachineStatus.TASK_START_DOING_OR_STARTING;
        }
        if (getStatus() != TaskBean.STATUS_TODO
                && getStatus() != TaskBean.STATUS_ERROR) {
            //在三种情况下可以启动下载任务：STATUS_TODO,STATUS_ERROR,preStatus[0]指定状态才能启动
            return StateMachineStatus.TASK_INVALIDATED_STATUS;
        }
        //将当前任务标记为正在启动状态
        setStatus(TaskBean.STATUS_STARTING);

        if (!onStart()) { // 子类要实现onStart()方法
            // 任务启动失败
            setStatus(oldStatus);
            return StateMachineStatus.TASK_START_FAIL;
        }
        // 任务启动完成，设置状态为进行中
        setStatus(TaskBean.STATUS_DOING);

        if (mListener != null) {
            mListener.onStart(getBean());
        }
        return StateMachineStatus.TASK_START_SUCCESS;
    }

    @Override
    public int pause(int... postStatus) {
    }

    @Override
    public boolean abort() {
    }

    @Override
    public void setStatus(int status) {
        mStatus = status;
        mBean.setStatus(status);
    }

    @Override
    public int getStatus() {
        return mStatus;
    }

    // 子类通过实现抽象方法，获取执行中的状态回调
    protected abstract boolean onStart();
    protected abstract boolean onPause();
    protected abstract boolean onAbort();
    protected abstract boolean onEndSuccess();
    protected abstract boolean onEndError(String errorCode, boolean retry);
}
~~~
> 在这个抽象类里，定义了listener，子类可以自己判断是否需要监听状态事件，定义抽象方法，子类必须在特定状态实现特定方法。setStatus()和getStatus()是操作**任务对象（B）**的状态，而start()方法的返回值是状态机的状态，这样的好处是状态机调度任务执行状态（这是使用状态的目的）和状态机本身的状态相互分离。

##### 状态机与下载任务结合
~~~ Java
public class DownloadFileTask extends StateMachineExecutor<FileDownloadObject> {

    /**
     * 负责启动和轮询的异步线程
     */
    private volatile FileDownloadRunnable mRunnable;
    @Override
    protected boolean onStart() {
        // 在开始状态中创建runnable，并提交给线程池
        mRunnable = new FileDownloadRunnable(mContext, this);
        FileDownloadExecutors.UNIVERSAL_DOWNLOAD_POOL.submit(mRunnable);
    }

     @Override
    protected boolean onPause() {
        exitRunnable();
        return true;
    }

    @Override
    protected boolean onAbort() {
        exitRunnable();
        return true;
    }

    @Override
    protected boolean onEndSuccess() {
        mRunnable = null;
        return true;
    }

    @Override
    protected boolean onEndError(String errorCode, boolean retry) {
        mRunnable = null;
        return true;
    }

    private void exitRunnable() {
        if (mRunnable != null) {
            mRunnable.cancel();
            mRunnable = null;
        }
    }

    public void notifyDoing(long completeSize) {
        if (mListener != null) {
            if (this.getStatus() != this.getBean().getStatus()) {
                //保持下载任务和下载对象的状态统一
                this.getBean().setStatus(this.getStatus());
            }
            mListener.onDoing(getBean(), completeSize);
        }
    }
}
~~~
这里的FileDownloadRunnable就是继承了XInfiniteRetryRunnable类，实现了onExecute()、onExecuteFinish()、onPreExecute()方法
FileDownloadObject是实现了TaskBean接口，拓展一些TaskBean没有的属性
~~~ Java
class FileDownloadRunnable extends XInfiniteRetryRunnable<FileDownloadObject>  {
    private XBaseTaskExecutor<FileDownloadObject> mHost;

    protected FileDownloadRunnable(Context context, XBaseTaskExecutor<FileDownloadObject> host) {
        mHost = host;
    }

    @Override
    public boolean onPreExecute(FileDownloadObject bean) {
        //检查url，filePath是否正确
        if (TextUtils.isEmpty(bean.getId())) {
            getBean().errorCode = FileDownloadConstant.FILE_DOWNLOAD_URL_NULL;
            // 校验失败返回false，线程不在继续执行
            return false;
        }

        if (TextUtils.isEmpty(bean.getDownloadPath())) {
            getBean().errorCode = FileDownloadConstant.FILE_DOWNLOAD_PATH_NULL;
            return false;
        }
    }

    @Override
    public boolean onExecute(FileDownloadObject bean) {
        int result = -1;
        //标记下载开始时间
        bean.setDownloadStartTime(System.currentTimeMillis());
        // 封装了HttpUrlConnection，下载文件，并监听下载进度
        result = httpAdapter.downloadFile(bean, bean.getCallbackInterval(), new    DownloadProgressCallback<FileDownloadObject>() {
            @Override
            public void onDataChanged(FileDownloadObject bean) {
                // 下载进度保存在了bean中
                if (bean.verifyContentLength() && bean.getFileSzie() == -1) {
                    bean.setFileSize(bean.getContentLength());
                }
                // 通过DownloadFileTask的listener将进度通知出去
                mHost.notifyDoing(-1);
            }
        });
    }

    @Override
    public void onExecuteFinish(FileDownloadObject bean) {
        if (isDownloadSuccess) {
            mHost.endSuccess();
        } else {
            deleteFileIfError(bean);
            mHost.endError(getBean().errorCode, true);
        }
    }
    //... 
}
~~~
到这里就将下载线程和状态机联系了起来，外面只需要接触DownloadFileTask这一个类，就可以监听执行下载的状态
#### HttpUrlConnection断点下载
再说下HttpUrlConnection实现断点下载，补充文章开头的部分
~~~ Java
int responseCode = connection.getResponseCode();
switch (responseCode) {
    case HttpURLConnection.HTTP_OK:
    case HttpURLConnection.HTTP_PARTIAL: // 关键在处理206状态码
            bean.setFileSize(connection.getContentLength());
    return downloadFileByUrlConnection(connection, bean, callbackInterval, callback);
    //...
}
~~~
下载涉及到BufferedInputStream的使用，具体可以看下这篇博文：[JavaIO之BufferedInputStream详解](https://www.jianshu.com/p/c4bb6ad69866)
~~~ Java
private static final int DOWNLOAD_BUFFER_SIZE = 16 * 1024;

private int downloadFileByUrlConnection(HttpURLConnection connection, B bean, long callbackInterval, DownloadProgressCallback<B> callback) {

    FileOutputStream fos = null;
    BufferedInputStream bis = null;
    InputStream inputStream = null;

    File downloadingFile = null;
    long contentLength = -1;
    long completeSize = 0;
    byte[] mBuffer;

    bean.setErrorCode("");
    try {
        if (connection.getURL() != null) {
            bean.setDownloadUrl(connection.getURL().toString()); 
        }
        inputStream = connection.getInputStream();
        if (inputStream == null) {
            bean.setErrorCode(FileDownloadConstant.FILE_DOWNLOAD_INPUTSTREAM_IS_NULL);
            return FileDownloadConstant.DOWNLOAD_INTERVAL_RETRY;
        }
        mBuffer = new byte[DOWNLOAD_BUFFER_SIZE];
        //从contentLength获取，若返回时200，则contentLength等于整个文件大小
        //若返回206，则contentLength表示这个文件剩余下载的大小，需要更新整个文件大小
        contentLength = bean.getFileSzie();
        completeSize = bean.getCompleteSize();
        downloadingFile = new File(bean.getDownloadingPath());

        //contentLength对于下载没有帮助，仅用于计算下载进度
        if (contentLength <= 0) {
            deliverException(bean);
        } else {
            //修正completeSize大小
            if (completeSize < 0) {
                completeSize = 0;
            }
            //更新总大小
            long realSize = completeSize + contentLength;
            if (realSize > 0 && realSize != bean.getFileSzie()) {
                bean.setFileSize(realSize);
            }
        }

        bis = new BufferedInputStream(inputStream);
        fos = new FileOutputStream(downloadingFile, true);

        // 之前read读取的数据量
        int bufferStart = 0;
        // 一次read读取的数据量
        int numRead = 0;
        // 用于控制刷新进度的变量
        long curUpdateTime = 0;
        long lastUpdateTime = System.currentTimeMillis();
        long lastCompleteSize = completeSize;
        while (true) {
            // 如果被中断，则整体退出
            if (!isRunning()) {
                bean.setErrorCode(FileDownloadConstant.FILE_DOWNLOAD_ABORT);
                return FileDownloadConstant.DOWNLOAD_ERROR;
            }
            curUpdateTime = System.currentTimeMillis();
            numRead = bis.read(mBuffer, bufferStart, DOWNLOAD_BUFFER_SIZE - bufferStart);
            // 已经没有数据了，退出循环
            if (numRead == -1) {
                if (bufferStart > 0) {
                    // buffer未填充满，但已经没数据了，则写入文件
                    fos.write(mBuffer, 0, bufferStart);
                }
                break;
            }

            // 递增已下载大小
            completeSize = completeSize + numRead;
            // 第一次刷新UI，同时为了防止频繁刷新界面，第二次刷新UI间隔大于1秒
            if (curUpdateTime - lastUpdateTime >= callbackInterval || isFirstCallback) {
                //计算速度
                long increaseCompleteSize = completeSize - lastCompleteSize;
                long gapTime = curUpdateTime - lastUpdateTime;
                long speed = 0;
                if (gapTime != 0) {
                    speed = increaseCompleteSize / gapTime * 1000;
                }

                bean.setSpeed(speed);
                bean.setCompleteSize(completeSize);
                lastUpdateTime = curUpdateTime;
                lastCompleteSize = completeSize;
                if (callback != null) {
                    callback.onDataChanged(bean);
                }
            }

            if (numRead + bufferStart < DOWNLOAD_BUFFER_SIZE) {
                // buffer未填满
                bufferStart = numRead + bufferStart;
            } else {
                // buffer已填满，则写入文件
                fos.write(mBuffer, 0, DOWNLOAD_BUFFER_SIZE);
                // buffer重新开始填充
                bufferStart = 0;
            }
        }
        //下载完成
        if (callback != null) {
            bean.setCompleteSize(completeSize);
            callback.onDataChanged(bean);
        }
        return FileDownloadConstant.DOWNLOAD_SUCCESS;
    } catch (SocketTimeoutException e) {
        return FileDownloadConstant.DOWNLOAD_SOCKET_RETRY;
    } catch (SocketException e) {
        return FileDownloadConstant.DOWNLOAD_SOCKET_RETRY;
    } catch (IOException e) {
        bean.setErrorCode(FileDownloadConstant.FILE_DOWNLOAD_IO_EXCEPTION);
        bean.setErrorInfo(e.getMessage());
        return FileDownloadConstant.DOWNLOAD_INTERVAL_RETRY;
    } finally {
        FileUtils.silentlyCloseCloseable(bis);
        FileUtils.silentlyCloseCloseable(inputStream);
        FileUtils.silentlyCloseCloseable(fos);
    }
}
~~~
文章就到这里了，后面也可以把 DownloadFileTask 这个类加到一个单独的进程，采用aidl的方式跨进程调用，这里就不详说了。
