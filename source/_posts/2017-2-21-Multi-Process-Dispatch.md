---
layout: post
title:  解决Android多进程导致Application重复创建问题
category: accumulation
tags: multi-process
keywords: multi-process
banner: http://obxk8w81b.bkt.clouddn.com/Avenue%20of%20Plane%20Trees%20near%20Arles%20Station.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Avenue%20of%20Plane%20Trees%20near%20Arles%20Station.jpg
toc: true
---

> 本编文章是在阅读：[Android架构思考(模块化、多进程)](http://blog.spinytech.com/2016/12/28/android_modularization/)过程中，结合自己的理解，对Android开启多进程，会导致Application重复创建问题进行一个总结。

------

### Android开启多进程
一般情况下，一个应用程序就一个进程，这个进程的名称就是应用程序包名。Android的四大组件在AndroidManifest文件中注册的时候，有个属性是android:process，这里可以指定组件的所处的进程。

一个进程情况下，Application的onCreate方法只会执行一次，但如果应用中采用多进程方式，onCreate方法会执行多次。

### 解决Application的onCreate方法多次调用
大概有两种方式
<!--more-->

#### 一、根据不同的进程名字进行不同数据的初始化。
这是现在网络上通用的方法，在自定义的Application的onCreate方法中控制不同进程的初始化

代码如下：

~~~ Java
@Override
public void onCreate() {
        super.onCreate();
    String processName = getProcessName(this, android.os.Process.myPid());
    if (processName != null) {
    boolean defaultProcess = processName.equals(Constants.REAL_PACKAGE_NAME);
    // 默认的主进程启动时初始化应用
    if (defaultProcess) {
	    initAppForMainProcess();
    }
    // 其他进程启动时初始化对应内容
    else if (processName.contains(":webbrowser")) {

    } else if (processName.contains(":bgmusic")) {

    }  
}    
~~~

获取当前进程名的方法如下：

~~~ Java
/**
 * @return null may be returned if the specified process not found
 */
public static String getProcessName(Context cxt, int pid) {
    ActivityManager am = (ActivityManager) cxt.getSystemService(Context.ACTIVITY_SERVICE);
    List<RunningAppP.rocessInfo> runningApps = am.getRunningAppProcesses();
    if (runningApps == null) {
        return null;
    }
    for (RunningAppProcessInfo procInfo : runningApps) {
        if (procInfo.pid == pid) {
            return procInfo.processName;
        }
    }
    return null;
}
~~~

#### 二、剥离出一个类，具有同Application相同的生命周期方法，每个进程拥有一个该类实例

这就是文章开头提到的博客中使用的方式[Android架构思考(模块化、多进程)](http://blog.spinytech.com/2016/12/28/android_modularization/)

实现这种方式，一共有涉及到3个类，
- 一个是MaApplication继承了Application，是所有程序的入口，这是一个抽象类，需要子类去实现一些方法
- 一个是BaseApplicationLogic，这也是基类，由这个类来实现每个进程单独管理Application的生命周期，每个进程实现一个该类的子类
- 还有一个类是PriorityLogicWrapper，它是一个封装类，继承了Comparable接口，实现了对BaseApplicationLogic按照指定顺序排序（也就是可以按照优先级顺序初始化BaseApplicationLogic）

首先，我们先把所有ApplicationLogic注册到MaApplication中；
然后，MaApplication会根据注册时的进程名信息进行筛选，选择相同进程名的ApplicationLogic，保存到本进程中；
其次，对这些本进程的ApplicationLogic进行实例化；
最后，调用ApplicationLogic的onCreate方法，实现ApplicationLogic与Application生命周期同步，同时还有onTerminate、onLowMemory、onTrimMemory、onConfigurationChanged等方法，与onCreate一致。

流程图如下所示：
![多进程Application启动流程](/images/blogimages/2017/multi-Application-flow-chart.png)

代码实现，先看基类BaseApplicationLogic，每个进程都要实现一个该类的子类：

~~~ Java
public class BaseApplicationLogic {
    protected MaApplication mApplication;
    public BaseApplicationLogic() {
    }

    public void setApplication(@NonNull MaApplication application) {
        mApplication = application;
    }

    public void onCreate() {
    }

    public void onTerminate() {
    }

    public void onLowMemory() {
    }

    public void onTrimMemory(int level) {
    }

    public void onConfigurationChanged(Configuration newConfig) {
    }
}
~~~

其次PriorityLogicWrapper，这是一个封装类，实现了BaseApplicationLogic的按优先级排列：
~~~ Java
public class PriorityLogicWrapper implements Comparable<PriorityLogicWrapper> {

    public int priority = 0;
    public Class<? extends BaseApplicationLogic> logicClass = null;
    public BaseApplicationLogic instance;

    public PriorityLogicWrapper(int priority, Class<? extends BaseApplicationLogic> logicClass) {
        this.priority = priority;
        this.logicClass = logicClass;
    }

    @Override
    public int compareTo(PriorityLogicWrapper o) {
        return o.priority - this.priority;
    }
}
~~~



在MaApplication中直接对PriorityLogicWrapper进行操作，无需操作BaseApplicationLogic对象
~~~ Java
public abstract class MaApplication extends Application {

    //mLogicList只持有当前进程的PriorityLogicWrapper对象
    private ArrayList<PriorityLogicWrapper> mLogicList;
    //mLogicClassMap持有所有进程的PriorityLogicWrapper数组对象
    private HashMap<String, ArrayList<PriorityLogicWrapper>> mLogicClassMap;

    @Override
    public void onCreate() {
        super.onCreate();
        sInstance = this;
        init();
        startWideRouter();
        initializeLogic();
        dispatchLogic();
        instantiateLogic();

        if (null != mLogicList && mLogicList.size() > 0) {
            for (PriorityLogicWrapper priorityLogicWrapper : mLogicList) {
                if (null != priorityLogicWrapper && null != priorityLogicWrapper.instance) {
                    //找到当前进程的BaseApplicationLogic实例后，执行其onCreate()方法
                    priorityLogicWrapper.instance.onCreate();
                }
            }
        }
    }

    private void init() {
        mLogicClassMap = new HashMap<>();
    }

    protected void startWideRouter() {
        if (needMultipleProcess()) {
            //WideRouterApplicationLogic就是BaseApplicationLogic的一个子类
            registerApplicationLogic(WideRouterApplicationLogic.PROCESS_NAME, 1000, WideRouterApplicationLogic.class);
        }
    }

    public abstract boolean needMultipleProcess();

    /**
     * 添加所有来自不同进程的，不同的BaseApplicationLogic对象到HashMap中
     * @param processName 进程名
     * @param priority 优先级
     * @param logicClass 继承BaseApplicationLogic的对象
     * @return
     */
    protected boolean registerApplicationLogic(String processName, int priority, @NonNull Class<? extends BaseApplicationLogic> logicClass) {
        boolean result = false;
        if (null != mLogicClassMap) {
            ArrayList<PriorityLogicWrapper> tempList = mLogicClassMap.get(processName);
            if (null == tempList) {
                tempList = new ArrayList<>();
                mLogicClassMap.put(processName, tempList);
            }
            if (tempList.size() > 0) {
                for (PriorityLogicWrapper priorityLogicWrapper : tempList) {
                    if (logicClass.getName().equals(priorityLogicWrapper.logicClass.getName())) {
                        throw new RuntimeException(logicClass.getName() + " has registered.");
                    }
                }
            }
            PriorityLogicWrapper priorityLogicWrapper = new PriorityLogicWrapper(priority, logicClass);
            tempList.add(priorityLogicWrapper);
            //tempList更新，则mLogicClassMap中的value也跟着更新了
        }
        return result;
    }

    /**
     * 得到一个属于本进程的ArrayList对象，里面保存着封装类PriorityLogicWrapper
     */
    private void dispatchLogic() {
        if (null != mLogicClassMap) {
            //根据进程名，得到该进程名对应的ArrayList<PriorityLogicWrapper>
            mLogicList = mLogicClassMap.get(ProcessUtil.getProcessName(this, ProcessUtil.getMyProcessId()));
        }
    }

    /**
     * 取得mLogicList中的PriorityLogicWrapper对象，并按优先级顺序初始化BaseApplicationLogic对象
     */
    private void instantiateLogic() {
        if (null != mLogicList && mLogicList.size() > 0) {
            if (null != mLogicList && mLogicList.size() > 0) {
                Collections.sort(mLogicList); //根据进程优先级，按顺序初始化
                for (PriorityLogicWrapper priorityLogicWrapper : mLogicList) {
                    if (null != priorityLogicWrapper) {
                        try {
                            /**
                             * 调用Class.newInstance()，会创建这个Class的实例，但是不会执行Android中这个类相关的生命周期
                             * **/
                            priorityLogicWrapper.instance = priorityLogicWrapper.logicClass.newInstance();
                        } catch (InstantiationException e) {
                            e.printStackTrace();
                        } catch (IllegalAccessException e) {
                            e.printStackTrace();
                        }
                        if (null != priorityLogicWrapper.instance) {
                            priorityLogicWrapper.instance.setApplication(this);
                        }
                    }
                }
            }
        }
    }

    //Application生命周期的处理，下面方法都类似
    @Override
    public void onTerminate() {
        super.onTerminate();
        if (null != mLogicList && mLogicList.size() > 0) {
            for (PriorityLogicWrapper priorityLogicWrapper : mLogicList) {
                if (null != priorityLogicWrapper && null != priorityLogicWrapper.instance) {
                    priorityLogicWrapper.instance.onTerminate();
                }
            }
        }
    }

    @Override
    public void onLowMemory() {
        super.onLowMemory();
        if (null != mLogicList && mLogicList.size() > 0) {
            for (PriorityLogicWrapper priorityLogicWrapper : mLogicList) {
                if (null != priorityLogicWrapper && null != priorityLogicWrapper.instance) {
                    priorityLogicWrapper.instance.onLowMemory();
                }
            }
        }
    }

    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        if (null != mLogicList && mLogicList.size() > 0) {
            for (PriorityLogicWrapper priorityLogicWrapper : mLogicList) {
                if (null != priorityLogicWrapper && null != priorityLogicWrapper.instance) {
                    priorityLogicWrapper.instance.onTrimMemory(level);
                }
            }
        }
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        if (null != mLogicList && mLogicList.size() > 0) {
            for (PriorityLogicWrapper priorityLogicWrapper : mLogicList) {
                if (null != priorityLogicWrapper && null != priorityLogicWrapper.instance) {
                    priorityLogicWrapper.instance.onConfigurationChanged(newConfig);
                }
            }
        }
    }

}
~~~
