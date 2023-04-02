#include "../include/std_function_c++17.hpp"
#include <iostream>

using namespace X17;

void print(int a, int b) {
    std::cout << a + b << "\n";
}

int main() {
    function<void(int, int)> func0;
    func0 = print;
    func0(120182, 24893);

    function<void(int, int)> func = func0;
    func(10, 15);

    return 0;
}