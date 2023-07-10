#include <unistd.h>
#include <memory>
#include <new>
#include <atomic>
#include <cassert>
#include <tuple>
#include <string>

#include "argparser.h"
#include "lock_free_allocator.h"

LockFreeAllocator lock_free_allocator;

LockFreeAllocator::LockFreeAllocator()
: segment_size(GetSegmentSize())
, initialized(false)
{
    allocator_control.store(AllocatorControl{CreateSegment(), 1});
    static_assert(SegmentControl::is_always_lock_free());
    static_assert(std::atomic<AllocatorControl>::is_always_lock_free());
}

size_t LockFreeAllocator::GetSystemMemory() {
    size_t pages = static_cast<size_t>(sysconf(_SC_PHYS_PAGES));
    size_t page_size = static_cast<size_t>(sysconf(_SC_PAGE_SIZE));
    return pages * page_size;
}

struct LfAllocatorSegmentSize {
    std::string name = "lf_allocator_segment_size";
    std::string description = "Size of minimal allocated segment for Lock Free Allocator. "
        "If not specified or zero, a reasonable default is used.";
    using type = size_t;
    using default_value = 0;
};

void LockFreeAllocator::Initialize() {
    assert(!initialized);
    size_t hint_size = ArgParser::GetValue<LfAllocatorSegmentSize>();
    if (hint_size == 0) {
        size_t system_memory = GetSystemMemory();
        hint_size = system_memory / 10;
    }
    initialized = true;
    segment_size = std::clamp(system_memory / 10, min_segment_size, max_segment_size);
}

size_t LockFreeAllocator::GetSegmentSize() {
    size_t system_memory = GetSystemMemory();
    return std::clamp(system_memory / 10, min_segment_size, max_segment_size);
}

SegmentControl* LockFreeAllocator::CreateSegment() const {
    char* arena = new char[segment_size];
    void* aligned_segment_control = reinterpret_cast<void*>(arena + 1);
    assert(std::align(alignof(SegmentControl), sizeof(SegmentControl), aligned_segment_control, segment_size - 1));
    size_t shift = reinterpret_cast<char*>(aligned_segment_control) - arena;
    *(reinterpret_cast<char*>(aligned_segment_control) - 1) = static_cast<char>(shift);
    SegmentControl* segment_control = new (aligned_segment_control) SegmentControl();
    SegmentControlData segment_control_data;
    segment_control_data->Set<SCFreed>(sizeof(SegmentControl));
    segment_control_data->Set<SCEnd>(sizeof(SegmentControl));
    segment_control_data->Set<SCUsageCounter>(usage_counter_offset - 1);
    segment_control->store(segment_control_data);
    return segment_control;
}

void LockFreeAllocator::DeleteSegment(SegmentControl* segment_control) {
    size_t shift = static_cast<size_t>(*(reinterpret_cast<char*>(segment_control) - 1));
    char* arena = reinterpret_cast<char*>(segment_control) - shift;
    delete [] arena;
}

void LockFreeAllocator::TryDeleteSegment(const SegmentControl* segment_control, const SegmentControlData& segment_control_data) {
    size_t freed = segment_control_data.Get<SCFreed>();
    size_t end = segment_control_data.Get<SCEnd>();
    int64_t usage_counter = segment_control_data.Get<SCUsageCounter>() - usage_counter_offset;
    if (freed != end) {
        // All allocations were freed.
        return;
    }
    if (usage_counter != 0) {
        // SegmentControl is not used by any AllocatorControl.
        // No allocations are in progress.
        return;
    }
    DeleteSegment(segment_control);
}

