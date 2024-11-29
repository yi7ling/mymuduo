#include <iostream>

class Entity
{
    int x;
    int y;

public:
    Entity() {
        x = 0;
        y = 0;
    }

    void printxy() {
        std::cout << x << ", " << y << std::endl;
    }
};

int main() {
    Entity e;
    
    e.printxy();
}