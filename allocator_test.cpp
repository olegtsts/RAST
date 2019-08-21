#include "allocator.h"
#include <iostream>
#include <map>
#include <vector>
#include <utility>
#include <cstdlib>
#include <new>
#include <atomic>
#include <deque>
#include <cstring>

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
    std::map<float, double, std::less<float>, FixedFreeListMultiLevelAllocator<std::pair<const float, double>>> m1;
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
    std::cout << "OK\n";
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
    std::cout << "OK\n";
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
    std::cout << "OK\n";
}

struct StructWith16Aligment {
    int a;
    struct Struct16Bytes {
        int64_t a;
        int64_t b;
    };
    std::atomic<Struct16Bytes> b;
};

void TestWith16Alignment() {
    std::vector<StructWith16Aligment* > pointers;
    for (int i = 0; i < 10; ++i) {
        StructWith16Aligment * pointer = FixedFreeListMultiLevelAllocator<StructWith16Aligment>().allocate(1);
        new(pointer) StructWith16Aligment();
        pointer->b.store({});
        pointers.push_back(pointer);
    }
    for (StructWith16Aligment * pointer : pointers) {
        FixedFreeListMultiLevelAllocator<StructWith16Aligment>().deallocate(pointer, 1);
    }
    std::cout << "OK\n";
}

void RandomAllocationTest() {
    for (int i = 0; i < 100; ++i) {
        std::vector<char *> char_pointers;
        std::vector<uint64_t *> uint_pointers;
        for (int j = 0; j < rand() % 100; ++j) {
            size_t size = rand() % 500 + 1;
            char* pointer = FixedFreeListMultiLevelAllocator<char>().allocate(size);
            memset(pointer, '\0', size);
            char_pointers.push_back(pointer);
        }
        for (int j = 0; j < rand() % 100; ++j) {
            size_t size = rand() % 500 + 1;
            uint64_t* pointer = FixedFreeListMultiLevelAllocator<uint64_t>().allocate(size);
            memset(pointer, '\0', size * sizeof(uint64_t));
            uint_pointers.push_back(pointer);
        }
        for (auto pointer : char_pointers) {
            FixedFreeListMultiLevelAllocator<char>().deallocate(pointer, 1);
        }
        for (auto pointer : uint_pointers) {
            FixedFreeListMultiLevelAllocator<uint64_t>().deallocate(pointer, 1);
        }
    }
}

int main() {
    TestWith16Alignment();
    TestWithStdStructs();
    SimpleTest();
    srand(0);
    RandomAllocationTest();
    CrossReferenceTest1();
    CrossReferenceTest2();
    return 0;
}
