---
layout: post
title: Android录音相关方法介绍
category: accumulation
tags:
    - AudioRecord
    - AAC
keywords: AudioRecord, AAC
banner: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Entrance%20to%20the%20Public%20Park%20in%20Arles.jpg
thumbnail: https://raw.githubusercontent.com/agehua/blog-imags/img/lib-jekyll/Entrance%20to%20the%20Public%20Park%20in%20Arles.jpg
toc: true
---

`本文介绍Android开发录音功能中用到的相关API和音频数据编码解码的处理`

### 录音基础知识

**常用音频格式：**
- WAV 格式：音质高 无损格式 体积较大
- AAC（Advanced Audio Coding） 格式：相对于 mp3，AAC 格式的音质更佳，文件更小，有损压缩，一般苹果或者Android SDK4.1.2（API 16）及以上版本支持播放,性价比高
- AMR 格式：压缩比比较大，但相对其他的压缩格式质量比较差，多用于人声，通话录音
    - AMR分类:  AMR(AMR-NB): 语音带宽范围：300－3400Hz，8KHz抽样
- mp3 格式：特点 使用广泛， 有损压缩，牺牲了12KHz到16KHz高音频的音质
<!--more-->

**音频开发的具体内容**
- 音频采集/播放
- 音频算法处理（去噪、静音检测、回声消除、音效处理、功放/增强、混音/分离，等等）
- 音频的编解码和格式转换
- 音频传输协议的开发（SIP，A2DP、AVRCP，等等）

### 录音API: [AudioRecord](https://developer.android.com/reference/android/media/AudioRecord)

