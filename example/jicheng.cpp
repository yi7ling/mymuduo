#include <iostream>

class base1 {
protected:
    int val;
public:
    virtual void fun1() {};
};

class base2 {
    char ch; // 会做内存对齐
public:
    virtual void fun2() {};
};

class child : base1, base2 {
public:
    void fun1() {};
    void fun2() {};
};


int main() {
    std::cout << "Size of child: " << sizeof(child) << " bytes" << std::endl;
    return 0;
}
