#include <iostream>
#include <vector>
#include <future>
#include <functional>
#include <thread>

#include "queue.h"

void thread_push(const int number, LockFreeQueue<int>& queue, std::shared_future<void> wait_to_go) noexcept {
    wait_to_go.wait();
    for (size_t i = 0; i < 100000; ++i) {
        queue.Push(number);
    }
}
void thread_pop(LockFreeQueue<int>& queue, std::shared_future<void> wait_to_go) noexcept {
    wait_to_go.wait();
    for (size_t i = 0; i < 100000; ++i) {
        auto ptr = queue.Pop();
    }
}
int main() {
    std::cout << "Started\n";
    {
        LockFreeQueue<int> queue;
        std::promise<void> go;
        std::shared_future<void> wait_to_go = go.get_future();
        std::vector<std::thread> threads;
        for (size_t i = 0; i < 2; ++i) {
            threads.push_back(std::thread(thread_push, i + 100, std::ref(queue), wait_to_go));
        }
        for (size_t i = 0; i < 2; ++i) {
            threads.push_back(std::thread(thread_pop, std::ref(queue), wait_to_go));
        }
        go.set_value();
        for (auto& one_thread: threads) {
            one_thread.join();
        }
    };
    std::cout << "Done\n";
    return 0;
}
