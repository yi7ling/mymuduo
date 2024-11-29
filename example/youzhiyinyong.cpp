#include <iostream>

class Myclass {
public:
    const int cval;
    static int sval;
    int* ptr;
    int& ref;

    Myclass(int cval, int& ref)
    : cval(cval),
      ptr(new int(3)),
      ref(ref)
    {}

    Myclass(const Myclass& other)
    : cval(other.cval),
      ptr(new int(*other.ptr)),
      ref(other.ref)
    {
    }

    Myclass(Myclass&& other)
    : cval(other.cval),
      ptr(other.ptr),
      ref(other.ref)
    {

    }

    Myclass& operator=(const Myclass& other)
    {
        *ptr = *other.ptr;
        ref = other.ref;
    }

    Myclass& operator=(Myclass&& other)
    {
        if (this != &other)
        {
            delete ptr;
            ptr = other.ptr;
            ref = ref;
        }
    }

    ~Myclass()
    {
        delete ptr;
    }
};

int Myclass::sval = 2;

class A {
public:
    const int cval;
    int* ptr;
    int& ref;
    int& ref1;

    A(int cval, int& ref, int& ref1)
    : cval(cval),
      ptr(new int(3)),
      ref(ref),
      ref1(ref1)
    {}
};


int main(void)
{
    int refval = 4;
    // Myclass my(1, refval);
    // std::cout << sizeof(my) << std::endl;

    int refval1 = 5;
    A a(1, refval, refval1);
    std::cout << &a << std::endl;
    std::cout << &a.cval << std::endl;
    std::cout << &a.ptr << std::endl;
    std::cout << &a.ref << std::endl;
    std::cout << &refval << std::endl;

    std::cout << &a.ref1 << std::endl;


    std::cout << sizeof(a) << std::endl;
}