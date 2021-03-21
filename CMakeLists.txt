cmake_minimum_required(VERSION 3.5)
project(metro)
set(CMAKE_BUILD_TYPE Debug)
add_executable(metro 
metro/net/Channel.cc 
metro/net/inner/Poller.cc 
metro/net/inner/Poller/EpollPoller.cc 
metro/utils/Logger.cc 
metro/utils/Date.cc 
metro/utils/LogStream.cc  
metro/net/inner/Timer.cc 
metro/net/inner/TimerQueue.cc 
metro/net/EventLoop.cc 
metro/net/EventLoopThread.cc
metro/net/EventLoopThreadPool.cc 
metro/tests/EventLoopThreadPool_Test/EventLoopThreadPool_Test.cc
)
find_package(Threads)
target_link_libraries (${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

target_include_directories(metro PUBLIC ./)