#include <iostream>
#include <string>
#include <thread>
#include <mutex>

class Singleton
{
public:
    int val;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static Singleton& getInstance()
    {
        static Singleton instance;
        return instance;
    }

    void print(const std::string& str)
    {
        std::cout << "this = " << this << std::endl;
        std::cout << "this.val = " << &this->val << std::endl;
        
        std::cout << str << std::endl;
    }

private:
    Singleton() {
        std::cout << "create instance" << std::endl;
    }
    ~Singleton() {
        std::cout << "delete instance" << std::endl;
    }

};

void test(const std::string& str)
{
    Singleton::getInstance().print(str);
}

int main(void)
{
    std::thread t1(test, "hello t1");
    std::thread t2(test, "hello t2");

    t1.join();
    t2.join();
}