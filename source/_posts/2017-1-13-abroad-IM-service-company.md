---
layout: post
title:  IM 国外公司，服务费，方案
category: accumulation
tags: IM Company
keywords: instant messaging, IM Company
banner: http://obxk8w81b.bkt.clouddn.com/Cottages%20with%20a%20Woman%20Working%20in%20the%20Foreground.jpg
thumbnail: http://obxk8w81b.bkt.clouddn.com/Cottages%20with%20a%20Woman%20Working%20in%20the%20Foreground.jpg
toc: true
---

### 一、海外提供IM服务的公司

#### 1.1 Snich
官网： https://www.sinch.com/products/instant-messaging/  
收费介绍
![Snich_pricing](/images/blogimages/2017/im_company_abroad/im_snich.png)

<!--more-->

#### 1.2 quickblox

官网：https://quickblox.com/
收费介绍
![quickblox](/images/blogimages/2017/im_company_abroad/im_quickblox.png)

#### 1.3 Layer
https://layer.com/use-cases

具体价格需要注册，
![Layer](/images/blogimages/2017/im_company_abroad/im_layer.png)
然后注册的邮箱里会收到一封邮件：
![](/images/blogimages/2017/im_company_abroad/im_layer_email.png)

#### 1.4sendbird
https://sendbird.com

收费介绍
![sendbird](/images/blogimages/2017/im_company_abroad/im_sendbird.png)

#### 1.5 applozic
https://www.applozic.com/
收费介绍
![applozic](/images/blogimages/2017/im_company_abroad/im_applozic.png)

### 二、各家IM服务公司比较
![im_comparation](/images/blogimages/2017/im_company_abroad/im_comparation.png)

图片来源：[Which Chat SDK is better for user experience? Applozic, Sendbird, Layer or Quickblox!](https://www.applozic.com/blog/applozic-vs-sendbird-vs-layer-vs-quickblox-alternatives/a)


#### 2.1 QuickBlox vs Applozic

这部分内容转载自[stackoverflow](http://stackoverflow.com/questions/37678528/comparing-layer-applozic-quickblox-sendbird-for-quick-whatsapp-like-messagin)

I have recently just tested two of the solutions you have cited.

**QuickBlox:**
- protocol xmpp
- open source
- documentated less clearly than applozic
- easy set up

**Applozic:**
- protocol mqtt
- open source easy set up, if a bit laborious
- well documented, and very easy API to use.
- Very customizable.

**Both programs:**
- have video call/audio call/groups
- have code bases that are constantly being updated, and from their Github pages new version have been released in the past few months

> I preferred Applozic as the user interface was nicer and easier to use, and the features and pricing clearer. I also preferred the API and the setup code was easier.


#### 2.2 Quickblox vs SendBird vs Layer

本部分内容转载自[Do I need to have my own backend?](https://www.quora.com/Quickblox-vs-SendBird-vs-Layer-com-Do-I-need-to-have-my-own-backend)

My research on Quickblox vs SendBird vs Layer.com is:

- 1.Backend-as-a-Service (BaaS) or Chat-as-a-Service (CaaS):
  - Quickblox: Both, BaaS and CaaS
  - SendBird (aka JIVER): CaaS (BaaS for Enterprise-only)
  - Layer.com: CaaS only
- 2.WebSocket-based:
  - Quickblox: No (XMPP)
  - SendBird: Yes
  - Layer.com: Yes
- 3.Voice and Video:
  - Quickblox: Yes
  - SendBird: No
  - Layer.com: No
- 4.Chat UI Kit:
  - Quickblox: QMChatViewController
  - SendBird: Open-source Sample UIs for iOS, Android, JavaScript (Web & React Native), Xamarin, Unity (You can use their Sample UI or 3rd party UI Kit like JSQMessagesViewController)
  - Layer.com: Atlas
- 5.Number of Members in a Group:
  - Quickblox: Thousands (you may need to host Quickblox BaaS at a dedicated AWS infrastructure for better performance)
  - SendBird: Thousands (no need for an extra infrastructure)
  - Layer.com: limited to 25 per group
- 6.The ability to host the BaaS/CaaS at your own IaaS:
  - Quickblox: Yes (setup cost starts from $599)
  - SendBird: Yes (limited to Enterprise plans only)
  - Layer.com: No
- 7.Custom Objects Backend (the need for extra infrastructure):
  - Quickblox: Included in Quickblox BaaS
  - SendBird: Partial support (Key/value custom object stores for channels to be released in March)
  - Layer.com: No
- 8.BaaS/CaaS Admin Panel/Dashboard:
  - Quickblox: Basic with Custom Dashboard when subscribed to the enterprise plan, starts from $599/month (Classic UI)
  - SendBird: Advanced (Modern UI)
  - Layer.com: Basic (Modern UI)
- 9.Attachment max file size:
  - Quickblox: Unknown
  - SendBird: 25MB per file (custom limit for Enterprise plans)
  - Layer.com: 2GB (be aware that their pricing is based on the data transfer)
- 10.Availability SLA:
  - Quickblox: Based on AWS EC2 SLA (99.95%)
  - SendBird: 99.9%
  - Layer.com: 99.9%
- 11.Pricing (per month):
  - Quickblox: Mainly based on the number of messengers per second (/s) and monthly active users: Free-$49-$214-$599-$1,199-$2,399 (Plans) 
  - SendBird: Mainly based on the number of monthly active users (MAU): Free-$59-$179-$599-Custom (https://sendbird.com/pricing) 
  - Layer.com: Mainly based on the monthly data transfer and monthly users: Free-$599 per month-Custom (https://layer.com/plans)
- 12.Number of freelancers at UpWork.com who have this Baas/CaaS as one of their skill set:
  - Quickblox: 547 (keyword: "Quickblox")
  - SendBird: 0 (keyword: "SendBird" or "JIVER") « new to the market
  - Layer: 13 (keyword: "Layer.com" not Layer)

#### 2.3 更多比较：

https://www.quora.com/Quickblox-vs-SendBird-vs-Layer-com-Do-I-need-to-have-my-own-backend

http://stackoverflow.com/questions/37678528/comparing-layer-applozic-quickblox-sendbird-for-quick-whatsapp-like-messagin

https://www.applozic.com/blog/applozic-vs-sendbird-vs-layer-vs-quickblox-alternatives/

https://siftery.com/product-comparison/sendbird-vs-quickblox-vs-layer

https://stackshare.io/stackups/sendbird-vs-sinch-vs-layer

https://www.quora.com/What-SDKs-APIs-are-viable-for-implementing-instant-messaging-between-clients-Android-iOS-and-web
