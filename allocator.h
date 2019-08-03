#pragma once
#include <tuple>

constexpr int MAX_MEM_LAYERS = 50;
constexpr int MEM_ALLOCATED_AT_ONCE = 10000000;

struct FrontControl {
    size_t data_size;
    FrontControl* local_next;
    FrontControl* local_prev;
    size_t offset;
    size_t total_size;
    int source_layer;
    bool is_owned;
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
    template <typename U>
    FixedFreeListMultiLevelAllocator(const FixedFreeListMultiLevelAllocator<U>&) noexcept {
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

