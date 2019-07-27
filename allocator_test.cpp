#include "allocator.h"
#include <iostream>
#include <map>
#include <vector>
#include <utility>

void TestWithVector() {
    std::vector<int, FixedFreeListMultiLevelAllocator<int>> v;
    std::cout << "first vector\n";
    for (int i = 0; i < 10; ++i) {
        v.push_back(1);
    }
    std::cout << "second vector\n";
    std::vector<int, FixedFreeListMultiLevelAllocator<int>> v2;
    for (int i = 0; i < 10; ++i) {
        v2.push_back(1);
    }
    std::cout << "map\n";
    std::map<float, double, std::less<float>, FixedFreeListMultiLevelAllocator<std::pair<float, double>>> m1;
    for (int i = 0; i < 10; ++i) {
            m1[i] = 1.;
    }
}

void SimpleTest() {
    std::vector<int*> pointers;
    for (int i = 0; i < 10; ++i) {
        pointers.push_back(FixedFreeListMultiLevelAllocator<int>().allocate(82));
    }
    for (int* pointer: pointers) {
        FixedFreeListMultiLevelAllocator<int>().deallocate(pointer, 0);
    }
}

int main() {
    TestWithVector();
    SimpleTest();
    std::cout << "OK\n";
    return 0;
}
