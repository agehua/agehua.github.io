---
layout: post
title: google cloud message（GCM）和Azure实现Notification总结
category: accumulation
tags:
  - ANDROID
  - GCM
  - azure
  - notification
keywords: gcm, azure, notification
description: google cloud message（GCM）和Azure实现Notification总结
banner: http://cdn.conorlee.top/Country%20Lane%20with%20Two%20Figures.jpg
thumbnail: http://cdn.conorlee.top/Country%20Lane%20with%20Two%20Figures.jpg

---


### 1.相关资料
我也是一知半解，基本上就是根据官方教程来实现，但microsoft的azure文档我找了好久，英文不好，汗。。。

gcm start: https://developers.google.com/cloud-messaging/android/start

azure start: https://azure.microsoft.com/zh-cn/documentation/articles/notification-hubs-android-get-started/

gcm official demo: https://github.com/google/gcm

gcm personal demo: https://github.com/iammert/FastGCM

<!--more-->

### 2.遇到的问题
- 1.手机运行官方demo时，发送消息，消息收不到必须切换一下网络才可以

stackoverflow上有人问过这个问题：http://stackoverflow.com/questions/13835676/google-cloud-messaging-messages-sometimes-not-received-until-network-state-cha


### 3.onActivityResult()和onResume()调用顺序
API中这样描述：当你一个Activity是以请求码开始，结束时返回给前页面结果码，页面根据结果码进行相应的信息处理。我们会在返回的页面先接受结果码，然后才调用onResume()。

通常我们还会遇到这样一个问题：
在处理返回页面的数据问题
1.需要从服务器上刷新数据时我们会在onResume()方法里处理
2.而刷新从结束界面返回的数据我们会在onAcitviyResult()方法里面处理

为了避免二者在同一块控件上对数据处理，我们只需加个标识符，在两个方法里进行判断，要用哪个方法进行刷新

### 4.Android4.4以上系统根据Uri正确获取文件路径的方法

~~~ Java

public static String getPhotoPathFromContentUri(Context context, Uri uri) {
    String photoPath = "";
    if(context == null || uri == null) {
        return photoPath;
    }

    if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.KITKAT && DocumentsContract.isDocumentUri(context, uri)) {
        String docId = DocumentsContract.getDocumentId(uri);
        if(isExternalStorageDocument(uri)) {
            String [] split = docId.split(":");
            if(split.length >= 2) {
                String type = split[0];
                if("primary".equalsIgnoreCase(type)) {
                    photoPath = Environment.getExternalStorageDirectory() + "/" + split[1];
                }
            }
        }
        else if(isDownloadsDocument(uri)) {
            Uri contentUri = ContentUris.withAppendedId(Uri.parse("content://downloads/public_downloads"), Long.valueOf(docId));
            photoPath = getDataColumn(context, contentUri, null, null);
        }
        else if(isMediaDocument(uri)) {
            String[] split = docId.split(":");
            if(split.length >= 2) {
                String type = split[0];
                Uri contentUris = null;
                if("image".equals(type)) {
                    contentUris = MediaStore.Images.Media.EXTERNAL_CONTENT_URI;
                }
                else if("video".equals(type)) {
                    contentUris = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                }
                else if("audio".equals(type)) {
                    contentUris = MediaStore.Audio.Media.EXTERNAL_CONTENT_URI;
                }
                String selection = MediaStore.Images.Media._ID + "=?";
                String[] selectionArgs = new String[] { split[1] };
                photoPath = getDataColumn(context, contentUris, selection, selectionArgs);
            }
        }
    }
    else if("file".equalsIgnoreCase(uri.getScheme())) {
        photoPath = uri.getPath();
    }
    else {
        photoPath = getDataColumn(context, uri, null, null);
    }

    return photoPath;
}

private static boolean isExternalStorageDocument(Uri uri) {
    return "com.android.externalstorage.documents".equals(uri.getAuthority());
}

private static boolean isDownloadsDocument(Uri uri) {
    return "com.android.providers.downloads.documents".equals(uri.getAuthority());
}

private static boolean isMediaDocument(Uri uri) {
    return "com.android.providers.media.documents".equals(uri.getAuthority());
}

private static String getDataColumn(Context context, Uri uri, String selection, String[] selectionArgs) {
    Cursor cursor = null;
    String column = MediaStore.Images.Media.DATA;
    String[] projection = { column };
    try {
        cursor = context.getContentResolver().query(uri, projection, selection, selectionArgs, null);
        if (cursor != null && cursor.moveToFirst()) {
            int index = cursor.getColumnIndexOrThrow(column);
            return cursor.getString(index);
        }
    } finally {
        if (cursor != null && !cursor.isClosed())
                cursor.close();
    }
    return null;
}

~~~
