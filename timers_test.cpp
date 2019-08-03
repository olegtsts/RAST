#include "timers.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std::chrono_literals;

void TestPeriodicTimer() {
    PeriodicTimer periodic_timer;
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 10; ++i) {
            std::cout << "PeriodicTimer::GetPassedTime = " << periodic_timer.GetPassedTime() << std::endl;
            std::this_thread::sleep_for(2ms);
        }
        periodic_timer.Reset();
    }
}

void TestStartFinishTimer() {
    StartFinishTimer start_finish_timer;
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 10; ++i) {
            start_finish_timer.Start();
            std::this_thread::sleep_for(2ms);
            start_finish_timer.Finish();
            std::cout << "StartFinishTimer::GetCount = " << start_finish_timer.GetCount() << std::endl;
            std::cout << "StartFinishTimer::GetDurationSum = " << start_finish_timer.GetDurationSum() << std::endl;
        }
        start_finish_timer.Reset();
    }
}

void TestWaitingTimer() {
    WaitingTimer waiting_timer(5000);
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 10; ++i) {
            std::this_thread::sleep_for(2ms);
            std::cout << "WaitingTimer::CheckTime = " << waiting_timer.CheckTime() << std::endl;
        }
        waiting_timer.Reset();
    }
}

void TestPeriodicClock() {
    PeriodicClock period_clock;
    int64_t first_time = 0;
    int64_t last_time;
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(2ms);
            last_time = period_clock.GetTime();
            std::cout << "PeriodicClock::GetTime = " << last_time << std::endl;
            if (first_time == 0) {
                first_time = last_time;
            }
        }
    }
    std::cout << "Total time lapse = " << last_time - first_time << std::endl;
}

void TestPeriodicClockFast() {
    PeriodicClock period_clock;
    int64_t first_time = 0;
    int64_t last_time;
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(0.1ms);
            last_time = period_clock.GetTime();
            std::cout << "PeriodicClock::GetTime = " << last_time << std::endl;
            if (first_time == 0) {
                first_time = last_time;
            }
        }
    }
    std::cout << "Total time lapse = " << last_time - first_time << std::endl;
}

int main() {
    TestPeriodicTimer();
    TestStartFinishTimer();
    TestWaitingTimer();
    TestPeriodicClock();
    TestPeriodicClockFast();
    return 0;
}

