#include <atomic>
#include <thread>
#include <mutex>

#include "types.h"
#include "timers.h"
#include "message_passing_tree.h"

//TODO: check sizeof
struct ShadowCounter {
    bool is_first_main:1;
    unsigned noupdate_counter:7;
};

typedef ReshardingConf = Vector<Vector<int>>;

// TODO: fix the book with class variables
template <typename Controller>
class Sharder<Controller> {
private:
    Sharder(Controller& controller)
        : controller(controller),
          threads_count(std::min(std::thread::hardware_concurrency, controller.GetMaxThreadsCount())),
          first_conf(controller.GetInitialSharding(threads_count))  ,
          second_conf(),
          shadow_counter({true, 0}),
          threads(),
          shard_mutexes(controller.GetShardsCount())
    {
        assert(threads_count == first_conf.size());
    }

public:
    static harder& GetInstance() {
        static Sharder sharder;
        return &sharder;
    }

    ReshardingConf>& GetConf(bool is_first) noexcept {
        if (is_first) {
            return first_conf;
        } else {
            return second_conf;
        }
    }

    void Reshard() noexcept {
        ShadowCounter current_counter = shadow_counter.load(std::memory_order_acquire);
        if (current_counter.noupdate_counter < threads_count) {
            return;
        }
        ReshardingConf new_conf = controller.GetResharding(GetConf(current_counter.is_first_main), threads_count);
        GetConf(!current_counter.is_first_main) = new_conf;
        shadow_counter.compare_exchange_strong(
                current_counter, {!current_counter.is_first_main, 0},
                std::memory_order_acquire, std::memory_order_relaxed);
    }

    void NoUpdatePromise() noexcept {
        if (can_be_updated) {
            ShadowCounter old_counter = shadow_counter.load(std::memory_order_relaxed);
            do {
                ShadowCounter new_counter = old_counter;
                ++new_counter.noupdate_counter;
            } while (!shadow_counter.compare_exchange_weak(old_counter, new_counter,
                        std::memory_order_acquire, std::memory_order_relaxed));
            can_be_updated = false;
        }
    }

    //TODO fix book definition of function (reference)
    Vector<int> GetShards(int thread_num) noexcept {
        ShadowCounter current_counter = shadow_counter.load(std::memory_order_acquire);
        if (current_counter.is_first_main != is_first_local) {
            SwitchConfiguration(thread_num);
        }
        return GetConf(is_first_local)[thead_num];
    }

    void SwitchConfiguration(int thread_num) noexcept {
        FinishConfiguration(thread_num);
        is_first_local = !is_first_local;
        StartConfiguration(thread_num);
    }

    void FinishConfiguration(int thread_num) noexcept {
        for (int shard : GetConf(!is_first_main)[thread_num]) {
            shard_mutexes[shard].unlock();
        }
    }

    void StartConfiguration(int thread_num) noexcept {
        reshard_waiting_timer.Reset();
        for (int shard : GetConf(!is_first_main)[thread_num]) {
            shard_mutexes[shard].lock();
        }
        can_be_updated = true;
        controller.OnSwitch(thread_num, GetConf(is_first_local)[thread_num]);
    }

    //TODO fix the book with variable name
    //TODO fix book with startconfiguration and finishconfiguration calls
    void ThreadAction(int thread_num) noexcept {
        exception top keeper.SetPath("file " + std::to_string(thread_num));
        StartConfiguration(thread_num);
        while (true) {
            exception_top_keeper.WithCatchingException([thread_num] {
                Vector<int> shards = GetShards(thread_num);
                controller.PreProcess(thread_num, can_be_updated);
                controller.ProcessShard(shard_num, can_be_updated);
                if (reshard_waiting_timer.CheckTime()) {
                    NoUpdatePromise();
                    if (thread_num == 0) {
                        Reshard();
                    }
                }
            });
        }
    }

    void Run() noexcept {
        for (int i = 1; i < threads_count; ++i) {
            threads.push_back(std::thread(ThreadAction, this, i));
        }
        ThreadAction(0);
        for (auto& one_thread : threads) {
            one_thread.join();
        }
    }
private:
    Controller& controller;
    int threads_count;
    ReshardingConf first_conf;
    ReshardingConf second_conf;
    std::atomic<ShadowCounter> shadow_counter;
    Vector<std::thread> threads;
    Vector<std::mutex> shard_mutexes;
    static thread_local bool can_be_updated;
    static thread_local bool is_first_local;
    static thread_local WaitingTimer reshard_waiting_timer;
};

//TODO update signatures in the book
template <typename ... Args>
class MessagePassingController {
public:
    MessagePassingController();
    void ProcessShard(int shard_num, bool can_be_updated);
    void PreProcess(int thread_num, bool can_be_updated);
    void OnSwitch(int thread_num, const Vector<int>& new_shards) noexcept;
    ReshardingConf GetInitialSharding(int threads_count);
    ReshardingConf GetResharding(const ReshardingConf& old_conf, int threads_count);
    const int GetMaxThreadCount();
private:
    MessagePassingTree<Args...> message_passing_tree;
    Vector<std::condition_variable_any> message_wait_cvs;
    Vector<std::condition_variable_any *> message_send_cvs;
    static thread_local bool are_queues_empty;
    static const uint64_t wait_for_message_time;
    Vector<StartFinishTimer> message_processor_timers;
    Vector<StartFinishTimer> edge_timers;
};

// TODO: fix constructor name
// TODO  sharder is singleton, not a variable
template <typename ... Args>
class DynamicallyShardedMessagePassingPool {
public:
    DynamicallyShardedMessagePassingPool();
public:
    static DynamicallyShardedMessagePassingPool& GetInstance();
    static void Run() noexcept {
    }
private:
    MessagePassingController<Args...> controller;
};
