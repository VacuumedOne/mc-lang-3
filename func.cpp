#include <iostream>

extern "C" {
    int f1(int x, int y);
    double f2(int x, double y);
    double f3(double x, int y);
    double f4(double x, double y);
    double f5(int x, int y);
    int f6(double x, double y);
}

int main() {
    std::cout << "f1 = " << f1(1, 1) << std::endl;
    std::cout << "f2 = " << f2(1, 1.0) << std::endl;
    std::cout << "f3 = " << f3(1.0, 1) << std::endl;
    std::cout << "f4 = " << f4(1.0, 1.0) << std::endl;
    std::cout << "f5 = " << f5(1, 1) << std::endl;
    std::cout << "f6 = " << f6(1.0, 1.0) << std::endl;

    return 0;
}
