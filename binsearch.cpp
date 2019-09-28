#include <iostream>
#include <iomanip>

extern "C" {
    double BinarySearch(double target, double left, double right, int reps);
}

int main() {
    std::cout << "[Binary Search] √2 = " << std::setprecision(10) << BinarySearch(2.0, 1.0, 2.0, 60) << std::endl;
    std::cout << "[Binary Search] √3 = " << std::setprecision(10) << BinarySearch(3.0, 1.0, 2.0, 60) << std::endl;
    std::cout << "[Binary Search] √5 = " << std::setprecision(10) << BinarySearch(5.0, 2.0, 3.0, 60) << std::endl;

    return 0;
}
