#include <iostream>

class __attribute((annotate("reflect"))) Test {
public:
    Test() = default;
    void a(int) {}
};

int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
