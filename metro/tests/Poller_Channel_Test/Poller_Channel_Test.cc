#include "metro/net/inner/Poller.h"
#include "metro/net/Channel.h"
#include "metro/net/EventLoop.h"
#include "metro/net/inner/Poller/EpollPoller.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>



/* 
g++ -g metro/net/Channel.cc metro/net/inner/Poller.cc metro/net/inner/Poller/EpollPoller.cc metro/utils/Logger.cc metro/utils/Date.cc metro/utils/LogStream.cc metro/tests/Poller_Channel_Test/Poller_Channel_Test.cc -I./ 
*/
namespace metro
{

void Test()
{
    EventLoop *loop = new EventLoop();
    EpollPoller* poller =  new EpollPoller(loop);

    int f1_fd = open("./metro/tests/Poller_Channel_Test/f1", O_RDWR);
    int f2_fd = open("./metro/tests/Poller_Channel_Test/f2", O_RDWR);
    int f3_fd = open("./metro/tests/Poller_Channel_Test/f3", O_RDWR);

    Channel channel_f1(loop, f1_fd);
    Channel channel_f2(loop, f2_fd);
    Channel channel_f3(loop, f3_fd);

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
    
    channel_f1.enableWriting();
    channel_f2.enableWriting();
    channel_f3.enableReading();

    poller->updateChannel(&channel_f1);
    poller->updateChannel(&channel_f2);
    poller->updateChannel(&channel_f3);
    


    ChannelList list;
    poller->poll(10000, list);
    for(int i = 0; i < list.size(); ++i)
    {
        list[i]->handleEvent();
    }

    /* 测试删除 */
    channel_f1.events_ = 0;
    channel_f2.events_ = 0;
    channel_f3.events_ = 0;

    poller->updateChannel(&channel_f1);
    std::cout << channel_f1.index_ << std::endl;
    poller->updateChannel(&channel_f2);
    std::cout << channel_f2.index_ << std::endl;
    poller->updateChannel(&channel_f3);
    std::cout << channel_f3.index_ << std::endl;

    poller->removeChannel(&channel_f1);
    std::cout << channel_f1.index_ << std::endl;
    poller->removeChannel(&channel_f2);
    std::cout << channel_f2.index_ << std::endl;
    poller->removeChannel(&channel_f3);
    std::cout << channel_f3.index_ << std::endl;

}
}

using namespace metro;

int main(int argc, char const *argv[])
{
    Test();
    return 0;
}

