#pragma once

#include <atomic>
#include <tuple>

#include "packed.h"

class LockFreeAllocator {
private:
    class SCFreed {
        using VarType=size_t;
    };

    class SCEnd {
        using VarType=size_t;
    };

    class SCUsageCounter {
        using VarType=int64_t;
    };

    // 5 bytes for size covers 4GB of memory.
    using SegmentControlData=Packed<16, Param<SCFreed, 0, 5>, Param<SCEnd, 5, 10>, Param<SCUsageCounter, 10, 16>>>
    using SegmentControl=std::atomic<SegmentControlData>;

    class FCSegmentStart {
        using VarType=SegmentControl*;
    };

    class FCDataSize {
        using VarType=size_t;
    };

    // Size has to not be divisible by 8, because the last byte might get overridden with the pointer alignment offset.
    using FrontControl=Packed<12, Param<FCSegmentStart, 0, 6>, Param<FCDataSize, 6, 12>>;

    class AllocatorControl {
        SegmentControl* segment_control;
        int64_t usage_counter;
    };

    struct SegmentControlCopyGuard {
    public:
        SegmentControlCopyGuard(std::atomic<AllocatorControl>& allocator_control);

        SegmentControlCopyGuard(const SegmentControlCopyGuard&) = delete;
        SegmentControlCopyGuard(SegmentControlCopyGuard&&) = delete;
        SegmentControlCopyGuard& operator=(const SegmentControlCopyGuard&) = delete;

        SegmentControl* GetCopy() const;

        ~SegmentControlCopyGuard();
    private:
        SegmentControl* copied_segment_control;
    };
private:
    static size_t GetSystemMemory();
    static size_t GetSegmentSize();
    static void DeleteSegment(SegmentControl*);
    static void TryDeleteSegment(SegmentControl*, const SegmentControlData&);

private:
    void* Allocate(size_t size, size_t alignment, size_t struct_size);
    void Deallocate(void* pointer);
    SegmentControl* CreateSegment() const;
    void ReallocateSegment();
    std::tuple<FrontControl*, char*, bool> GetAlignedFrontControlAndData(
            char * arena, size_t arena_size, size_t data_size, size_t alignment, size_t struct_size) const;

public:
    LockFreeAllocator();
    LockFreeAllocator(const LockFreeAllocator&) = delete;
    LockFreeAllocator(LockFreeAllocator&&) = delete;
    LockFreeAllocator& operator=(const LockFreeAllocator&) = delete;

    // Should be called after ArgParser initialization in main.
    void Initialize();

    template <typename T>
    T* Allocate(size_t size) {
        static_assert(alignof(T) % 8 == 0 || 8 % alignof(T) == 0);
        return reinterpret_cast<T*>(Allocate(size * sizeof(T), alignof(T), sizeof(T)));
    }

    template <typename T >
    void Deallocate(T* pointer) noexcept {
        Deallocate(reinterpret_cast<void*>(pointer));
    }
private:
    static constexpr size_t max_segment_size = 4000000000;
    static constexpr size_t min_segment_size = 10000000;
    // Offset is 2**63 to avoid negative counter values.
    static constexpr int64_t usage_counter_offset = 9223372036854775808ll;

private:
    size_t segment_size;
    bool initialized;
    std::atomic<AllocatorControl> allocator_control;
};

extern LockFreeAllocator lock_free_allocator;

template <typename T>
class FixedTypeLockFreeAllocator {
public:
    FixedTypeLockFreeAllocator() noexcept {
    }
    FixedTypeLockFreeAllocator(const FixedTypeLockFreeAllocator&) noexcept {
    }
    template <typename U>
    FixedTypeLockFreeAllocator(const FixedTypeLockFreeAllocator<U>&) noexcept {
    }
    T* allocate (const size_t n, const void* hint = nullptr) {
        return lock_free_allocator.Allocate<T>(n);
    }
    void deallocate (T* p, size_t n) noexcept {
        lock_free_allocator.Deallocate(p);
    }
    template <typename T2>
    bool operator== (const FixedTypeLockFreeAllocator<T2>& other) const noexcept {
        return true;
    }
    template <typename T2>
    bool operator!= (const FixedTypeLockFreeAllocator<T2>& other) const noexcept {
        return false;
    }
    using value_type=T;
    using size_type=size_t;
    using difference_type=std::ptrdiff_t;
};
