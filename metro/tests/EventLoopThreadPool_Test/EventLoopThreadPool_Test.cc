#include "metro/net/inner/Poller.h"
#include "metro/net/Channel.h"
#include "metro/net/EventLoop.h"
#include "metro/net/inner/Poller/EpollPoller.h"
#include "metro/net/EventLoopThreadPool.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace metro
{

void Test()
{
    std::cout << "staring---------" << std::endl;
    EventLoopThreadPool eltp(1, "EventLoopThread");
    std::cout << "staring---------" << std::endl;
    eltp.start();
    std::cout << "staring" << std::endl;
    int f1_fd = open("/home/heng/metro/metro/tests/EventLoopThreadPool_Test/f1", O_RDWR);
    int f2_fd = open("/home/heng/metro/metro/tests/EventLoopThreadPool_Test/f2", O_RDWR);
    int f3_fd = open("/home/heng/metro/metro/tests/EventLoopThreadPool_Test/f3", O_RDWR);
    if(f1_fd < 0 || f2_fd < 0 || f3_fd < 0)
    {
        std::cout << "can't open file" << std::endl;
        exit(-1);
    }
    auto f1_ep = eltp.getNextLoop();
    auto f2_ep = eltp.getNextLoop();
    auto f3_ep = eltp.getNextLoop();

    Channel channel_f1(f1_ep, f1_fd);
    Channel channel_f2(f2_ep, f2_fd);
    Channel channel_f3(f3_ep, f3_fd);

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
                                    
        channel_f2.setReadCallBack([](){
                                    std::cout << "f2 ReadCallBack" << std::endl;
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
    
    f1_ep->queueInLoop([&](){
        channel_f1.enableWriting();
    });
    f2_ep->queueInLoop([&](){
        channel_f2.enableWriting();
    });
    f3_ep->queueInLoop([&](){
        channel_f3.enableReading();
    });

    eltp.getNextLoop()->runAfter(5, [channel_ptr = &channel_f1]{
        std::cout << "f1 disable all" << std::endl;
        channel_ptr->disableAll();
    });
    
    eltp.getNextLoop()->runAfter(10, [channel_ptr = &channel_f2]{
        std::cout << "f2 disable all" << std::endl;
        channel_ptr->disableAll();
    });

    eltp.getNextLoop()->runAfter(15, [channel_ptr = &channel_f3]{
        std::cout << "f3 disable all" << std::endl;
        channel_ptr->disableAll();
    });
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "wake up" << std::endl;
    }
    
}
}

using namespace metro;

int main(int argc, char const *argv[])
{
    Test();
    return 0;
}

