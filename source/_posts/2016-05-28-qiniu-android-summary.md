---
layout: post
title: 七牛android使用总结
category: technology
tags:
  - ANDROID
  - qiniu
keywords: qiniu, android
description: 七牛android使用总结，包括list，delete，upload，download
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Avenue%20of%20Plane%20Trees%20near%20Arles%20Station.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Avenue%20of%20Plane%20Trees%20near%20Arles%20Station.jpg

---



### 1.在android上实现对七牛空间操作

  在android上实现对七牛空间的各种操作，包括list，delete，upload，download。支持私有空间

  注意：**官方不建议开发者把AccessKey和SecretKey放在前端的java文件里**，最好还是有一台应用服务器

  如果只是想尝试一下，好吧:)  代码中都有说明，直接上代码

<!--more-->

### 2.代码

一共有三个类：

- 工具类：

~~~ Java
 package com.qiniu.ui.utils;

 import android.graphics.Bitmap;
 import android.graphics.Bitmap.CompressFormat;
 import android.os.Environment;
 import android.util.Log;
 import com.loopj.android.http.AsyncHttpClient;
 import com.loopj.android.http.AsyncHttpResponseHandler;
 import com.loopj.android.http.RequestParams;
 import com.qiniu.android.storage.UpProgressHandler;
 import com.qiniu.android.storage.UploadManager;
 import com.qiniu.android.storage.UploadOptions;
 import com.qiniu.android.utils.UrlSafeBase64;
 import com.qiniu.api.auth.AuthException;
 import com.qiniu.api.auth.digest.Mac;
 import com.qiniu.api.rs.PutPolicy;
 import com.qiniu.ui.SHA;
 import com.qiniu.ui.contains.QiNiuConfig;
 import org.apache.http.Header;
 import org.json.JSONException;
 import java.io.BufferedOutputStream;
 import java.io.FileOutputStream;
 import java.util.Random;
 import static com.qiniu.ui.SHA.hMacSHA1Encrypt;


 /**
  * @date 2016年5月26日 上午11:00:43
  * @author lijixin
  * @web https://agehua.github.io
  * @Description: 七牛云图片操作
  */
 public class QiniuUitls {

 	private static final String fileName = "temp.jpg";
 	private static final String tempJpeg = Environment.getExternalStorageDirectory().getPath() + "/" + fileName;

 	private static int maxWidth = 720;
 	private static int maxHeight = 1080;

 	public interface QiniuUploadUitlsListener {
 		void onSucess(String fileUrl);
 		void onError(int errorCode, String msg);
 		void onProgress(int progress);
 	}

 	public interface QiniuRequestListener {
 		void onSucceed(byte[] bytes);
 		void onFailed(byte[] bytes);
 	}

 	/**
 	 * 将bitmap转换成jpeg，控制图片大小不大于720*1080，可以指定图片压缩质量
 	 * @param bitmap
 	 * @param filePath
 	 * @param quality
 	 * @return
 	 */
 	public static boolean saveBitmapToJpegFile(Bitmap bitmap, String filePath, int quality) {
 		try {
 			FileOutputStream fileOutStr = new FileOutputStream(filePath);
 			BufferedOutputStream bufOutStr = new BufferedOutputStream(fileOutStr);
 			resizeBitmap(bitmap).compress(CompressFormat.JPEG, quality, bufOutStr);
 			bufOutStr.flush();
 			bufOutStr.close();
 		} catch (Exception exception) {
 			return false;
 		}
 		return true;
 	}

 	/**
 	 * 缩小图片
 	 *
 	 * @param bitmap
 	 * @return
 	 */
 	public static Bitmap resizeBitmap(Bitmap bitmap) {
 		if (bitmap != null) {
 			int width = bitmap.getWidth();
 			int height = bitmap.getHeight();
 			//宽度大于720
 			if (width > maxWidth) {
 				//按宽度等比例压缩图片
 				int pWidth = maxWidth;
 				int pHeight = maxWidth * height / width;
 				Bitmap result = Bitmap.createScaledBitmap(bitmap, pWidth, pHeight, false);
 				bitmap.recycle();
 				return result;
 			}
 			if (height > maxHeight) {
 				//按高度等比例缩小图片
 				int pHeight = maxHeight;
 				int pWidth = maxHeight * width / height;
 				Bitmap result = Bitmap.createScaledBitmap(bitmap, pWidth, pHeight, false);
 				bitmap.recycle();
 				return result;
 			}
 		}
 		return bitmap;
 	}

 	public static void uploadImage(Bitmap bitmap, QiniuUploadUitlsListener listener) {
 		saveBitmapToJpegFile(bitmap, tempJpeg,100);
 		uploadImage(tempJpeg, listener);
 	}

 	/**
 	 * 上传图片选择jpg格式，七牛图片api目前支持对jpg格式进行指定图片质量请求
 	 * @param filePath
 	 * @param listener
 	 */
 	public static void uploadImage(String filePath, final QiniuUploadUitlsListener listener) {
 		final String fileUrlUUID = getFileUrlUUID();
 		String token = getToken();
 		if (token == null) {
 			if (listener != null) {
 				listener.onError(-1, "token is null");
 			}
 			return;
 		}
 		UploadManager uploadManager = new UploadManager();
 		uploadManager.put(filePath, fileUrlUUID, token, (key, info, response) -> {
 			System.out.println("debug:info = " + info + ",response = " + response);
 			if (info != null && info.statusCode == 200) {// 上传成功
 				String fileRealUrl = getRealUrl(fileUrlUUID);
 				System.out.println("debug:fileRealUrl = " + fileRealUrl);
 				if (listener != null) {
 					listener.onSucess(fileRealUrl);
 				}
 			} else {
 				if (listener != null) {
 					listener.onError(info.statusCode, info.error);
 				}
 			}
 		}, new UploadOptions(null, null, false, new UpProgressHandler() {
 			public void progress(String key, double percent) {
 				if (listener != null) {
 					listener.onProgress((int) (percent * 100));
 				}
 			}
 		}, null));
 	}

 	/**
 	 * 生成远程文件路径（全局唯一）
 	 * <p>格式类似：	 H60-L12__1464851303930__156750_1884</p>
 	 * @return
 	 */
 	private static String getFileUrlUUID() {
 		String filePath = android.os.Build.MODEL + "__" + System.currentTimeMillis() + "__" + (new Random().nextInt(500000))
 				+ "_" + (new Random().nextInt(10000));
 		return filePath.replace(".", "0");
 	}

 	private static String getRealUrl(String fileUrlUUID) {
 		String filePath = "http://" + QiNiuConfig.QINIU_BUCKNAME + ".qiniudn.com/" + fileUrlUUID;
 		return filePath;
 	}

 	/**
 	 * 获取token 本地生成
 	 *
 	 * @return
 	 */
 	private static String getToken() {
 		Mac mac = new Mac(QiNiuConfig.QINIU_AK, QiNiuConfig.QINIU_SK);
 		PutPolicy putPolicy = new PutPolicy(QiNiuConfig.QINIU_BUCKNAME);
 		putPolicy.returnBody = "{\"name\": $(fname),\"size\": \"$(fsize)\",\"w\": \"$(imageInfo.width)\",\"h\": \"$(imageInfo.height)\",\"key\":$(etag)}";
 		try {
 			String uptoken = putPolicy.token(mac);
 			System.out.println("debug:uptoken = " + uptoken);
 			return uptoken;
 		} catch (AuthException e) {
 			e.printStackTrace();
 		} catch (JSONException e) {
 			e.printStackTrace();
 		}
 		return null;
 	}

 	/**
 	 * 根据host和文件名，生成file的url下载地址，支持私有空间
 	 * @param domain host：七牛的私有空间域名
 	 * @param imgKey file key：空间里的文件名
 	 * @return url下载地址
 	 */
 	public static String downloadFile(String domain,String imgKey){
 		//密钥配置
 		//构造私有空间的需要生成的下载的链接
 //		String domain ="http://example.xxx.clouddn.com/";
 //		String path ="H60-L12__1464917382714__36888_5255";
 		StringBuilder sb =new StringBuilder();
 		//加上过期时间戳字段
 		String url = sb.append(domain).append(imgKey).append("?e=1478365261").toString();
 		Log.e("encodedEntryURI",url);

 		byte[] sign =null;
 		try {
 			sign = SHA.hMacSHA1Encrypt(url,QiNiuConfig.QINIU_SK);
 		} catch (Exception e) {
 			e.printStackTrace();
 		}
 		String encodedSign = UrlSafeBase64.encodeToString(sign);
 		sb.append("&token=").append(QiNiuConfig.QINIU_AK).append(":").append(encodedSign);
 		Log.e("download token",sb.toString());
 		return sb.toString();
 	}

 	/**
 	 * 列出空间中指定格式的文件
 	 * @param bucket 空间名
 	 * @param prefix 要指定的格式（前缀）
 	 * @param listener
 	 * @return
 	 */
 	public static void listFile(String bucket, String prefix,final QiniuRequestListener listener){

 		try{
 			StringBuilder sb = new StringBuilder();

 			String entryUrl = sb.append("bucket=").append(bucket)
 					.append("&prefix=").append(prefix).toString();
 			String host = "http://rsf.qbox.me";
 			String path = "/list?" +entryUrl;
 			String url = host+path;
 			Log.e("AAAAAAA", url);
 			byte[] sign = SHA.hMacSHA1Encrypt(path+"\n", QiNiuConfig.QINIU_SK);
 			String encodedSign = UrlSafeBase64.encodeToString(sign);
 			String authorization = QiNiuConfig.QINIU_AK + ':' + encodedSign;
 			AsyncHttpClient client = new AsyncHttpClient();
 			client.addHeader("Content-Type","application/x-www-form-urlencoded");
 			client.addHeader("Authorization", "QBox "+authorization);
 			RequestParams params = new RequestParams();
 			client.post(url, params, new AsyncHttpResponseHandler() {
 				@Override
 				public void onSuccess(int i, Header[] headers, byte[] bytes) {
 					if (null!=listener)
 						listener.onSucceed(bytes);
 				}

 				@Override
 				public void onFailure(int i, Header[] headers, byte[] bytes, Throwable throwable) {
 					if (null!=listener)
 						listener.onFailed(bytes);
 				}
 			});
 		}catch(Exception e){
 			e.printStackTrace();
 		}
 		return ;
 	}

 	/**
 	 * 删除空间中的文件
 	 * @param bucket 删除文件的空间
 	 * @param fileName 删除的文件
 	 * */
 	public static boolean deleteFile(String bucket,String fileName,final QiniuRequestListener listener){
 		try{
 			String entryUrl = bucket+":"+fileName;
 			String encodedEntryURI = UrlSafeBase64.encodeToString(entryUrl.getBytes());
 			String host = "http://rs.qiniu.com";
 			String path = "/delete/"+encodedEntryURI;
 			String url = host+path;
 			Log.e("AAAAAAA", url);
 			byte[] sign = hMacSHA1Encrypt(path+"\n", QiNiuConfig.QINIU_SK);
 			String encodedSign = UrlSafeBase64.encodeToString(sign);
 			String authorization = QiNiuConfig.QINIU_AK + ':' + encodedSign;

 			AsyncHttpClient client = new AsyncHttpClient();
 			client.addHeader("Content-Type","application/x-www-form-urlencoded");
 			client.addHeader("Authorization", "QBox "+authorization);
 			RequestParams params = new RequestParams();
 			client.post(url, params, new AsyncHttpResponseHandler() {
 				@Override
 				public void onSuccess(int i, Header[] headers, byte[] bytes) {
 					if (null!=listener)
 						listener.onSucceed(bytes);
 				}

 				@Override
 				public void onFailure(int i, Header[] headers, byte[] bytes, Throwable throwable) {
 					if (null!=listener)
 						listener.onFailed(bytes);
 					if (null!=bytes) {
 						String s = new String(bytes);
 					}
 				}
 			});

 		}catch(Exception e){
 			e.printStackTrace();
 		}
 		return false;
 	}

 }
