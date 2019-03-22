---
layout: post
title: android Google map使用总结
category: accumulation
tags:
  - ANDROID
  - google map
keywords: android, google map
description: google map2.0使用总结
banner: http://cdn.conorlee.top/Chestnut%20Tree%20in%20Blossom.jpg
thumbnail: http://cdn.conorlee.top/Chestnut%20Tree%20in%20Blossom.jpg
---


### 1.在代码中编译google map
要想编译编译google map必须使用Google map api。并在自己的工程中引入google-play-services_lib。两个都需要在SDK Manager中下载。前者要在对应android api中勾选，后者要勾选在Extras下的Google Play Service。

要注意的是map2.0要使用com.google.android.gms包名下的类

Google Maps Android API 使用OpenGL ES第2版来渲染地图。如果未安装OpenGL ES第2版，地图将不会出现。可以在 AndroidManifest.xml 中添加以下<uses-feature>元素作为<manifest>元素的子元素来过滤不能支持的手机：

~~~ Java
<uses-feature
        android:glEsVersion="0x00020000"
        android:required="true"/>
~~~

<!--more-->

### 2.在编码中遇到的问题
1.可以实现OnCameraChangeListener接口，来实现对相机状态的监听，比如我就记录下了地图缩放的大小

~~~ Java
@Override
public void onCameraChange(CameraPosition arg0) {
	// TODO Auto-generated method stub
	zoom = arg0.zoom;
}
~~~

2.向将处理地图的 Activity 添加 Fragment 对象。 最简单的实现方式是，向Activity 的布局文件添加 <fragment> 元素。

3.实现 OnMapReadyCallback 接口，并使用onMapReady(GoogleMap)回调方法获取GoogleMap对象的句柄。GoogleMap对象是对地图本身的内部表示。如需设置地图的视图选项，可以使用UiSettings设置地图的样式。

4.调用Fragment上的getMapAsync()以注册回调。

5.使用手机定位，定位成功后再map上显示标记：

~~~ Java
MarkerOptions markerOpt = new MarkerOptions();  
markerOpt.position(new LatLng(geoLat, geoLng));  
markerOpt.draggable(false);  
markerOpt.visible(true);  
markerOpt.anchor(0.5f, 0.5f);//设为图片中心  
markerOpt.icon(BitmapDescriptorFactory  
	.fromResource(R.drawable.sos_location_38x53));  
mMap.addMarker(markerOpt);  
//将摄影机移动到指定的地理位置  
cameraPosition = new CameraPosition.Builder()  
.target(new LatLng(geoLat, geoLng))              // Sets the center of the map to ZINTUN  
	.zoom(zoom)                  // 缩放比例  
	.bearing(0)                // Sets the orientation of the camera to east  
	.build();                  // Creates a CameraPosition from the builder  
mMap.animateCamera(CameraUpdateFactory.newCameraPosition(cameraPosition));
~~~   

6.实现地图圆角效果：使用圆角.9图片，中间透明，圆角四周不透明<br>
详细可以看这个提问：[Is there a way to implement rounded corners to a Mapfragment?](http://stackoverflow.com/questions/14469208/is-there-a-way-to-implement-rounded-corners-to-a-mapfragment)

7.去掉google地图自带的蓝色圆点
GoogleMap.setMyLocationEnabled(false);

8.解决mapview与scrollview嵌套滑动的问题：
思路就是使用getParent().requestDisallowInterceptTouchEvent(true);方法，让子类接收到touch事件

~~~ Java
public class MyMapView extends MapView {
    private ViewParent mViewParent;

    public MyMapView(Context context) {
        super(context);
    }

    public MyMapView(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public MyMapView(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public MyMapView(Context context, GoogleMapOptions options) {
        super(context, options);
    }


    public void setViewParent(@Nullable final ViewParent viewParent) { //any ViewGroup
        mViewParent = viewParent;
    }

    @Override
    public boolean onInterceptTouchEvent(final MotionEvent event) {
        switch (event.getAction()) {
            case MotionEvent.ACTION_DOWN:
                if (null == mViewParent) {
                    //设置父类不拦截touch事件，子view可以接收到touch事件
                    getParent().requestDisallowInterceptTouchEvent(true);
                } else {
                    mViewParent.requestDisallowInterceptTouchEvent(true);
                }
                break;
            case MotionEvent.ACTION_UP:
                if (null == mViewParent) {
                    //让父类拦截touch事件
                    getParent().requestDisallowInterceptTouchEvent(false);
                } else {
                    mViewParent.requestDisallowInterceptTouchEvent(false);
                }
                break;
            default:
                break;
        }

        return super.onInterceptTouchEvent(event);
    }
}
~~~

9.LocationListener，一直回调到onProviderDisabled

有可能是因为手机没有开启定位服务，解决办法是：

~~~ Java
@Override
public void onProviderDisabled(String provider) {
    isLocatedSuccess = false;
    if (provider.equals("network")) //跳到位置服务设置页面
        startActivity(new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS));
}
~~~

### 3.在真机上测试效果
需要在真机上安装这两个包：com.android.vending.apk（Google play store）和com.google.android.gms.apk（Google play services）
可以在国内应用市场上去搜索最新版本，也可以使用我上传的文件：

链接：http://pan.baidu.com/s/1i5q8jo5 密码：solm

安装成功以后，再运行自己的程序，查看效果了
