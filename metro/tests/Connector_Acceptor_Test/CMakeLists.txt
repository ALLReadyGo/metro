cmake_minimum_required(VERSION 3.5)
project(metro)
set(CMAKE_BUILD_TYPE Debug)
add_executable(metro_service 
metro/net/Channel.cc 
metro/net/inner/Poller.cc 
metro/net/inner/Poller/EpollPoller.cc 
metro/utils/Logger.cc 
metro/utils/Date.cc 
metro/utils/LogStream.cc  
metro/utils/MsgBuffer.cc
metro/utils/TimingWheel.cc
metro/net/inner/Timer.cc 
metro/net/inner/TimerQueue.cc 
metro/net/inner/Socket.cc
metro/net/inner/TcpConnectionImpl.cc
metro/net/inner/Acceptor.cc
metro/net/inner/Connector.cc
metro/net/InetAddress.cc
metro/net/TcpConnection.cc
metro/net/EventLoop.cc 
metro/net/EventLoopThread.cc
metro/net/EventLoopThreadPool.cc 
metro/tests/Connector_Acceptor_Test/service.cc
)


add_executable(metro_client 
metro/net/Channel.cc 
metro/net/inner/Poller.cc 
metro/net/inner/Poller/EpollPoller.cc 
metro/utils/Logger.cc 
metro/utils/Date.cc 
metro/utils/LogStream.cc  
metro/utils/MsgBuffer.cc
metro/utils/TimingWheel.cc
metro/net/inner/Timer.cc 
metro/net/inner/TimerQueue.cc 
metro/net/inner/Socket.cc
metro/net/inner/TcpConnectionImpl.cc
metro/net/inner/Acceptor.cc
metro/net/inner/Connector.cc
metro/net/InetAddress.cc
metro/net/TcpConnection.cc
metro/net/EventLoop.cc 
metro/net/EventLoopThread.cc
metro/net/EventLoopThreadPool.cc 
metro/tests/Connector_Acceptor_Test/client.cc
)

find_package(Threads)
target_link_libraries (metro_service ${CMAKE_THREAD_LIBS_INIT})

find_package(Threads)
target_link_libraries (metro_client ${CMAKE_THREAD_LIBS_INIT})
target_include_directories(metro_service PUBLIC ./)
target_include_directories(metro_client PUBLIC ./)

