#include <iostream>

class A{
public:
    A(const A&) = delete;
    A(const A&&) = delete;
    A& operator=(const A&) = delete;
    A& operator=(const A&&) = delete;

    static A& getA()
    {
        static A a;
        return a;
    }

    void print_data() {
        printf("data = %d\n", data);
    }

private:
    int data;
    A() { data = 1; }
    ~A() {}
};

int main(void)
{
    A& ta = A::getA();
    
    ta.print_data();
}