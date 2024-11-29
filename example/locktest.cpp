#include <functional>
#include <iostream>

int func1(int a, int b)
{
    return a+b;
}

int main(void)
{
    std::function<int(int, int)> f1 = func1;
    std::function<int(int)>      f2 = std::bind(func1, std::placeholders::_1, 1);
    std::function<int()>         f3 = std::bind(func1, 1, 1);

    std::cout << f1(1, 1) << std::endl; // 2
    std::cout << f2(1) << std::endl; // 2
    std::cout << f3() << std::endl; // 2
}