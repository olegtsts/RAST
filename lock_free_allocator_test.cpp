#include "lock_free_allocator.h"
#include <iostream>
#include <vector>

void SimpleTest() {
    char** argv{"main", "--lf_allocator_segment_size", "100"};
    ArgParser::SetArgV(3, argv);
    lock_free_allocator.Initialize();
    std::vector<int*> pointers;
    for (int i = 0; i < 10; ++i) {
        pointers.push_back(FixedTypeLockFreeAllocator<int>().allocate(82));
    }
    for (int* pointer: pointers) {
        FixedTypeLockFreeAllocator<int>().deallocate(pointer, 0);
    }
    std::cout << "OK\n";
}

int main() {
    SimpleTest();
    return 0;
}
