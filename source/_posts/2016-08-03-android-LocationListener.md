---
layout: post
title: Android手机定位服务
category: technology
tags:
  - ANDROID
  - LocationListener
keywords: LocationListener
description: Android使用手机定位服务使用，GPS服务不可用则跳转到手机位置服务设置页面
banner: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Acacia%20Branches.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Blossoming%20Acacia%20Branches.jpg
---



### 1.LocationListener使用

优先使用网络定位服务，当GPS服务不可用则跳转到手机位置服务设置页面

<!--more-->

代码收集：

~~~ Java
/**
  * 使用手机定位服务
  */
private void startLocationService(){
    locationListener = new LocationListener() {

        @Override
        public void onStatusChanged(String provider, int status, Bundle extras) {
        }
        @Override
        public void onProviderEnabled(String provider) {
            // 当GPS LocationProvider可用时，更新位置
            location = locManager.getLastKnownLocation(provider);
        }
        @Override
        public void onProviderDisabled(String provider) {
            isLocatedSuccess = false;
            if (provider.equals("network")) {
                    locManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 3 * 1000, 8,locationListener);
            }else if(provider.equals("gps")){//GPS服务不可用，跳到位置服务设置页面
                startActivity(new Intent(android.provider.Settings.ACTION_LOCATION_SOURCE_SETTINGS));
            }else {
                updateToNewLocation(null);
            }
        }
        @Override
        public void onLocationChanged(Location location) {
            // 当定位信息发生改变时，更新位置
            isLocatedSuccess = true;
            updateToNewLocation(location);
            locManager.removeUpdates(this);
        }
    };

    if (locManager.getProvider(LocationManager.NETWORK_PROVIDER) != null)
        locManager.requestLocationUpdates(LocationManager.NETWORK_PROVIDER, 3 * 1000, 8,locationListener);
    else if (locManager.getProvider(LocationManager.GPS_PROVIDER) != null)
        locManager.requestLocationUpdates(LocationManager.GPS_PROVIDER, 3 * 1000, 8,locationListener);
    else Toast.makeText(mActivity(), "获取手机位置信息错误", Toast.LENGTH_SHORT).show();
}
~~~
