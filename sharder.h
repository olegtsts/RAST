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
public:
    Sharder(Controller& controller)
        : controller(controller),
          threads_count(std::min(std::thread::hardware_concurrency, controller.GetMaxThreadsCount())),
          first_conf(controller.GetInitialSharding(threads_count))  ,
          second_conf(),
          shadow_counter({true, 0}),
          threads(),
          shard_mutexes(controller.GetShardsCount()),
          can_be_updated(threads_count, true),
          is_first_local(threads_count, false),
          reshard_waiting_timer(threads_count, {time_between_reshards})
    {
        assert(threads_count == first_conf.size());
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

    void NoUpdatePromise(int thread_num) noexcept {
        if (can_be_updated[thread_num]) {
            ShadowCounter old_counter = shadow_counter.load(std::memory_order_relaxed);
            do {
                ShadowCounter new_counter = old_counter;
                ++new_counter.noupdate_counter;
            } while (!shadow_counter.compare_exchange_weak(old_counter, new_counter,
                        std::memory_order_acquire, std::memory_order_relaxed));
            can_be_updated[thread_num] = false;
        }
    }

    //TODO fix book definition of function (reference)
    Vector<int> GetShards(int thread_num) noexcept {
        ShadowCounter current_counter = shadow_counter.load(std::memory_order_acquire);
        if (current_counter.is_first_main != is_first_local) {
            SwitchConfiguration(thread_num);
        }
        return GetConf(is_first_local[thread_num])[thead_num];
    }

    void SwitchConfiguration(int thread_num) noexcept {
        FinishConfiguration(thread_num);
        is_first_local[thread_num] = !is_first_local[thread_num];
        StartConfiguration(thread_num);
    }

    void FinishConfiguration(int thread_num) noexcept {
        for (int shard : GetConf(!is_first_main)[thread_num]) {
            shard_mutexes[shard].unlock();
        }
    }

    void StartConfiguration(int thread_num) noexcept {
        reshard_waiting_timer[thread_num].Reset();
        for (int shard : GetConf(!is_first_main)[thread_num]) {
            shard_mutexes[shard].lock();
        }
        can_be_updated[thread_num] = true;
        controller.OnSwitch(thread_num, GetConf(is_first_local[thread_num])[thread_num]);
    }

    //TODO fix the book with variable name
    //TODO fix book with startconfiguration and finishconfiguration calls
    void ThreadAction(int thread_num) noexcept {
        exception top keeper.SetPath("file " + std::to_string(thread_num));
        StartConfiguration(thread_num);
        while (true) {
            exception_top_keeper.WithCatchingException([thread_num] {
                Vector<int> shards = GetShards(thread_num);
                controller.PreProcess(thread_num, can_be_updated[thread_num]);
                controller.ProcessShard(shard_num, thread_num, can_be_updated[thread_num]);
                if (reshard_waiting_timer[thread_num].CheckTime()) {
                    NoUpdatePromise(thread_num);
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
    Vector<bool> can_be_updated;
    Vector<bool> is_first_local;
    Vector<bool> reshard_waiting_timer;
    static const uint64_t time_between_reshards;
};

template <typename Controller>
const uint64_t Sharder<Controller>::time_between_reshards = 1e6; // 1s

//TODO update signatures in the book
template <typename ... Args>
class MessagePassingController {
public:
    MessagePassingController()
        : message_passing_tree(),
          message_wait_cvs(),
          message_send_cvs(),
          are_queues_empty(),
          message_processor_timers(),
          edge_timers()
    {}

    void PreProcess(int thread_num, bool can_be_updated) {
        if (are_queues_empty[thread_num]) {
            // TODO plug in dummy lock class
            message_wait_cvs[thread_num].wait_for(..., std::chrono::microseconds(wait_for_message_time));
        }
        are_queues_empty[thread_num] = true;
    }

    void ProcessShard(int shard_num, int thread_num, bool can_be_updated) {
        if (can_be_updated) {
            message_processor_timers[shard_num].Start();
        }
        message_passing_tree.GetMessageProcessorProxy(shard_num)->Ping();
        if (can_be_updated) {
            message_processor_timers[shard_num].Finish();
        }
        for (int edge : message_passing_tree.GetIncomingEdges(shard_num)) {
            if (can_be_updated) {
                edge_timers[edge].Start();
            }
            if (message_passing_tree.GetEdgeProxy(edge)->NotifyAboutMessage()) {
                are_queues_empty[thread_num] = false;
            }
            if (can_be_updated) {
                edge_timers[edge].Finish();
            }
        }
    }

    void SetSenderCVS(const ReshardingConf& conf) {
        std::vector<int> shard_thread;
        for (int thread_num= 0; thread_num < static_cast<int>(conf.size()); ++thread_num) {
            for (auto shard_num : conf[thread_num]) {
                while (shard_thread.size() <= shard_num) {
                    shard_thread.push_back(0);
                }
                shard_thread[shard_num] = thread_num;
            }
        }
        for (int queue_index = 0; queue_index < static_cast<int>(message_passing_tree.GetEdgesCount()); ++queue_index) {
            int receiver_index = message_passing_tree.GetEdgeProxy(queue_index)->GetToIndex();
            int receiver_thread = shard_thread[receiver_index];
            message_send_cvs[queue_index] = &message_wait_cvs[receiver_thread];
        }
    }

    ReshardingConf GetInitialSharding(int threads_count) {
        message_wait_cvs.resize(threads_count);
        message_send_cvs.resize(message_passing_tree.GetEdgesCount());
        are_queues_empty.assign(threads_count, false);
        message_processor_timers.assign(message_passing_tree.GetMessageProcessorsCount(), {});
        edge_times.assign(message_passing_tree.GetEdgesCount(), {});
        ReshardingConf conf(threads_count, {});
        for (auto shard_num : message_passing_tree.GetMessageProcessorsCount()) {
            conf[shard_num % threads_count].push_back(shard_num);
        }
        SetSenderCVS(conf);
        return conf;
    }

    void OnSwitch(int thread_num, const Vector<int>& new_shards) noexcept {
        for (int new_shard : new_shards) {
            for (int outgoing_edge : message_passing_tree.GetOutgoingEdges(new_shard)) {
                message_passing_tree.GetEdgeProxy(outgoing_edge)->SetConditionVariable(&message_send_cvs[outgoing_edge]);
            }
            message_processor_timers[new_shard].Reset();
            for (int edge : message_passing_tree.GetIncomingEdges(shard_num)) {
                edge_timers[edge].Reset();
            }
        }
    }

    void ProcessMPChoice(int64_t duration, int mp, int64_t* accumulated_duration,
            Vector<int>* current_thread_mps, Set<std::pair<int64_t, int>>* available_duration_mp,
            Vector<int64_t>* message_passing_cost) {
        *accumulated_duration += duration;
        current_thread_mps->push_back(mp);
        available_duration_mp->erase(std::make_pair(duraction, mp));
        for (auto duration_mp : available_duration_mp) {
            for (int edge : message_passing_tree.GetConnectingEdges(mp, duration_mp.second)) {
                message_passing_cost->operator[](duration_mp.second) += edge_timers[edge].GetDurationSum();
            }
        }
    }

    ReshardingConf GetResharding(const ReshardingConf& old_conf, int threads_count) {
        int64_t avg_duration = 0;
        for (int i = 0; i < message_passing_tree.GetMessageProcessorsCount(); ++i) {
            avg_duration += message_processor_timers[i].GetDurationSum();
        }
        for (int i = 0; i < message_passing_tree.GetEdgesCount(); ++i) {
            avg_duration += edge_timers[i].GetDurationSum();
        }
        avg_duration /= threads_count;
        avg_duration = std::max(avg_duration, 1);
        Set<std::pair<int64_t, int>> available_duration_mp;
        for (int i = 0; i < message_passing_tree.GetMessageProcessorsCount(); ++i) {
            int64_t duration = message_processor_timers[i].GetDurationSum();
            for (int j : message_passing_tree.GetIncomingEdges(i)) {
                duration += edge_timers[j].GetDurationSum();
            }
            available_duration_mp.insert(std::make_pair(duration, i));
        }
        ReshardingConf conf;
        for (int thread_num = 0; thread_num < threads_count; ++thread_num) {
            int64_t accumulated_duration = 0;
            Vector<int> current_thread_mps;
            Vector<int64_t> message_passing_cost(message_passing_tree.GetMessageProcessorsCount(), 0);
            assert(available_duration_mp.size() > 0);
            auto it = available_duration_mp.rbegin()
            if (it->first > avg_duration) {
                ProcessMPChoice(it->first, it->second, &accumulated_duration,
                        &current_thread_mps, &available_duration_mp, &message_passing_cost);
            } else {
                while (available_duration_mp.size() > 0) {
                    auto chosen_it = available_duration_mp.end();
                    double largest_cost = -1;
                    for (auto it = available_duration_mp.begin(); it != available_duration_mp.upper_bound(std::make_pair(avg_duration, 0)); ++it) {
                        if (largest_cost < message_passing_cost[it->second]) {
                            largest_cost = message_passing_cost[it->second];
                            chosen_it = it;
                        }
                    }
                    if (chosen_it == available_duration_mp.end()) {
                        break;
                    } else {
                        ProcessMPChoice(chosen_it->first, chosen_it->second, &accumulated_duration,
                            &current_thread_mps, &available_duration_mp, &message_passing_cost);
                    }
                }
            }
            assert(current_thread_mps.size() > 0);
            conf.push_back(current_thread_mps);
        }
        SetSenderCVS(conf);
        return conf;
    }

    size_t GetMaxThreadsCount() const noexcept {
        return message_passing_tree.GetMessageProcessorsCount();
    }
private:
    MessagePassingTree<Args...> message_passing_tree;
    Vector<std::condition_variable_any> message_wait_cvs;
    Vector<std::condition_variable_any *> message_send_cvs;
    Vector<bool> are_queues_empty;
    Vector<StartFinishTimer> message_processor_timers;
    Vector<StartFinishTimer> edge_timers;
    static const uint64_t wait_for_message_time;
};

template <typename ... Args>
const uint64_t MessagePassingController<Args...>::wait_for_message_time = 1e3; // 1ms

// TODO: fix constructor name
// TODO  sharder is singleton, not a variable
template <typename ... Args>
class DynamicallyShardedMessagePassingPool {
public:
public:
    DynamicallyShardedMessagePassingPool();
    static void Run() noexcept {
    }
private:
    Sharder<MessagePassingController<Args...>> sharder;
};
