#pragma once

#include <cstdint>

class PeriodicTimer {
private:
    void WindUp() noexcept;
public:
    PeriodicTimer() noexcept;
    int64_t GetPassedTime() noexcept;
    void Reset() noexcept;
private:
    static const int64_t min_system_click_call_period;
    static const int64_t max_wind_up_steps;
    static const int64_t min_measured_time;
    int64_t wind_up_counter;
    int64_t wind_up_balance;
    int64_t last_wind_up_counter;
    int64_t last_system_time;
    int64_t approx_time_step;
};

class StartFinishTimer {
public:
    StartFinishTimer() noexcept;
    void Start() noexcept;
    void Finish() noexcept;
    uint64_t GetCount() noexcept;
    int64_t GetDurationSum() noexcept;
    void Reset() noexcept;
private:
    PeriodicTimer start_timer;
    PeriodicTimer finish_timer;
    uint64_t measurements_counter;
    int64_t last_duration;
    int64_t duration_sum;
};

class WaitingTimer {
public:
    explicit WaitingTimer(const uint64_t time_period) noexcept;
    bool CheckTime() noexcept;
    void Reset() noexcept;
private:
    uint64_t time_period;
    int64_t time_elapsed;
    PeriodicTimer periodic_timer;
};

class PeriodicClock {
public:
    PeriodicClock() noexcept;
    int64_t GetTime() noexcept;
private:
    PeriodicTimer periodic_timer;
    int64_t current_time;
};

