package com.gxa.jackpusher;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * @Author: Jack Ou
 * @CreateDate: 2021/7/31 15:01
 * @UpdateUser: 更新者
 * @UpdateDate: 2021/7/31 15:01
 * @UpdateRemark: 更新说明
 */
public class AudioChannel {
    private JackPusher mPusher; // 把AudioRecord音频数据 回调给中转站，其他数据 回调给中转站，只有中转站才有资格和C++层打交道
    private boolean isLive; // 是否直播：非常重要的标记，开始直播就是true，停止直播就是false，通过此标记控制是否发送数据给C++层
    private int channels = 2; // 通道数为2，说明是2个通道(人类的耳朵，两个耳朵， 左声道/右声道)
    private AudioRecord audioRecord; // AudioRecord采集Android麦克风音频数据 --> C++层 --> 编码 --> 封包 ---> 加入队列
    private ExecutorService executorService; // 线程池 就是为了执行子线程而已
    int inputSamples; // 4096

    public AudioChannel(JackPusher pusher) {
        this.mPusher = pusher;
        executorService = Executors.newSingleThreadExecutor(); // 单例线程池而已，就相当于 new Thread
        int channelConfig;
        if (channels == 2) {
            channelConfig = AudioFormat.CHANNEL_IN_STEREO; // 双声道
        } else {
            channelConfig = AudioFormat.CHANNEL_IN_MONO; // 单声道
        }
        // 初始化faac音频编码器
        mPusher.native_initAudioEncoder(44100, channels);

        // (getInputSamples单通道样本数1024 * 通道数2)=2048 * 2(一个样本16bit，2字节) = 4096
        inputSamples = mPusher.getInputSamples() * 2;
        // TODO Log.d("JackOu", "AudioChannel.java inputSamples:" + inputSamples); // AudioChannel.java inputSamples:4096

        // AudioRecord.getMinBufferSize 得到的minBufferSize 能大不能小，最好是 * 2
        int minBufferSize = AudioRecord.getMinBufferSize(44100, channelConfig, AudioFormat.ENCODING_PCM_16BIT) * 2;
        Log.d("JackOu", "AudioChannel.java minBufferSize:" + minBufferSize); // AudioChannel.java minBufferSize:14208

        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, // 安卓手机的麦克风
                44100,  // 采样率
                channelConfig, // 声道数 双声道
                AudioFormat.ENCODING_PCM_16BIT, // 位深 16位 2字节
                Math.max(inputSamples, minBufferSize)); // 缓冲区大小（以字节为单位）：max在两者中取最大的，内置缓冲buffsize大一些 没关系的，能大 但是不能小
    }

    // 开始直播，修改标记 让其可以进入while 完成音频数据推送， 并开启子线程
    public void startLive() {
        isLive = true;
        executorService.submit(new AudioTask()); // 子线程启动 Runnable（AudioTask）
    }

    // 停止直播，只修改标记 让其可以不要进入while 就不会再数据推送了
    public void stopLive() {
        isLive = false;
    }

    // AudioRecord的释放工作
    public void release() {
        if (audioRecord != null) {
            audioRecord.release();
            audioRecord = null;
        }
    }

    // 子线程：AudioRecord采集录制音频数据，再把此数据传递给 --> C++层(进行编码) --> 封包(RTMPPacket) --> 发送
    private class AudioTask implements Runnable {
        @Override
        public void run() {
            audioRecord.startRecording(); // 开始录音（调用Android的API录制手机麦克风的声音）

            // 单通道样本数：1024
            // 位深： 16bit位 2字节
            // 声道数：双声道
            // 以上规格：之前说过多遍了，经验值是4096
            // 1024单通道样本数 * 2 * 2 = 4096
            byte[] bytes = new byte[inputSamples]; // 接收录制声音数据的 byte[]
            // 读取数据
            while (isLive) {
                // 每次读多少数据要根据编码器来定！
                int len = audioRecord.read(bytes, 0, bytes.length);
                if (len > 0) {
                    // 成功采集到音频数据了
                    // 对音频数据进行编码并发送（将编码后的数据push到安全队列中）
                    mPusher.native_pushAudio(bytes);
                }
            }
            audioRecord.stop(); // 停止录音
        }
    }
}

