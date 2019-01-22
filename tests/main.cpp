#include <iostream>

#include <Reflection.hpp>

class __attribute((annotate("reflect"))) Test {
public:
    Test() = default;
    void a(int) {}
};

//extern bool generateReflection(flappy::Reflection&);

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
