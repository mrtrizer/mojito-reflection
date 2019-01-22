#include <iostream>

#include <Reflection.hpp>
#include <Type.hpp>
#include <Function.hpp>
#include <ReflectionMarkers.hpp>

class REFLECT Test {
public:
    Test() = default;
    void a(int) {}
    void b(int) {}
    void c(int) {}
};

extern bool generateReflection(flappy::Reflection&);

using namespace flappy;

int main() {
    auto reflection = std::make_shared<Reflection>();
    generateReflection(*reflection);
    std::cout << "Method list:" << std::endl;
    for (auto method : reflection->getType("Test").functionMap())
        std::cout << method.first << std::endl;
    return 0;
}
