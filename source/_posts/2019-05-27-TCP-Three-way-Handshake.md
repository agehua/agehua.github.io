---
layout: post
title: TCP三次握手解读
category: accumulation
tags:
    - TCP
keywords: TCP
banner: http://cdn.conorlee.top/Mike_Tyson_8.jpg
thumbnail: http://cdn.conorlee.top/Mike_Tyson_8.jpg
toc: true
---

`本篇文章节选自知乎问答，部分图片来自网络，文末已给出链接`

网络上有很多关于TCP三次握手的解读和类比，这些都是不全面的，如下：
> 三次握手：
“喂，你听得到吗？”
“我听得到呀，你听得到我吗？”
“我能听到你，今天 balabala……”
<!--more-->
![三次握手简略图](/images/blogimages/2019/Three-way-Handshake1.png)

SYN、ACT是TCP封包中的控制单元，但其实过程中省略了其他封包内容，例如：
![三次握手包含SEQ图](/images/blogimages/2019/Three-way-Handshake-ex2.png)

上面的例子错误，就是因为忽略了SEQ内容。看完下面内容，你就会明白为什么了。

### 三次握手的误解与错误类比(RFC解读)

如果你细读[RFC793](https://www.ietf.org/rfc/rfc793.txt)，也就是 TCP 的协议 RFC，你就会发现里面就讲到了为什么三次握手是必须的——TCP 需要 seq 序列号来做可靠重传或接收，而避免连接复用时无法分辨出 seq 是延迟或者是旧链接的 seq，因此需要三次握手来约定确定双方的 ISN（初始 seq 序列号）。

下面给出详细的 RFC 解读说明：（数据分组称为分段（Segment），国内通常用包来称呼）

我们首先要知道到一点就是， TCP 的可靠连接是靠  seq（ sequence numbers 序列号）来达成的。

> A fundamental notion in the design is that every octet of data sent over a TCP connection has a sequence number.  Since every octet is sequenced, each of them can be acknowledged.  The acknowledgment mechanism employed is cumulative so that an acknowledgment of sequence number X indicates that all octets up to but not including X have been received. 

TCP 设计中一个基本设定就是，通过TCP 连接发送的每一个包，都有一个sequence number。而因为每个包都是有序列号的，所以都能被确认收到这些包。确认机制是累计的，所以一个对sequence number X 的确认，意味着 X 序列号之前(不包括 X) 包都是被确认接收到的。

> The protocol places no restriction on a particular connection being used over and over again.  The problem that arises from this is  -- "how does the TCP identify duplicate segments from previous incarnations of the connection?"  This problem becomes apparent if the connection is being opened and closed in quick succession, or if the connection breaks with loss of memory and is then reestablished.

TCP 协议是不限制一个特定的连接（两端 socket 一样）被重复使用的。所以这样就有一个问题：这条连接突然断开重连后，TCP 怎么样识别之前旧链接重发的包？——这就需要独一无二的 **ISN（初始序列号）** 机制。

> When new connections are created, an initial sequence number (ISN) generator is employed which selects a new 32 bit ISN.  The generator is bound to a (possibly fictitious) 32 bit clock whose low order bit is incremented roughly every 4 microseconds.  Thus, the ISN cycles approximately every 4.55 hours. Since we assume that segments will stay in the network no more than the Maximum Segment Lifetime (MSL) and that the MSL is less than 4.55 hours we can reasonably assume that ISN's will be unique.

当一个新连接建立时，初始序列号（ initial sequence number ISN）生成器会生成一个新的32位的 ISN。

这个生成器会用一个32位长的时钟，差不多4µs 增长一次，因此 ISN 会在大约 4.55 小时循环一次（2^32位的计数器，需要2^32*4 µs才能自增完，除以1小时共有多少µs便可算出2^32*4 /(1*60*60*1000*1000)=4.772185884 ）

而一个段在网络中并不会比最大分段寿命（Maximum Segment Lifetime (MSL) ，默认使用2分钟）长，MSL 比4.55小时要短，所以我们可以认为 ISN 会是唯一的。

发送方与接收方都会有自己的 ISN （下面的例子中就是 X 与 Y）来做双方互发通信，具体的描述如下：

> 1 A --> B  SYN my sequence number is X 
2 A <-- B  ACK your sequence number is X 
3 A <-- B  SYN my sequence number is Y 
4 A --> B  ACK your sequence number is Y

2与3都是 B 发送给 A，因此可以合并在一起，因此成为three way (or three message) handshake（其实翻译为三步握手，或者是三次通信握手更为准确）

因此最终可以得出，三次握手是必须的：
> A three way handshake is necessary because sequence numbers are not tied to a global clock in the network, and TCPs may have different mechanisms for picking the ISN's. The receiver of the first SYN has no way of knowing whether the segment was an old delayed one or not, unless it remembers the last sequence number used on the connection (which is not always possible), and so it must ask the sender to verify this SYN. The three way handshake and the advantages of a clock-driven scheme are discussed in [3].

三次握手（A three way handshake）是必须的， 因为 sequence numbers（序列号）没有绑定到整个网络的全局时钟（全部统一使用一个时钟，就可以确定这个包是不是延迟到的）以及 TCPs 可能有不同的机制来选择 ISN（初始序列号）。

接收方接收到第一个 SYN 时，没有办法知道这个 SYN 是是否延迟了很久了，除非他有办法记住在这条连接中，最后接收到的那个sequence numbers（然而这不总是可行的）。

这句话的意思是：一个 seq 过来了，跟现在记住的 seq 不一样，我怎么知道他是上条延迟的，还是上上条延迟的呢？

所以，接收方一定需要跟发送方确认 SYN。

假设不确认 SYN 中的 SEQ，那么就只有：
> 1 A --> B  SYN my sequence number is X 
2 A <-- B  ACK your sequence number is X  SYN my sequence number is Y

只有B确认了收到了 A 的 SEQ， A 无法确认收到  B 的。也就是说，只有 A 发送给 B 的包都是可靠的， 而 B 发送给 A 的则不是，所以这不是可靠的连接。这种情况如果只需要 A 发送给 B ，B 无需回应，则可以不做三次握手。

所以，正确的类比应该是这样的：

TCP 传递信息可以理解为美国与中国用货船来传货物，但因为一首轮船穿放不下，货物要分开一只只轮船来发货。所以需要一个序列号来识别该货物是第几个，以便到达后将其拼接回原来的货物。因为同一条航道（也就是 tcp连接）上，可能会有多批货物发送（复用 tcp 连接）。发货时，双方需要通知对方这个序列号是从哪里开始（init seq）的，这样才能辨识过来的是不是一个对的货物，以及能拼接成完整的货物。

货物运输拼接（tcp）最重要的是可靠性，如果没有用三次握手来确认双方都可以获得对方的 序列号（seq）的话，就无法知道当前航班（连接）中，对的货物序号是怎么样的了。


### 三次握手详细过程

> |* TCP A  *|* TCP B *|
|:--------|:-------:|
|1.  CLOSED                 |                    LISTEN|
|2.  SYN-SENT    --> &lt;SEQ=100&gt;&lt;CTL=SYN&gt;           |    --> SYN-RECEIVED|
|3.  ESTABLISHED <-- &lt;SEQ=300&gt;&lt;ACK=101&gt;&lt;CTL=SYN,ACK&gt; | <-- SYN-RECEIVED|
|4.  ESTABLISHED --> &lt;SEQ=101&gt;&lt;ACK=301&gt;&lt;CTL=ACK&gt;      | --> ESTABLISHED|
|5.  ESTABLISHED --> &lt;SEQ=101&gt;&lt;ACK=301&gt;&lt;CTL=ACK&gt;&lt;DATA&gt; |--> ESTABLISHED|
Basic 3-Way Handshake for Connection Synchronization
Figure 7.

在上图
- 第二行中， A 发送了 SEQ 100，标志位是 SYN.
- 第三行，B 发回了 ACK 101 与 SEQ 300，标志位是 SYN 与 ACK（两个过程合并了）。注意，ACK 是101意味着，B 希望接收到 101序列号开始的数据段。
- 第四行，A 返回了空的数据，SEQ 101， ACK 301，标志位为 ACK。至此，双方的开始 SEQ （也就是 ISN）号100与300都被确认接收到了。
- 第五行，开始正式发送数据包，注意的是 ACK 依旧是第四行的301，因为没有需要 ACK 的 SYN 了（第四行已经 ACK 完）。

以上，4 最后这个确认的过程，是可以带上数据的。

> The principle reason for the three-way handshake is to prevent old duplicate connection initiations from causing confusion.  
To deal with this, a special control message, reset, has been devised.  
If the receiving TCP is in a  non-synchronized state (i.e., SYN-SENT, SYN-RECEIVED), it returns to LISTEN on receiving an acceptable reset. 
If the TCP is in one of the synchronized states (ESTABLISHED, FIN-WAIT-1, FIN-WAIT-2, CLOSE-WAIT, CLOSING, LAST-ACK, TIME-WAIT), it aborts the connection and informs its user.  We discuss this latter case under "half-open" connections below.

三次握手的原则设计是防止旧复用链接的初始化导致问题，为了解决此问题，我们设计了reset这个特别的控制信号来处理。

如果接收中的 TCP 在一个未同步状态如 SYN-SENT, SYN-RECEIVED，它会返回 reset 给对方。

如果 TCP 是同步状态中如(ESTABLISHED, FIN-WAIT-1, FIN-WAIT-2, CLOSE-WAIT, CLOSING, LAST-ACK, TIME-WAIT)，他会终止此连接并通知用户。

看起来有点绕，我们举个图例看看：

> |* TCP A  *|* TCP B *|
|:--------|:-------:|
|1.  CLOSED               |                                LISTEN|
|2.  SYN-SENT    --> &lt;SEQ=100&gt;&lt;CTL=SYN&gt;            |   ... |
|3.  (duplicate) ... &lt;SEQ=90&gt;&lt;CTL=SYN&gt;            |  --> SYN-RECEIVED |
|4.  SYN-SENT    <-- &lt;SEQ=300&gt;&lt;ACK=91&gt;&lt;CTL=SYN,ACK&gt; | <-- SYN-RECEIVED |
|5.  SYN-SENT    --> &lt;SEQ=91&gt;&lt;CTL=RST&gt;            |  --> LISTEN |
|6.  &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&emsp;&emsp;&emsp;&emsp;   ... &lt;SEQ=100&gt;&lt;CTL=SYN&gt;     |         --> SYN-RECEIVED  |
|7.  SYN-SENT    <-- &lt;SEQ=400&gt;&lt;ACK=101&gt;&lt;CTL=SYN,ACK&gt;  | <-- SYN-RECEIVED |
|8.  ESTABLISHED --> &lt;SEQ=101&gt;&lt;ACK=401&gt;&lt;CTL=ACK&gt;     | --> ESTABLISHED |
                    Recovery from Old Duplicate SYN

这是 复用连接时，旧在途包发往新连接中的例子。
- 3中，一个旧的重复的 SYN到达 B。 
- 4中， B分别不出是否旧的，照样子正常回包。
- 5中，A检测到 B 返回的ACK不正确，所以返回 RST(reset)
- 6中，B接收到  RST(reset)信号，于是变成 LISTEN 状态。
- 7中，新连接正常的 SYN终于到达了，三次握手正常进行。

这种是简化的情况，但是可以看出 TCP 是如何处理复用旧链接的包到达的。

### Ref
[TCP 为什么是三次握手，而不是两次或四次？](https://www.zhihu.com/question/24853633/answer/573627478)

[TCP 三向交握 (Three-way Handshake)](https://notfalse.net/7/three-way-handshake)
