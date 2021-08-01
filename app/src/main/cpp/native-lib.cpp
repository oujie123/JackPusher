#include <jni.h>
#include <string>
#include <pthread.h>

#include <rtmp.h>
#include <x264.h>
#include "VideoChannel.h"
#include "AudioChannel.h"
#include "util.h"
#include "safe_queue.h"

using namespace std;

VideoChannel *videoChannel = nullptr;
AudioChannel *audioChannel = nullptr;
bool isStart = false;
bool readyPushing = false;
pthread_t pid_start;
SafeQueue<RTMPPacket *> packets;   // 不区分音视频，直接拿出来发给服务器
uint32_t start_time;

extern "C" JNIEXPORT jstring JNICALL
Java_com_gxa_jackpusher_MainActivity_stringFromJNI(
        JNIEnv *env,
        jobject /* this */) {
    char version[50];
    sprintf(version, "RTMP version: %d", RTMP_LibVersion());

    return env->NewStringUTF(version);
}

void callback(RTMPPacket * packet) {
    if (packet) {
        if (packet->m_nTimeStamp == -1) {
            packet->m_nTimeStamp = RTMP_GetTime() - start_time;   // 如果是i帧需要记录时间戳
        }
        packets.push(packet);
    }
}

void releasePackets(RTMPPacket **pPacket) {
    if (pPacket) {
        RTMPPacket_Free(*pPacket);
        //delete pPacket;
        pPacket = nullptr;
    }
}

/**
 * 初始化
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1init(JNIEnv *env, jobject instance) {

    videoChannel = new VideoChannel();
    videoChannel->setVideoCallback(callback);

    audioChannel = new AudioChannel();
    audioChannel->setAudioCallback(callback);

    packets.setReleaseCallback(releasePackets);
}

void *task_start(void *args) {
    char *url = static_cast<char *>(args);

    LOGE("xxx->%s", url);
    // 九部曲开始推流
    RTMP *rtmp = nullptr;
    int ret;

    do {
        rtmp = RTMP_Alloc();
        if (!rtmp) {
            LOGE("rtmp init failed.");
            break;
        }

        RTMP_Init(rtmp);
        rtmp->Link.timeout = 5;  // 设置五秒超时

        ret = RTMP_SetupURL(rtmp, url); //设置地址
        if (!ret) {
            LOGE("rtmp 设置流媒体地址失败");
            break;
        }

        RTMP_EnableWrite(rtmp);  // 开启输出模式

        ret = RTMP_Connect(rtmp, 0);
        if (!ret) {
            LOGE("trmp 建立连接失败: %d, url: %s.", ret, url);
            break;
        }

        ret = RTMP_ConnectStream(rtmp, 0); // 连接流
        if (!ret) {
            LOGE("rtmp 连接流失败. ret: %d", ret);
            break;
        }

        start_time = RTMP_GetTime();

        readyPushing = true;  // 准备好了可以推流

        // TODO 测试是不用发送序列头信息，是没有任何问题的，但是还是规矩
        //callback(audioChannel->getAudioSeqHeader());

        // 从队列拿到东西，直接发给服务器
        packets.setWork(true);

        RTMPPacket *pkt = nullptr;

        while (readyPushing) {
            packets.pop(pkt);

            if (!readyPushing) {
                break;
            }

            if (!pkt) {
                continue;  // 没取到就继续取
            }

            // TODO 成功取出流媒体包

            pkt->m_nInfoField2 = rtmp->m_stream_id;  // 赋一个流ID

            ret = RTMP_SendPacket(rtmp, pkt, 1);   // 1 代表开启rtmp内部队列

            // 释放函数
            releasePackets(&pkt);

            if (!ret) {
                LOGE("rtmp 发送失败，断开服务器.");
                break;
            }
        } // end while
        releasePackets(&pkt);
    } while (false); //为了流程控制，跳出释放资源

    // 释放
    isStart = false;
    readyPushing = false;
    packets.setWork(false);
    packets.clear();

    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }

    delete url;  // url是传进来，栈区直接引导的，直接delete清除内存

    return nullptr;
}

/**
 * 开始直播
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1start(JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);

    // 连接服务器，然后发包
    if (isStart) {
        return;
    }
    isStart = true;

    // 深拷贝保存地址
    char *url = new char(strlen(path) + 1);
    strcpy(url, path);

    LOGE("url:%s",url);
    pthread_create(&pid_start, nullptr, task_start, url);

    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1stop(JNIEnv *env, jobject instance) {
    isStart = false; // start的jni函数拦截的标记，为false
    readyPushing = false; // 九部曲推流的标记，为false
    packets.setWork(0); // 队列不准工作了
    pthread_join(pid_start, nullptr);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1release(JNIEnv *env, jobject instance) {
    DELETE(videoChannel);
    DELETE(audioChannel);
}

/**
 * 初始化x264编码器给pushVideo用
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1initVideoEncoder(JNIEnv *env, jobject instance,
                                                            jint width, jint height, jint mFps,
                                                            jint bitrate) {
    // TODO 初始化编码器
    if (videoChannel) {
        videoChannel->initVideoEncoder(width, height, mFps, bitrate);
    }

}

extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1pushVideo(JNIEnv *env, jobject instance,
                                                     jbyteArray data_) {
    if (!videoChannel || !readyPushing) {
        return;
    }

    jbyte *data = env->GetByteArrayElements(data_, NULL);   // 转化成jni的byte数组

    // TODO 编码放入队列
    videoChannel->encodeData(data);

    env->ReleaseByteArrayElements(data_, data, 0);
}


/**
 * 初始化
 */
extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1initAudioEncoder(JNIEnv *env, jobject instance,
                                                            jint sampleRate, jint numChannels) {
    if (audioChannel) {
        audioChannel->initAudioEncoder(sampleRate, numChannels);
    }
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1getInputSamples(JNIEnv *env, jobject instance) {

    if (audioChannel) {
        return audioChannel->getInputSamples();
    }
    return 0;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_gxa_jackpusher_JackPusher_native_1pushAudio(JNIEnv *env, jobject instance,
                                                     jbyteArray bytes_) {
    if (!audioChannel || !readyPushing) {
        return;
    }
    // 此data数据就是AudioRecord采集到的原始数据
    jbyte *bytes = env->GetByteArrayElements(bytes_, NULL);
    audioChannel->encodeData(bytes);
    env->ReleaseByteArrayElements(bytes_, bytes, 0);
}