cmake_minimum_required(VERSION 3.5)
project(metro)

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
                metro/tests/EventLoop_Test/EventLoop_Test.cc)

target_include_directories(metro PUBLIC ./)