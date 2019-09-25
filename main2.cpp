#include <iostream>

extern "C" {
    double func(double x, int y);
}

int main() {
    std::cout << "answer = " << func(1.2, 1) << std::endl;

    return 0;
}
