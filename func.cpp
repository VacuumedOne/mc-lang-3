#include <iostream>

extern "C" {
    int f1(int x, int y);
    double f2(int x, double y);
    double f3(double x, int y);
    double f4(double x, double y);
    double f5();
}

int main() {
    std::cout << "f1 = " << f1(1, 2) << std::endl;
    std::cout << "f2 = " << f2(1, 2.2) << std::endl;
    std::cout << "f3 = " << f3(1.1, 2) << std::endl;
    std::cout << "f4 = " << f4(1.1, 2.2) << std::endl;
    std::cout << "f5 = " << f5() << std::endl;

    return 0;
}
