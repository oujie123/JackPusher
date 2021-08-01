//
// Created by Administrator on 2021/7/31.
//

#ifndef JACKPUSHER_VIDEOCHANNEL_H
#define JACKPUSHER_VIDEOCHANNEL_H

#include <pthread.h>
#include <x264.h>
#include "util.h"
#include <cstring>
#include <rtmp.h>

class VideoChannel {

public:
    typedef void (*VideoCallback)(RTMPPacket *);
    VideoChannel();
    ~VideoChannel();

    void initVideoEncoder(int width, int height, int fps, int bitrate);

    void encodeData(signed char *data);

    void sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len);

    void setVideoCallback(VideoCallback videoCallback);

    void sendFrame(int type, int payload, uint8_t *payload1);

private:
    pthread_mutex_t mutex;
    int mWidth;
    int mHeight;
    int mFps;
    int mBitrate;

    int y_len;
    int uv_len;
    x264_t *videoEncoder = nullptr;
    x264_picture_t *pic_in = 0; // 先理解是每一张图片 pic
    VideoCallback videoCallback;
};
#endif //JACKPUSHER_VIDEOCHANNEL_H
