#include "allocator.h"
#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <cstdlib>
#include <deque>

void TestWithStdStructs() {
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
    std::cout << "deque\n";
    std::deque<float, FixedFreeListMultiLevelAllocator<float>> deq;
    for (int i = 0; i < 10; ++i) {
        deq.push_back(i);
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

void CrossReferenceTest1() {
    int* pointers[1000];
    int* pointers2[1000];
    for (int i = 0; i < 1000; ++i) {
        pointers[i] = new int[1];
        pointers2[i] = new int[30000];
    }
    for (int i = 0; i < 10000000; ++i) {
        *pointers[rand() % 1000] += 1;
    }
    pointers2[0]++;
}

void CrossReferenceTest2() {
    int* pointers[1000];
    for (int i = 0; i < 1000; ++i) {
        pointers[i] = FixedFreeListMultiLevelAllocator<int>().allocate(1);
        FixedFreeListMultiLevelAllocator<int>().allocate(30000);
    }
    for (int i = 0; i < 10000000; ++i) {
        *pointers[rand() % 1000] += 1;
    }
}


int main() {
    TestWithStdStructs();
    SimpleTest();
    srand(0);
    CrossReferenceTest1();
    CrossReferenceTest2();
    std::cout << "OK\n";
    return 0;
}
