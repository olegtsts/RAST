#include <atomic>
#include <memory>

template <typename T>
class LockFreeQueue {
private:
    struct Node;

    struct CountedNodePtr {
        long long int external_count = 0;
        Node* ptr = nullptr;
    };

    struct NodeCounter {
        signed internal_count:30;
        unsigned external_counters:2;
    };

    struct Node {
        std::atomic<T*> data;
        std::atomic<NodeCounter> count;
        std::atomic<CountedNodePtr> next;

        Node()
        : data(nullptr)
        {
            count.store(NodeCounter{0, 3});
            next.store(CountedNodePtr{0, nullptr});
        }
    };

    struct CountedNodePtrCopyGuard {
    public:
        CountedNodePtrCopyGuard(std::atomic<CountedNodePtr>& counter) {
            copied_counter = counter.load();
            CountedNodePtr new_counter;
            do {
                if (copied_counter.ptr == nullptr) {
                    return;
                }
                new_counter = copied_counter;
                ++new_counter.external_count;
            } while (!counter.compare_exchange_strong(copied_counter, new_counter,
                                                      std::memory_order_acquire,
                                                      std::memory_order_relaxed));
            copied_counter.external_count = new_counter.external_count;
        }

        CountedNodePtrCopyGuard& operator=(const CountedNodePtrCopyGuard&) = delete;
        CountedNodePtrCopyGuard(const CountedNodePtrCopyGuard&) = delete;

        CountedNodePtr GetCopy() {
            return copied_counter;
        }

        ~CountedNodePtrCopyGuard() {
            if (copied_counter.ptr != nullptr) {
                NodeCounter old_counter = copied_counter.ptr->count.load(std::memory_order_relaxed);
                NodeCounter new_counter;
                do {
                    new_counter = old_counter;
                    --new_counter.internal_count;
                } while (!copied_counter.ptr->count.compare_exchange_strong(old_counter, new_counter,
                            std::memory_order_acquire, std::memory_order_relaxed));
                if (!new_counter.internal_count && !new_counter.external_counters) {
                    delete copied_counter.ptr;
                }
            }
        }
    private:
        CountedNodePtr copied_counter;
    };

    static void FreeExternalCounter(CountedNodePtr old_node_ptr) {
        Node* ptr = old_node_ptr.ptr;
        int count_increase = old_node_ptr.external_count - 1;
        NodeCounter old_counter = ptr->count.load(std::memory_order_relaxed);
        NodeCounter new_counter;
        do {
            new_counter = old_counter;
            --new_counter.external_counters;
            new_counter.internal_count += count_increase;
        } while (!ptr->count.compare_exchange_strong(old_counter, new_counter,
                std::memory_order_acquire, std::memory_order_relaxed));

        if (!new_counter.internal_count && !new_counter.external_counters) {
            delete ptr;
        }
    }

    void SetNewTail(CountedNodePtr old_tail, CountedNodePtr new_tail) {
        Node* const current_tail_ptr = old_tail.ptr;
        new_tail.external_count = 1;
        while (!tail.compare_exchange_weak(old_tail, new_tail) && old_tail.ptr == current_tail_ptr) {}
        if (old_tail.ptr == current_tail_ptr) {
            FreeExternalCounter(old_tail);
        }
    }
public:
    LockFreeQueue() {
        CountedNodePtr empty_node;
        empty_node.ptr = new Node();
        empty_node.external_count = 1;
        head.store(empty_node);
        tail.store(empty_node);
        FreeExternalCounter(empty_node);
    }

    void Push(std::unique_ptr<T>&& new_value) {
        CountedNodePtr new_next;
        new_next.ptr = new Node();
        new_next.external_count = 1;

        while (true) {
            CountedNodePtrCopyGuard tail_copy(tail);
            CountedNodePtr old_tail = tail_copy.GetCopy();
            T* old_data = nullptr;
            if (old_tail.ptr->data.compare_exchange_strong(old_data, new_value.get())) {
                CountedNodePtr old_next{0, nullptr};
                if (!old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                    delete new_next.ptr;
                    new_next = old_next;
                }
                SetNewTail(old_tail, new_next);
                new_value.release();
                break;
            } else {
                CountedNodePtr old_next{0, nullptr};
                if (old_tail.ptr->next.compare_exchange_strong(old_next, new_next)) {
                    old_next = new_next;
                    new_next.ptr = new Node();
                }
                SetNewTail(old_tail, old_next);
            }
        }
    }

    template <typename Function>
    std::unique_ptr<T> PopWithHeadDataCallback(Function current_head_data_callback) {
        while (true) {
            CountedNodePtrCopyGuard head_copy(head);
            CountedNodePtr old_head = head_copy.GetCopy();
            Node * ptr = old_head.ptr;
            if (ptr == tail.load().ptr) {
                return std::unique_ptr<T>();
            }
            CountedNodePtrCopyGuard next_copy(ptr->next);
            CountedNodePtr old_next = next_copy.GetCopy();
            if (old_next.ptr != nullptr) {
                CountedNodePtr new_head = old_next;
                new_head.external_count = 1;
                current_head_data_callback(*old_head.ptr->data);
                if (head.compare_exchange_strong(old_head, new_head)) {
                    T* res = ptr->data.exchange(nullptr);
                    while (!ptr->next.compare_exchange_weak(old_next, CountedNodePtr{1, nullptr})) {}
                    FreeExternalCounter(old_head);
                    FreeExternalCounter(old_next);
                    return std::unique_ptr<T>(res);
                }
            }
        }
    }

    std::unique_ptr<T> Pop() noexcept {
        return PopWithHeadDataCallback([](const T&){});
    }

    ~LockFreeQueue() {
        while (true) {
            auto popped = Pop();
            if (!popped) {
                break;
            }
        }
        FreeExternalCounter(head.load());
        FreeExternalCounter(tail.load());
    }
private:
    std::atomic<CountedNodePtr> head;
    std::atomic<CountedNodePtr> tail;
};

