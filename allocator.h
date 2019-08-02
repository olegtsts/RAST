#pragma once
#include <tuple>

#include "packed.h"

constexpr int MAX_MEM_LAYERS = 50;
constexpr int MEM_ALLOCATED_AT_ONCE = 10000000;

constexpr int FIRST_BLOCK_BIT = 1;
constexpr int LAST_BLOCK_BIT = 2;
constexpr int IS_OWNED_BIT = 4;

class FCDataSize {
public:
    using VarType=size_t;
};

class FCLocalNext;

class FCLocalPrev;

class FCSourceLayer {
public:
    using VarType=unsigned int;
};

class FCState {
public:
    using VarType=unsigned int;
};

using FrontControl=Packed<20,
      Param<FCDataSize, 0, 6>,
      Param<FCLocalNext, 6, 12>,
      Param<FCLocalPrev, 12, 18>,
      Param<FCSourceLayer, 18, 19>,
      Param<FCState, 19, 20>>;

class FCLocalNext {
public:
    using VarType=FrontControl*;
};

class FCLocalPrev {
public:
    using VarType=FrontControl*;
};

struct BackControl {
    FrontControl* front_control;
};

class FreeListMultiLevelAllocator {
private:
    size_t GetLog2(size_t number);
    void Attach(FrontControl* front_control);
    void Detach(FrontControl* front_control);
    BackControl* GetBackControl(FrontControl* front_control);
    void Join(FrontControl* first_block, FrontControl* second_block);
    void SplitBlock(FrontControl* front_control, size_t first_size);
    void* Allocate(const size_t size);
    void Deallocate(void* pointer);
public:
    FreeListMultiLevelAllocator();
    FreeListMultiLevelAllocator (const FreeListMultiLevelAllocator &) = delete;
    FreeListMultiLevelAllocator(FreeListMultiLevelAllocator&&) = delete;
    FreeListMultiLevelAllocator & operator =(const FreeListMultiLevelAllocator &) = delete;

    template <typename T>
    T* Allocate(const size_t size) {
        return reinterpret_cast<T*>(Allocate(size * sizeof(T)));
    }

    template <typename T >
    void Deallocate(T* pointer) noexcept {
        Deallocate(reinterpret_cast<void*>(pointer));
    }

    std::string DebugString() const;
private:
    FrontControl* layers[MAX_MEM_LAYERS];
};

extern thread_local FreeListMultiLevelAllocator global_allocator;

template <typename T>
class FixedFreeListMultiLevelAllocator {
public:
    FixedFreeListMultiLevelAllocator() noexcept {
    }
    FixedFreeListMultiLevelAllocator(const FixedFreeListMultiLevelAllocator&) noexcept {
    }
    T* allocate (const size_t n, const void* hint = nullptr) {
        return global_allocator.Allocate<T>(n);
    }
    void deallocate (T* p, size_t n) noexcept {
        global_allocator.Deallocate(p);
    }
    using value_type=T;
    using size_type=size_t;
    using difference_type=std::ptrdiff_t;
};

