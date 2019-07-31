#include <cstddef>
#include <iostream>
#include <cstdlib>

int main() {
    for (size_t i = 0; i < 100000; ++i) {
        int *a = new int[100];
        for (size_t j = 0; j < 100; ++j) {
            a[rand() % 100] = j;
        }
    }
    std::cout << "OK\n";
    return 0;
}
