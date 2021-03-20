#include "metro/net/EventLoop.h"
#include "metro/net/Channel.h"
#include <sys/unistd.h>
#include <sys/fcntl.h>

using namespace metro;
/*
g++ -g metro/net/Channel.cc metro/net/inner/Poller.cc metro/net/inner/Poller/EpollPoller.cc metro/utils/Logger.cc metro/utils/Date.cc metro/utils/LogStream.cc  metro/net/inner/Timer.cc metro/net/inner/TimerQueue.cc metro/net/EventLoop.cc metro/tests/EventLoop_Test/EventLoop_Test.cc  -I./ 
*/
void Test()
{
    EventLoop loop;
    int f1_fd = open("../metro/tests/EventLoop_Test/f1", O_RDWR);
    int f2_fd = open("../metro/tests/EventLoop_Test/f2", O_RDWR);
    int f3_fd = open("../metro/tests/EventLoop_Test/f3", O_RDWR);

    Channel channel_f1(&loop, f1_fd);
    Channel channel_f2(&loop, f2_fd);
    Channel channel_f3(&loop, f3_fd);

    {
        /* f1 */
        channel_f1.setEventCallBack([](){
                                    std::cout << "f1 EventCallBack" << std::endl;
                                        });
                                    
        channel_f1.setReadCallBack([](){
                                    std::cout << "f1 ReadCallBack" << std::endl;
                                        });

        channel_f1.setWriteCallBack([](){
                                    std::cout << "f1 WriteCallBack" << std::endl;
                                        });

        channel_f1.setErrorCallBack([](){
                                    std::cout << "f1 ErrorCallBack" << std::endl;
                                        });

        /* f2*/
        channel_f2.setEventCallBack([](){
                                    std::cout << "f2 EventCallBack" << std::endl;
                                        });
                 
        channel_f2.setReadCallBack([channel_ptr = &channel_f2](){                               
                                        });

        channel_f2.setWriteCallBack([](){
                                    std::cout << "f2 WriteCallBack" << std::endl;
                                        });

        channel_f2.setErrorCallBack([](){
                                    std::cout << "f2 ErrorCallBack" << std::endl;
                                        });

        /* f3 */
        channel_f3.setEventCallBack([](){
                                    std::cout << "f3 EventCallBack" << std::endl;
                                        });
                                    
        channel_f3.setReadCallBack([](){
                                    std::cout << "f3 ReadCallBack" << std::endl;
                                        });

        channel_f3.setWriteCallBack([](){
                                    std::cout << "f3 WriteCallBack" << std::endl;
                                        });

        channel_f3.setErrorCallBack([](){
                                    std::cout << "f3 ErrorCallBack" << std::endl;
                                        });


    }
    
    channel_f1.enableWriting();     // 持续触发
    channel_f2.enableWriting();     
    channel_f3.enableReading();

    loop.runAfter(1, [cf1_ptr = &channel_f1](){
        cf1_ptr->disableAll();
    });

    loop.runAfter(2, [cf2_ptr = &channel_f2](){
        cf2_ptr->disableAll();
    });
    loop.runAfter(3, [cf3_ptr = &channel_f3](){
        cf3_ptr->disableAll();
    });

    // loop.runAfter(1, [](){
    //     std::cout << "run after 1 second called" << std::endl;
    // });
    // loop.runAfter(2, [](){
    //     std::cout << "run after 2 second called" << std::endl;
    // });
    // loop.runAfter(3, [](){
    //     std::cout << "run after 3 second called" << std::endl;
    // });

    // loop.runEvery(1,[](){
    //     std::cout << "every 1 second called" << std::endl;
    // });

    loop.loop();

}


int main(int argc, char const *argv[])
{   
    Test();
    return 0;
}