~~~

- HMAC-SHA1签名加密类

使用下面方法生成对应七牛资源管理里用到的[管理凭证](http://developer.qiniu.com/article/developer/security/access-token.html)

~~~ Java
/**
 * 对外提供HMAC-SHA1签名方法
 * @author agehua
 *
 */
public class SHA {

    private static final String MAC_NAME = "HmacSHA1";
    private static final String ENCODING = "UTF-8";

    /**
     *
     * 使用 HMAC-SHA1 签名方法对对encryptText进行签名
     * @param encryptText 被签名的字符串
     * @param encryptKey 密钥
     * @return
     * @throws Exception
     */
    public static byte[] hMacSHA1Encrypt(String encryptText, String encryptKey)
            throws Exception {
        byte[] data = encryptKey.getBytes(ENCODING);
        // 根据给定的字节数组构造一个密钥,第二参数指定一个密钥算法的名称
        SecretKey secretKey = new SecretKeySpec(data, MAC_NAME);
        // 生成一个指定 Mac 算法 的 Mac 对象
        Mac mac = Mac.getInstance(MAC_NAME);
        // 用给定密钥初始化 Mac 对象
        mac.init(secretKey);
        byte[] text = encryptText.getBytes(ENCODING);
        // 完成 Mac 操作
        return mac.doFinal(text);
    }
}  
~~~  

- 还有一个Config文件

~~~ Java
public final class QiNiuConfig {
	public static final String token = getToken();
	public static final String QINIU_AK = "Your_AccessKey";
	public static final String QINIU_SK = "Your_SecretKey";
	public static final String QINIU_BUCKNAME = "你的私有空间";

	public static String getToken() {
		Mac mac = new Mac(QiNiuConfig.QINIU_AK, QiNiuConfig.QINIU_SK);
		PutPolicy putPolicy = new PutPolicy(QiNiuConfig.QINIU_BUCKNAME);
		putPolicy.returnBody = "{\"name\": $(fname),\"size\": \"$(fsize)\",\"w\": \"$(imageInfo.width)\",\"h\": \"$(imageInfo.height)\",\"key\":$(etag)}";
		try {
			String uptoken = putPolicy.token(mac);
			System.out.println("debug:uptoken = " + uptoken);
			return uptoken;
		} catch (AuthException e) {
			e.printStackTrace();
		} catch (JSONException e) {
			e.printStackTrace();
		}
		return null;
	}
}

~~~

 [点击在Gist上查看上面这些代码](https://gist.github.com/agehua/f73bcb98af2c0c44a4c49c7c7b7e6bda)

最后，如果有问题欢迎讨论，我的邮箱简介里有 :)  
