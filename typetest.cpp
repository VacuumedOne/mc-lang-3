#include <iostream>

extern "C" {
    double typetest(double x, int y);
}

int main() {
    std::cout << "answer = " << typetest(1.2, 1) << std::endl;

    return 0;
}
