cmake_minimum_required(VERSION 3.4.1)

# 引入子目录的CMakeLists.txt
# librtmp是子目录名
add_subdirectory(librtmp)

# 引入头文件路径, 可以使用<>来引用库文件
include_directories(librtmp libx264/include libfaac/include)

# 设置x264库路径
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libx264/libs/${CMAKE_ANDROID_ARCH_ABI}")

# 设置faac库路径
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libfaac/libs/${CMAKE_ANDROID_ARCH_ABI}")

file(GLOB src *.cpp)

add_library(
         native-lib
         SHARED
         ${src}
)

find_library(
          log-lib
          log
)

target_link_libraries(
           native-lib
           rtmp
           x264
           faac
           ${log-lib}
)