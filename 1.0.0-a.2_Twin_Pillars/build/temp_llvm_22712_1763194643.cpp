#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif
    std::cout << u8"你好，世界！" << std::endl;
    std::cout << u8"114514" << std::endl;
    std::cout << u8"HelloWorld" << std::endl;
    return 0;
}
