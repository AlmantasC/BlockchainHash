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
    std::getline(std::cin, in);

    uint64_t a[4] = { 1, 2, 3, 4 };
    uint64_t b[4] = {
        0x9e3779b97f4a7c15ULL, 0xbf58476d1ce4e5b9ULL,
        0x94d049bb133111ebULL, 0x100000001b3ULL
    };
    for (unsigned char c : in)
        for (int i=0; i<4; ++i)
            a[i]=a[i]*b[i]+c;

    std::ostringstream os;
    for (uint64_t v : a) os<<std::hex<<std::setw(16)<<std::setfill('0')<<v;
    std::cout<<os.str();
}
