//
// Created by Administrator on 2021/7/31.
//

#ifndef JACKPUSHER_UTIL_H
#define JACKPUSHER_UTIL_H

#include <android/log.h>

/*if (dataSource){
    delete dataSource;
    dataSource = 0;
}*/
//定义释放的宏函数
#define DELETE(object) if(object){delete object; object = 0;}

//定义日志打印宏函数
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "JackOu",__VA_ARGS__)

#endif //JACKPUSHER_UTIL_H