void LockFreeAllocator::ReallocateSegment() {
    // Creates new SegmentControl.
    SegmentControl* new_segment_control = CreateSegment();

    // Attempts to switch AllocatorControl to new SegmentControl.
    AllocatorControl old_allocator_control = allocator_control.load(std::memory_order_relaxed);
    SegmentControl* original_segment_control = old_allocator_control.segment_control;
    AllocatorControl new_allocator_control{new_segment_control, 1ll};
    while (
        old_allocator_control.segment_control == original_segment_control &&
        !allocator_control.compare_exchange_strong(
            old_allocator_control, new_allocator_control,
            std::memory_order_acquire, std::memory_order_relaxed
        )
    ) {
    }

    if (old_allocator_control.segment_control != original_segment_control) {
        // Another thread already switched AllocatorControl to a new SegmentControl.
        // The rest of relocation logic will be handled by the other thread.
        DeleteSegment(new_allocator_control);
        return;
    }

    // Releases usage counter from previous AllocatorControl into previous SegmentControl.
    // This signifies committement that previous SegmentControl is no longer used by AllocatorControl.
    SegmentControl* prev_segment_control = old_allocator_control.segment_control;
    int64_t usage_counter = old_allocator_control.usage_counter;
    SegmentControlData old_segment_control_data = prev_segment_control->load(std::memory_order_relaxed);
    SegmentControlData new_segment_control_data;
    do {
        new_segment_control_data = old_segment_control_data;
        new_segment_control_data.Set<SCUsageCounter>(new_segment_control_data.Get<SCUsageCounter>() + usage_counter);
    } while (
        !prev_segment_control->compare_exchange_strong(
            old_segment_control_data, new_segment_control_data,
            std::memory_order_acquire, std::memory_order_relaxed
        )
    );

    // Attempts to delete previous SegmentControl after updating usage counter.
    TryDeleteSegment(prev_segment_control, new_segment_control_data);
}

LockFreeAllocator::SegmentControlCopyGuard::SegmentControlCopyGuard(std::atomic<AllocatorControl>& allocator_control) {
    // SegmentControlCopyGuard incraeses usage_counter of AllocatorControl on contruction.
    AllocatorControl old_allocator_control = allocator_control.load();
    AllocatorControl new_allocator_control;
    do {
        new_allocator_control = old_allocator_control;
        ++new_allocator_control.usage_counter;
    } while (
        !allocator_control.compare_exchange_strong(
            old_allocator_control,
            new_allocator_control,
            std::memory_order_acquire,
            std::memory_order_relaxed
        )
    );
    copied_segment_control = new_allocator_control.segment_control;
}

SegmentControl* LockFreeAllocator::SegmentControlCopyGuard::GetCopy() {
    return copied_segment_control;
}

LockFreeAllocator::SegmentControlCopyGuard::~SegmentControlCopyGuard() {
    // SegmentControlCopyGuard decreases usage_counter of SegmentControl on destruction.
    // Total sum of usage_counter of AllocatorControl and SegmentControl remains the same.
    SegmentControlData old_segment_control_data = copied_segment_control->load(std::memory_order_relaxed);
    SegmentControlData new_segment_control_data;
    do {
        new_segment_control_data = old_segment_control_data;
        new_segment_control_data.Set<SCUsageCounter>(new_segment_control_data.Get<SCUsageCounter>() - 1);
    } while (
        !copied_segment_control->compare_exchange_strong(
            old_segment_control_data,
            new_segment_control_data,
            std::memory_order_acquire,
            std::memory_order_relaxed
        )
    );
    LockFreeAllocator::TryDeleteSegment(copied_segment_control, new_segment_control_data);
}

std::tuple<FrontControl*, char*, bool> LockFreeAllocator::GetAlignedFrontControlAndData(
    char * arena, size_t arena_size, size_t data_size, size_t alignment, size_t struct_size
) const {
    void* front_control_location = reinterpret_cast<void *>(arena + 1);
    if (!std::align(alignof(FrontControl), sizeof(FrontControl), front_control_location, arena_size - 1)) {
        return {nullptr, nullptr, false};
    }
    void* data_location = reinterpret_cast<void*>(reinterpret_cast<char*>(front_control_location) + sizeof(FrontControl));
    if (!std::align(alignment, struct_size, data_location, arena + arena_size - reinterpret_cast<char*>(data_location))) {
        return {nullptr, nullptr, false};
    }
    return {reinterpret_cast<FrontControl*>(front_control_location), reinterpret_cast<char*>(data_location), true};
}

