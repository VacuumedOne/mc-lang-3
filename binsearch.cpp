#include <iostream>

extern "C" {
    double BinarySearch(double target, double left, double right, int reps);
}

int main() {
    std::cout << "[Binary Search] √2 = " << BinarySearch(2.0, 1.0, 2.0, 30) << std::endl;
    std::cout << "[Binary Search] √3 = " << BinarySearch(3.0, 1.0, 2.0, 30) << std::endl;
    std::cout << "[Binary Search] √5 = " << BinarySearch(5.0, 2.0, 3.0, 30) << std::endl;

    return 0;
}
