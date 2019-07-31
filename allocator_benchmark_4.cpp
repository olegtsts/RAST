#include "allocator.h"
#include <cstdlib>

int main() {
    int* pointers[1000];
    for (int i = 0; i < 1000; ++i) {
        pointers[i] = FixedFreeListMultiLevelAllocator<int>().allocate(1);
        FixedFreeListMultiLevelAllocator<int>().allocate(30000);
    }
    for (int i = 0; i < 10000000; ++i) {
        *pointers[rand() % 1000] += 1;
    }
    return 0;
}