void* LockFreeAllocator::Allocate(size_t size, size_t alignment, size_t struct_size) {
    assert(initialized);
    if (size >= segment_size / 2) {
        // The object is too large, we use simple allocation without SegmentControl attachment.
        const size_t arena_size = size + 1 + alignment + sizeof(FrontControl) + alignof(FrontControl);
        char* arena = new char[arena_size];
        FrontControl* front_control;
        char* data_location;
        bool enough_memory;
        std::tie(front_control, data_location, enough_memory) = GetAlignedFrontControlAndData(
            arena,
            arena_size,
            size,
            alignment,
            struct_size
        );
        assert(enough_memory);
        front_control->Set<FCSegmentStart>(nullptr);
        *(reinterpret_cast<char*>(front_control) - 1) = static_cast<char>(static_cast<char*>(front_control) - arena);
        *(reinterpret_cast<char*>(data_location) - 1) = static_cast<char>(data_location - reinterpret_cast<char *>(front_control) - sizeof(FrontControl));
        return data_location;
    }

    while(true) {
        SegmentControlCopyGuard segment_control_copy_guard{allocator_control};
        SegmentControl* segment_control = segment_control_copy_guard.GetCopy();
        SegmentControlData old_segment_control_data = segment_control->load(std::memory_order_relaxed);
        SegmentControlData new_segment_control_data;
        FrontControl* front_control;
        char* data_location;
        bool enough_memory = true;
        do {
            new_segment_control_data = old_segment_control_data;
            std::tie(
                front_control,
                data_location,
                enough_memory
            ) = GetAlignedFrontControlAndData(
                reinterpret_cast<char*>(segment_control) + new_segment_control_data.Get<SCEnd>(),
                segment_size - new_segment_control_data.Get<SCEnd>(),
                size,
                alignment,
                struct_size
            );
            if (!enough_memory) {
                break;
            }
            new_segment_control_data->Set<SCEnd>(
                data_location + size - reinterpret_cast<char*>(segment_control)
            );
        } while (!segment_control->compare_exchange_strong(
            old_segment_control_data,
            new_segment_control_data,
            std::memory_order_acquire,
            std::memory_order_relaxed
        ));
        if (!enough_memory) {
            ReallocateSegment();
            continue;
        }
        *(reinterpret_cast<char*>(data_location) - 1) = static_cast<char>(data_location - reinterpret_cast<char *>(front_control) - sizeof(FrontControl));
        front_control->Set<FCSegmentStart>(segment_control);
        front_control->Set<FCDataSize>(new_segment_control_data.Get<SCEnd>() - old_segment_control_data.Get<SCEnd>());
        return data_location;
    }
}

void LockFreeAllocator::Deallocate(void* data_location) {
    assert(initialized);
    size_t shift = static_cast<size_t>(*(reinterpret_cast<char*>(data_location) - 1));
    FrontControl* front_control = reinterpret_cast<FrontControl*>(reinterpret_cast<char*>(data_location) - shift - sizeof(FrontControl));
    SegmentControl* segment_control = front_control->Get<FCSegmentStart>();

    if (segment_control == nullptr) {
        // In this case SegmentControl wasn't attached and we can delete the memory right away.
        size_t fc_shift = static_cast<size_t>(*(reinterpret_cast<char*>(front_control) - 1));
        char* arena = reinterpret_cast<char*>(front_control) - fc_shift;
        delete [] arena;
        return;
    }

    // In this case SegmentControl is attached.
    // Freed counter of the attached SegmentControl is incremented.
    SegmentControlData old_segment_control_data = segment_control->load(std::memory_order_relaxed);
    SegmentControlData new_segment_control_data;
    do {
        new_segment_control_data = old_segment_control_data;
        new_segment_control_data.Set<SCFreed>(
            new_segment_control_data.Get<SCFreed>() +
            front_control->Get<FCDataSize>()
        );
    } while (!segment_control->compare_exchange_strong(
        old_segment_control_data,
        new_segment_control_data,
        std::memory_order_acquire,
        std::memory_order_relaxed
    ));

    // If all the memory is freed, SegmentControl can be deleted.
    TryDeleteSegment(segment_control, new_segment_control_data);
}
