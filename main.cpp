#include <iostream>
#include <string>
#include <cstdint>
#include <windows.h>
#include <iomanip>
int main()
{
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    std::string in;
    std::cin>>in;
    uint64_t out1=0, out2=0, out3=0, out4=0;
    int x=in.size();
    for (int i=0; i<x; i++) {
        out1+=in[i]*rand();
        out2+=in[i]*rand();
        out3+=in[i]*rand();
        out4+=in[i]*rand();
    }
    std::cout<<std::hex<<std::setw(16)<<std::setfill('0')<<out1<<std::setw(16)<<std::setfill('0')<<out2<<std::setw(16)<<std::setfill('0')<<out3<<std::setw(16)<<std::setfill('0')<<out4;
    return 0;
}
