#include <cstddef>
#include <cstdlib>

int main() {
    int* pointers[1000];
    for (int i = 0; i < 1000; ++i) {
        pointers[i] = new int[1];
        new int[30000];
    }
    for (int i = 0; i < 10000000; ++i) {
        *pointers[rand() % 1000] += 1;
    }
    return 0;
}
