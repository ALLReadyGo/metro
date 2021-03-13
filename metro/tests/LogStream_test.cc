#include "metro/utils/LogStream.h"
#include <unistd.h>

/* 

g++ metro/net/InetAddress.cc metro/tests/InetAddress_test.cc -I

*/

int main(int argc, char const *argv[])
{
    metro::LogStream ls;
    ls << "Log first" << "\n";
    ls << 12 << "\n";
    ls << 12.3 << "\n";
    write(STDOUT_FILENO, ls.bufferData(), ls.bufferLength());    
    
    ls.reset();
    ls << "Log second" << "\n";
    ls << 100 << "\n";
    ls << 125.3 << "\n";
    write(STDOUT_FILENO, ls.bufferData(), ls.bufferLength());    
    
    return 0;
}
