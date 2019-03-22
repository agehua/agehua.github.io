---
layout: post
title:  PBOC知识收集（干货）
category: accumulation
tags:
    - PBOC
    - PBOC 2.0
    - Basic Knowledge
keywords: pboc, pboc2.0
banner: http://cdn.conorlee.top/Daubigny%20s%20Garden.jpg
thumbnail: http://cdn.conorlee.top/Daubigny%20s%20Garden.jpg
toc: true
---

## PBOC规范基础知识（干货）
摘自：《中国集成电路（IC）卡规范第2部分：电子钱包电子存折应用规范》
### 消费交易
消费交易允许持卡人使用电子存折或电子钱包的余额进行购物或获取服务。此交易可以在销售点终端(POS)上脱机进行。使用电子存折进行的消费交易必须提交个人识别码(PIN)，使用电子钱包则不需要。

#### 发出初始化消费(INITIALIZE FOR PURCHASE)命令
指令字节：

<!--more-->
~~~ Java
      命令                         CLA  INS   P1   P2
消费初始化(INITIALIZE FOR PURCHASE) ‘80’ ‘50’ ‘01’ ‘0X’
~~~
> 上面的指令字节，其实就是APDU（Application Protocol data unit), 是智能卡与智能卡读卡器之间传送的信息单元

APDU的格式为：**CLA    INS  P1  P2  Lc  Data  Le**
其中CLA为指令类别；INS为指令码；P1、P2为参数；Lc为Data的长度；Le为希望响应时回答的数据字节数，0表最大可能长度。

COS命令由具体应用分为4种命令报文结构如下：
① 情形1：CLA INS P1 P2 00
② 情形2：CLA INS P1 P2 Le
③ 情形3：CLA INS P1 P2 Lc Data
④ 情形4：CLA INS P1 P2 Lc Data Le

而消费交易初始化的具体命令如下：

![](/images/blogimages/2017/Initialize_for_purchase.png)
![](/images/blogimages/2017/Initialize_for_purchase_data.png)


响应报文结构：
~~~ Java
响应数据  响应状态码
Data      SW1 SW1
~~~
DATA： 返回给用户的数据，即命令的执行结果。
SW1、SW2： 返回命令处理的状态。
> 如果命令执行不成功，则只在响应报文中回送SW1和SW2。命令执行成功的状态字一般是“9000”，但个别卡商在执行成功后，返回61 xx，这里表示还有xx字节需要返回，需要再使用卡商提供的GET REPONSE命令接收从IC卡中传递向读卡设备的数据。

消费交易初始化执行成功，返回的响应报文，总长度是（Le 0F）15个字节：

![](/images/blogimages/2017/Initialize_for_purchase_responce.png)

> 一个字节是8bit，4个bit用一个16进制表示（0-9 A-F）。所以一个字节可以由两个16进制表示

#### VERIFY 命令
定义和范围
VERIFY命令用于校验命令数据域中的个人识别码的正确性。
如PIN文件位于某一应用下，当此应用被锁定时，禁止校验PIN;如PIN文件位于MF下，当应用被锁 定后可以执行校验PIN命令。

命令报文

|*代码*|*值*|
|:--------:|:-------:|
|CLA|‘00’|
|INS|‘20’|
|P1|‘00’|
|P2|‘00’|
|Lc|可变|
|Data|外部输入的个人识别码|
|Le|不存在|

P2=’00’表示无特殊限定符被使用。在IC卡上，VERIFY命令在处理过程中应明确知道如何去寻找个人识别码。
> 这里外部输入的PIN码长度是，4到12位（2-6字节），响应报文数据域不存在。

此命令执行成功的状态字是“9000”。
当前的应用选择中，命令数据域中外部输入的个人识别码与卡中存放的个人识别码校验失败时，IC 卡将回送SW2=’Cx’，其中’x’表示个人识别码允许重试的次数;当卡回送’C0’时，表示不能重试个人识别 码。此时再使用VERIFY命令时，将回送失败状态字SW1 SW2=“6983”。

IC卡可能回送的警告状态字见表48。
![](/images/blogimages/2017/pin_verify.png)

### 工具代码
下面贴一些转换工具代码：
~~~ Java
/**
  * 把16进制字符串转换成字节数组
  *
  * @param hex
  * @return
  */
public static byte[] hexStringToByte(String hex) {
    int len = (hex.length() / 2);
    byte[] result = new byte[len];
    char[] achar = hex.toCharArray();
    for (int i = 0; i < len; i++) {
        int pos = i * 2;
        result[i] = (byte) (toByte(achar[pos]) << 4 | toByte(achar[pos + 1]));
    }
    return result;
}