AudioRecord是相对MediaRecord更为底层的API，使用AudioRecord也可以很方便的完成录音功能。AudioRecord录音录制的是原始的PCM音频数据，可以使用[AudioTrack](https://developer.android.com/reference/android/media/AudioTrack)来播放PCM音频文件。

先看下AudioRecord的构造函数，
~~~ Java
/**
 * @param audioSource ：录音源
 * 这里选择使用麦克风：MediaRecorder.AudioSource.MIC
 * @param sampleRateInHz： 采样率
 * @param channelConfig：声道数
 * @param audioFormat： 采样位数.
 *   See {@link AudioFormat#ENCODING_PCM_8BIT}, {@link AudioFormat#ENCODING_PCM_16BIT},
 *   and {@link AudioFormat#ENCODING_PCM_FLOAT}.
 * @param bufferSizeInBytes： 音频录制的缓冲区大小
 *   See {@link #getMinBufferSize(int, int, int)}
*/
AudioRecord(int audioSource, int sampleRateInHz, int channelConfig, int audioFormat, int bufferSizeInBytes)
~~~

可以看到构造函数需要 5 个参数，最后一个参数bufferSizeInBytes，获取方式为：
~~~ Java
private int channelCount=2;     //音频采样通道，默认2通道
bufferSize = AudioRecord.getMinBufferSize(sampleRate, channelConfig, audioFormat)*2; // 5.录音过程中音频缓冲空间大小 (码率)
~~~

初始化完成后，就可以开始录制：
~~~ Java
//实例化AudioRecord
mRecorder=new AudioRecord(MediaRecorder.AudioSource.MIC,sampleRate,channelConfig,
    audioFormat,bufferSize);

private FileOutputStream fos; //用于保存录音文件
//开始录制
mRecorder.startRecording();
byte[] byteBuffer = new byte[bufferSize];

while (state == RecordState.RECORDING) { // //循环读取数据到buffer中，并保存buffer中的数据到文件中
    int end = audioRecord.read(byteBuffer, 0, byteBuffer.length);
    notifyData(byteBuffer);
    fos.write(byteBuffer, 0, end);
    fos.flush();
}
mRecorder.stop();//暂停录制
~~~

更新 RecordState 状态就可以退出 while循环，结束录制
~~~ Java
 /**
 * 表示当前状态
 */
public enum RecordState {
    /**
     * 空闲状态
     */
    IDLE,
    /**
     * 录音中
     */
    RECORDING,
    /**
     * 暂停中
     */
    PAUSE,
    /**
     * 正在停止
     */
    STOP,
    /**
     * 录音流程结束（转换结束）
     */
    FINISH
}
~~~
### 计算合适码率
不同的设备可以支持的采样率不同，根据采样率从大到小计算 bufferSize
~~~ Java
public void getValidBufferSize() {
    for (int rate : new int[]{44100, 22050, 11025, 16000, 8000}) {  // add the rates you wish to check against
        int bufferSize = AudioRecord.getMinBufferSize(rate, AudioFormat.CHANNEL_CONFIGURATION_DEFAULT, AudioFormat.ENCODING_PCM_16BIT);
        if (bufferSize > 0) {
            audioRecord = new AudioRecord(MediaRecorder.AudioSource.VOICE_COMMUNICATION, currentConfig.getSampleRate(),
                    currentConfig.getChannelConfig(), currentConfig.getEncodingConfig(), bufferSize);
            return;
        }
    }
}
~~~
### 根据码率计算已录制时间
Stack Overflow上有回答：[Calculate elapsed time from AudioRecord](https://stackoverflow.com/questions/15227087/calculate-elapsed-time-from-audiorecord)
上面提到 AudioRecord的最后一个参数就是码率：

**码率 = 采样频率 * 采样位数 * 声道个数**； 例：采样频率44.1KHz，量化位数16bit，立体声(双声道)，
未压缩时的码率 = 44.1KHz * 16 * 2 = 1411.2Kbps = 176.4KBps，即每秒要录制的资源大小, 理论上码率和质量成正比
对应时间的计算方式应该是：
~~~ Java
已录制时长(单位是秒) = dataSize/(getEncoding()*getChannelCount()*44100)
~~~

~~~ Java
/**
 * 获取当前录音的采样位宽 单位bit
 *
 * @return 采样位宽 0: error
 */
public int getEncoding() {
    if (encodingConfig == AudioFormat.ENCODING_PCM_8BIT) {
        return 8;
    } else if (encodingConfig == AudioFormat.ENCODING_PCM_16BIT) {
        return 16;
    } else {
        return 0;
    }
}

/**
 * 当前的声道数
 *
 * @return 声道数： 0：error
 */
public int getChannelCount() {
    if (channelConfig == AudioFormat.CHANNEL_IN_MONO) {
        return 1;
    } else if (channelConfig == AudioFormat.CHANNEL_IN_STEREO) {
        return 2;
    } else {
        return 0;
    }
}
~~~

按照上面的步骤，我们就能成功的录制PCM音频文件了，但是处于传输和存储方面的考虑，一般来说，我们是不会直接录制PCM音频文件的。而是在录制过程中就对音频数据进行编码为aac、mp3、wav等其他格式的音频文件

关于录制pcm，mp3格式，详细可以看这篇文章：[Android音频开发（1）：音频基础知识](https://www.jianshu.com/p/c0222de2faed)
### AAC格式介绍
AAC（高级音频编码技术 Advanced Audio Coding)，出现于1997年，是基于MPEG-2的音频编码技术。由Fraunhofer IIS、杜比、苹果、AT&T、索尼等公司共同开发，以取代mp3格式。2000年，MPEG-4标准出台，AAC从新整合了其特性，故现又称MPEG-4 AAC，即m4a。

作为一种高压缩比的音频压缩算法，AAC通常压缩比为18：1，也有资料说为20：1，远胜mp3，而音质由于采用多声道，和使用低复杂性的描述方式，使其比几乎所有的传统编码方式在同规格的情况下更胜一筹。不过直到2006年，使用这一格式储存音频的并不多，可以播放该格式的mp3播放器更是少之又少，目前所知仅有苹果iPod，而手机支持AAC的相对要多一些，此外电脑上很多音频播放软件都支持AAC格式，如苹果iTunes。
AAC所采用的运算法则

AAC所采用的运算法则与MP3的运算法则有所不同，AAC通过结合其他的功能来提高编码效率。AAC的音频算法在压缩能力上远远超过了以前的一些压缩算法（比如MP3等）。它还同时支持多达48个音轨、15个低频音轨、更多种采样率和比特率、多种语言的兼容能力、更高的解码效率。号称「最大能容纳48通道的音轨，采样率达96 KHz，并且在320Kbps的数据速率下能为5.1声道音乐节目提供相当于ITU-R广播的品质」。

总之，AAC可以在比MP3文件节省大约30%的储存空间与带宽的前提下提供更好的音质。但是在空间上和结构上AAC和mp3编码出来后的风格不太一样，喜欢与否属于仁者见仁智者见智的事情。

### 硬件编解码API: MediaCodec
[MediaCodec](https://developer.android.com/reference/android/media/MediaCodec) 是 Android 提供的可以访问底层解码和编码Media的组件，它是 Android 底层多媒体支持框架的一部分

下图是Android官方提供的MediaCodec工作流程：

![Android MediaCodec](/images/blogimages/2018/android_audio_mediacodec.png)

针对于上图，我们可以把InputBuffers和OutputBuffers简单的理解为它们共同组成了一个环形的传送带，传送带上铺满了空盒子。
编解码开始后，我们需要得到一个空盒子（dequeueInputBuffer），然后往空盒子中填充原料（需要被编/解码的音/视频数据），并且放回到传送带你取出时候的那个位置上面（queueInputBuffer）。
传送带经过处理器（Codec）后，盒子里面的原料被加工成了你所期望的东西（编解码后的数据），你就可以按照你放入原料时候的顺序，连带着盒子一起取出加工好的东西（dequeueOutputBuffer），并将取出来的东西贴标签（加数据头之类的非必须）和装箱（组合编码后的帧数据）操作，同样之后也要把盒子放回到原来的位置（releaseOutputBuffer）

#### 初始化编码器实例
~~~ Java
    public void initAudioEncoder(RecordConfig recordConfig){
        aBufferInfo = new MediaCodec.BufferInfo();
        audioQueue = new LinkedBlockingQueue<>();
        audioCodecInfo = selectCodec(AUDIO_MIME_TYPE);
        if (audioCodecInfo == null) {
            return;
        }
        // 取到前面录音时的相关配置
        int sampleRate = recordConfig.getSampleRate();
        int pcmFormat = recordConfig.getEncoding();
        int chanelCount = recordConfig.getChannelCount();

        audioFormat = MediaFormat.createAudioFormat(AUDIO_MIME_TYPE, sampleRate, chanelCount);
        audioFormat.setInteger(MediaFormat.KEY_AAC_PROFILE, MediaCodecInfo.CodecProfileLevel.AACObjectLC);
        audioFormat.setInteger(MediaFormat.KEY_CHANNEL_MASK, AudioFormat.CHANNEL_IN_STEREO);//CHANNEL_IN_STEREO 立体声
        int bitRate = sampleRate * pcmFormat * chanelCount;
        audioFormat.setInteger(MediaFormat.KEY_BIT_RATE, bitRate);
        audioFormat.setInteger(MediaFormat.KEY_CHANNEL_COUNT, chanelCount);
        audioFormat.setInteger(MediaFormat.KEY_SAMPLE_RATE, sampleRate);

        if (aEncoder != null) {
            return;
        }
        try { // 创建音频编码器
            aEncoder = MediaCodec.createEncoderByType(AUDIO_MIME_TYPE);
        } catch (IOException e) {
            e.printStackTrace();
            throw new RuntimeException("初始化音频编码器失败", e);
        }
    }


    private MediaCodecInfo selectCodec(String mimeType) {
        int numCodecs = MediaCodecList.getCodecCount();
        for (int i = 0; i < numCodecs; i++) {
            MediaCodecInfo codecInfo = MediaCodecList.getCodecInfoAt(i);
            if (!codecInfo.isEncoder()) {
                continue;
            }
            String[] types = codecInfo.getSupportedTypes();
            for (int j = 0; j < types.length; j++) {
                if (types[j].equalsIgnoreCase(mimeType)) {
                    return codecInfo;
                }
            }
        }
        return null;
    }
~~~

#### 解码器开始工作

~~~ Java
    //之前的音频录制是直接循环读取，然后写入文件，这里需要做编码处理再写入文件
    //这里的处理就是和之前传送带取盒子放原料的流程一样
    private void startAudioEncode() {
        audioEncoderThread = new Thread() { // 注意一般在子线程中循环处理
            @Override
            public void run() {
                Log.d(TAG, " 编码线程 启动...");
                presentationTimeUs = System.currentTimeMillis() * 1000;
                aEncoderEnd = false;
                aEncoder.configure(audioFormat, null, null,
                        MediaCodec.CONFIGURE_FLAG_ENCODE);
                aEncoder.start();
                while (audioEncoderLoop && !Thread.interrupted()) {
                    try {
                        byte[] data = audioQueue.take(); // 从队列中取得数据，这个队列在下面有解释
                        encodeAudioData(data);
                    } catch (InterruptedException e) {
                        e.printStackTrace();
                        break;
                    }
                }
                if (aEncoder != null) {
                    //停止音频编码器
                    aEncoder.stop();
                    //释放音频编码器
                    aEncoder.release();
                    aEncoder = null;
                }

                audioQueue.clear();
                Log.d(TAG, "= =lgd= ==Audio 编码线程 退出...");
            }
        };
        audioEncoderLoop = true;
        audioEncoderThread.start();
    }

    /**
    * 从队列中取得数据，并编码为 AAC 格式
    * @param input
    */
    private void encodeAudioData(byte[] input){
        try {
            //拿到输入缓冲区,用于传送数据进行编码
            ByteBuffer[] inputBuffers = aEncoder.getInputBuffers();
            //首先通过dequeueInputBuffer(long timeoutUs)请求一个输入缓存，timeoutUs代表等待时间，设置为-1代表无限等待
            int inputBufferIndex = aEncoder.dequeueInputBuffer(-1);

            if (inputBufferIndex >= 0) {
                ByteBuffer inputBuffer = inputBuffers[inputBufferIndex];
                //使用之前要clear一下，避免之前的缓存数据影响当前数据
                inputBuffer.clear();
                //把数据添加到输入缓存中，
                inputBuffer.put(input);
                //并调用queueInputBuffer()把缓存数据入队
                aEncoder.queueInputBuffer(inputBufferIndex, 0, input.length, 0, 0);
            }

            //拿到输出缓冲区,用于取到编码后的数据
            ByteBuffer[] outputBuffers = aEncoder.getOutputBuffers();
            //拿到输出缓冲区的索引
            int outputBufferIndex = aEncoder.dequeueOutputBuffer(aBufferInfo, TIMEOUT_USEC);

            ByteBuffer outputBuffer;
            int outBitSize;
            int outPacketSize;
            byte[] chunkAudio;

            while (outputBufferIndex >= 0) {
                outBitSize = aBufferInfo.size;

                //添加ADTS头,ADTS头包含了AAC文件的采样率、通道数、帧数据长度等信息。
                outPacketSize = outBitSize + 7;//7为ADTS头部的大小
                outputBuffer = outputBuffers[outputBufferIndex];//拿到输出Buffer
                outputBuffer.position(aBufferInfo.offset);
                outputBuffer.limit(aBufferInfo.offset + outBitSize);
                chunkAudio = new byte[outPacketSize];
                addADTStoPacket(chunkAudio, outPacketSize); // 给编码出的aac裸流添加adts头字段
                outputBuffer.get(chunkAudio, 7, outBitSize);//将编码得到的AAC数据 取出到byte[]中偏移量offset=7
                outputBuffer.position(aBufferInfo.offset);

                if (null != mCallback) {
                    mCallback.outputAudioData(chunkAudio, chunkAudio.length, (int) aBufferInfo.presentationTimeUs / 1000);
                }
                //releaseOutputBuffer方法必须调用
                aEncoder.releaseOutputBuffer(outputBufferIndex, false);
                outputBufferIndex = aEncoder.dequeueOutputBuffer(aBufferInfo, 10000);
            }

        } catch (Exception t) {
            Log.e(TAG, " =encodeAudioData=====error: " + t.toString());
        }
    }
~~~

上面的audioQueue 是一个链表 `LinkedBlockingQueue<byte[]>`。
它的作用是由 audioRecord.read(byteBuffer, 0, byteBuffer.length) 获取录音数据，放入队列
~~~ Java
/**
 * 放入音频数据
 * @param data 
 */
public void putAudioData(byte[] data) {
    try {
        audioQueue.put(data);
    } catch (InterruptedException e) {
        e.printStackTrace();
    }
}
~~~

给编码出的aac裸流添加adts头字段
~~~ Java
/**
 * 添加ADTS头
 *
 * @param packet 要空出前7个字节，否则会搞乱数据
 * @param packetLen
 */
private void addADTStoPacket(byte[] packet, int packetLen) {
    int profile = 2; // AAC LC
    int freqIdx = 8; // 44.1KHz
    int chanCfg = 1; // CPE

    // fill in ADTS data
    packet[0] = (byte) 0xFF;
    packet[1] = (byte) 0xF9;
    packet[2] = (byte) (((profile - 1) << 6) + (freqIdx << 2) + (chanCfg >> 2));
    packet[3] = (byte) (((chanCfg & 3) << 6) + (packetLen >> 11));
    packet[4] = (byte) ((packetLen & 0x7FF) >> 3);
    packet[5] = (byte) (((packetLen & 7) << 5) + 0x1F);
    packet[6] = (byte) 0xFC;
}
~~~

### 音视频混合API: 音视频混合API
[MediaMuxer](https://developer.android.com/reference/android/media/MediaMuxer)的使用很简单，在Android Developer官网上MediaMuxer的API说明中，也有其简单的使用示例代码，这里就不列了。

参照官方的说明和代码示例，我们可以知道，音视频混合（也可以音频和音频混合），只需要将编码器的MediaFormat加入到MediaMuxer中，得到一个音轨视频轨的索引，然后每次从编码器中取出来的ByteBuffer，写入（writeSampleData）到编码器所在的轨道中就ok了。 
这里需要注意的是，**一定要等编码器设置编码格式完成后，再将它加入到混合器中**，编码器编码格式设置完成的标志是dequeueOutputBuffer得到返回值为MediaCodec.INFO_OUTPUT_FORMAT_CHANGED。

### 源码
文章内用到的源码，在[agehua/ZlwAudioRecorder](https://github.com/agehua/ZlwAudioRecorder)，这个是forked from zhaolewei/ZlwAudioRecorder。在原来项目上加入了 AAC 格式支持

### 参考文章
[Android硬编码——音频编码、视频编码及音视频混合](https://blog.csdn.net/junzia/article/details/54018671)

[Android音频开发（1）：音频基础知识](https://www.jianshu.com/p/c0222de2faed)