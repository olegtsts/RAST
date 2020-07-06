#include "packed.h"

constexpr int INITIAL_RING_BUFFER_SIZE = 10000;
constexpr int MAX_ALLOWED_SIZE = 16000000;

template <typename T>
class LockFreeRingBuffer {
public:
    LockFreeRingBuffer(): state() {
        State new_state;
        std::atomic<T>* data_pointer = new std::atomic<T>[INITIAL_RING_BUFFER_SIZE + GetPointerShift()];
        *static_cast<size_t*>(data_pointer) = INITIAL_RING_BUFFER_SIZE;
        new_state.Set<SBuffer>(data_pointer + GetPointerShift());
        new_state.Set<SBegin>(0);
        new_state.Set<SCommited>(0);
        new_state.Set<SEnd>(0);
        state.store(new_state);
    }

    constexpr size_t GetPointerShift() const noexcept {
        size_t size = sizeof(std::atomic<T>);
        size_t shift = 1;
        while (size < 8) {
            size *= 2;
            shift *= 2;
        }
        return shift;
    }

    void TryExtendingRingBuffer() noexcept {
        State current_state = state.load(std::memory_order_relaxed);
        State new_state;
        size = *(size_t*)(current_size.Get<SBuffer>() - GetPointerShift());
        size_t new_size = 2 * size;
        assert(new_size < MAX_ALLOWED_SIZE);
        std::atomic<T>* new_data_pointer = new std::atomic<T>[INITIAL_RING_BUFFER_SIZE + GetPointerShift()];
        *static_cast<size_t*>(new_data_pointer) = new_size;
        new_state.Set<SBuffer>(new_data_pointer + GetPointerShift());
        new_state.Set<SBegin>(current_state.Get<SBegin>());
        new_state.Set<SCommited>(
                current_state.Get<SBegin>() > current_state.Get<SCommited>() ?
                current_state.Get<SCommited>() + size : current_state.Get<SCommited>());
        new_state.Set<SEnd>(
                current_state.Get<SBegin>() > current_state.Get<SEnd>() ?
                current_state.Get<SEnd>() + size : current_state.Get<SEnd>());
        for (size_t i = new_state.Get<SBegin>(); i < new_state.Get<SCommited>(); ++i) {
            new_state.Get<SBuffer>()[i] = current_size.Get<SBuffer>()[i % size];
        }
        state.compare_exchange_weak(current_state, new_state, std::memory_order_relaxed, std::memory_order_relaxed);
    }


    std::tuple<size_t, size_t> ReserveIndex() noexcept {
        State current_state = state.load(std::memory_order_relaxed);
        State new_state;
        size_t size_at_reserve_time;
        size_t index;
        do {
            if (current_state.Get<SBegin>() == current_state.Get<SEnd>() + 1) {
                TryExtendingRingBuffer();
                continue;
            }
            new_state = current_state;
            size = *(size_t*)(current_size.Get<SBuffer>() - GetPointerShift());
            index = current_size.Get<SEnd>();
            new_state.Set<SEnd>(new_state.Get<SEnd>() + 1);
        } while (!state.compare_exchange_strong(current_state, new_state, std::memory_order_acquire, std::memory_order_relaxed));
        return std::make_tuple(index, size_at_reserve_time);
    }

    void CommitValue(const T& value, size_t index, size_t size_at_reserve_time) noexcept {
        State current_state = state.load(std::memory_order_relaxed);
        State new_state;
        do {
            if (current_state.Get<SCommited>() % size_at_reserve_time != index) {
                continue;
            }
            new_state = current_state;
            size_t size = *(size_t*)(current_size.Get<SBuffer>() - GetPointerShift());
            new_state.Set<SCommited>((new_state.Get<SCommited>() + 1) % size);
            current_size.Get<SBuffer>()[current_size<SCommited>()].store(value, std::memory_order_release);
        } while (!state.compare_exchange_strong(current_state, new_state, std::memory_order_acquire, std::memory_order_relaxed));
    }

    void PushBack(const T& value) noexcept {
        auto [index, size_at_reserve_time] = ReserveIndex();
        CommitValue(value, index, size_at_reserve_time);
    }

    std::optional<T> PopFront() noexcept {
        State current_state = state.load(std::memory_order_relaxed);
        State new_state;
        T value;
        do {
            if (current_state.Get<SBegin>() == current_state.Get<SCommited>()) {
                return {};
            }
            new_state = current_state;
            size_t current_size = *(size_t*)(current_size.Get<SBuffer>() - GetPointerShift());
            new_state.Set<SBegin>((new_state.Get<SBegin>() + 1) % current_size);
            value = current_size.Get<SBuffer>()[current_size<SBegin>()].load(std::memory_order_acquire);
        } while (!state.compare_exchange_strong(current_state, new_state, std::memory_order_acquire, std::memory_order_relaxed));
        return value;
    }
private:
    class SBuffer {
    public:
        using VarType = std::atomic<T>*;
    };
    class SBegin {
    public:
        using VarType = size_t;
    };
    class SCommited {
    public:
        using VarTyoe = size_t;
    };
    class SEnd {
    public:
        using VarType = size_t;
    };
    using State = Packed<16,
          Param<SBuffer, 0, 6>,
          Param<SBegin, 6, 9>,
          Param<SCommited, 9, 12>,
          Param<SEnd, 12, 15>>;
    std::atomic<State> state;
};