private static byte toByte(char c) {
    byte b = (byte) "0123456789ABCDEF".indexOf(c);
    return b;
}

/**
 * 把字节数组转换成16进制字符串
 *
 * @param bArray
 * @return
 */
public static final String bytesToHexString(byte[] bArray) {
    if(bArray == null )
    {
        return "";
    }
    StringBuffer sb = new StringBuffer(bArray.length);
    String sTemp;
    for (int i = 0; i < bArray.length; i++) {
        sTemp = Integer.toHexString(0xFF & bArray[i]);
        if (sTemp.length() < 2)
            sb.append(0);
        sb.append(sTemp.toUpperCase());
    }
    return sb.toString();
}
~~~

IC卡**交易金额转换**（以分为单位，输出byte[]）
~~~ Java
public static String IntToHex(int n){
  char[] ch = new char[20];
  int nIndex = 0;
    while ( true ){
        int m = n/16;
        int k = n%16;
        if ( k == 15 )
            ch[nIndex] = 'F';
        else if ( k == 14 )
            ch[nIndex] = 'E';
        else if ( k == 13 )
            ch[nIndex] = 'D';
        else if ( k == 12 )
            ch[nIndex] = 'C';
        else if ( k == 11 )
            ch[nIndex] = 'B';
        else if ( k == 10 )
            ch[nIndex] = 'A';
        else
            ch[nIndex] = (char)('0' + k);
        nIndex++;
        if ( m == 0 )
            break;
        n = m;
    }
    StringBuffer sb = new StringBuffer();
    sb.append(ch, 0, nIndex);
    sb.reverse();
    String strHex = new String("");
    strHex += sb.toString();
    return strHex;
}

public static byte[] hexStringToByteArray(String s) {
    if (TextUtils.isEmpty(s))
        return null;
    int len = s.length();
    String append= "";
    switch (len){
        case 1:
            s = "0000000"+s;
            break;
        case 2:
            s = "000000"+s;
            break;
        case 3:
            s = "00000"+s;
            break;
        case 4:
            s = "0000"+s;
            break;
        case 5:
            s = "000"+s;
            break;
        case 6:
            s = "00"+s;
            break;
        case 7:
            s = "0"+s;
            break;
        case 8:
            break;
    }
    len = s.length();
    byte[] b = new byte[len / 2];
    for (int i = 0; i < len; i += 2) {
        // 两位一组，表示一个字节,把这样表示的16进制字符串，还原成一个字节
        b[i / 2] = (byte) ((Character.digit(s.charAt(i), 16) << 4) + Character
                .digit(s.charAt(i + 1), 16));
    }
    return b;
}

/**
 * 交易金额转换
 */
public static byte[] intTo4HexByte(int num){
    return hexStringToByteArray(IntToHex(num));
}
~~~

Hex字符串转换为GBK
~~~ Java
public static String decode(String hexStr) throws UnsupportedEncodingException {
    if (null == hexStr || "".equals(hexStr) || (hexStr.length()) % 2 != 0) {
        return null;
    }

    int byteLength = hexStr.length() / 2;
    byte[] bytes = new byte[byteLength];

    int temp = 0;
    for (int i = 0; i < byteLength; i++) {
        temp = hex2Dec(hexStr.charAt(2 * i)) * 16 + hex2Dec(hexStr.charAt(2 * i + 1));
        bytes[i] = (byte) (temp < 128 ? temp : temp - 256);
    }
    return new String(bytes,"GBK");
}

private static int hex2Dec(char ch) {
    if (ch == '0')
        return 0;
    if (ch == '1')
        return 1;
    if (ch == '2')
        return 2;
    if (ch == '3')
        return 3;
    if (ch == '4')
        return 4;
    if (ch == '5')
        return 5;
    if (ch == '6')
        return 6;
    if (ch == '7')
        return 7;
    if (ch == '8')
        return 8;
    if (ch == '9')
        return 9;
    if (ch == 'a')
        return 10;
    if (ch == 'A')
        return 10;
    if (ch == 'B')
        return 11;
    if (ch == 'b')
        return 11;
    if (ch == 'C')
        return 12;
    if (ch == 'c')
        return 12;
    if (ch == 'D')
        return 13;
    if (ch == 'd')
        return 13;
    if (ch == 'E')
        return 14;
    if (ch == 'e')
        return 14;
    if (ch == 'F')
        return 15;
    if (ch == 'f')
        return 15;
    else
        return -1;

}
~~~
