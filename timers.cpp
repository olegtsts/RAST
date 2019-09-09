#include <chrono>
#include <algorithm>

#include "timers.h"

const int64_t PeriodicTimer::min_system_click_call_period = 1000; // 1 ms
const int64_t PeriodicTimer::max_wind_up_steps = 10000;
const int64_t PeriodicTimer::min_measured_time = 1; // 1 ns

PeriodicTimer::PeriodicTimer() noexcept
    : wind_up_counter(1),
      wind_up_balance(0),
      last_wind_up_counter(1),
      last_system_time(0)
{
}

void PeriodicTimer::WindUp() noexcept {
    int64_t system_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    int64_t time_passed = system_time - last_system_time;
    int64_t new_wind_up_counter = time_passed > 0 ? std::max(1l, std::min(
            min_system_click_call_period / time_passed * last_wind_up_counter,
            max_wind_up_steps)) : max_wind_up_steps;

    wind_up_balance += time_passed;
    approx_time_step = (time_passed * new_wind_up_counter / last_wind_up_counter + wind_up_balance) / (new_wind_up_counter + 1);
    last_wind_up_counter = new_wind_up_counter;
    wind_up_counter = last_wind_up_counter;
    last_system_time = system_time;
}

int64_t PeriodicTimer::GetPassedTime() noexcept {
    if (--wind_up_counter == 0) {
        WindUp();
    }
    int64_t result_time = std::max(min_measured_time, approx_time_step);
    wind_up_balance -= result_time;
    return result_time;
}

void PeriodicTimer::Reset() noexcept {
    wind_up_counter = 1;
    wind_up_balance = 0;
    last_wind_up_counter = 1;
    last_system_time = 0;
}

StartFinishTimer::StartFinishTimer() noexcept
    : start_timer(),
      finish_timer(),
      measurements_counter(0),
      last_duration(0),
      duration_sum(0)
{
}

void StartFinishTimer::Start() noexcept {
    last_duration -= start_timer.GetPassedTime();
}

void StartFinishTimer::Finish() noexcept {
    last_duration += finish_timer.GetPassedTime();
    duration_sum += last_duration;
    ++measurements_counter;
}

uint64_t StartFinishTimer::GetCount() noexcept {
    return measurements_counter;
}

int64_t StartFinishTimer::GetDurationSum() noexcept {
    return duration_sum;
}

int64_t StartFinishTimer::GetAverageDuration() noexcept {
    if (measurements_counter > 0) {
        return std::max(static_cast<int64_t>(duration_sum / measurements_counter), PeriodicTimer::min_measured_time);
    } else {
        return PeriodicTimer::min_measured_time;
    }
}

void StartFinishTimer::Reset() noexcept {
    start_timer.Reset();
    finish_timer.Reset();
    measurements_counter = 0;
    last_duration = 0;
    duration_sum = 0;
}

WaitingTimer::WaitingTimer(const uint64_t time_period) noexcept
    : time_period(time_period),
      time_elapsed(0),
      periodic_timer()
{
    Reset();
}

bool WaitingTimer::CheckTime() noexcept {
    time_elapsed += periodic_timer.GetPassedTime();
    return time_elapsed > static_cast<int64_t>(time_period);
}

void WaitingTimer::Reset() noexcept {
    periodic_timer.Reset();
    periodic_timer.GetPassedTime();
    time_elapsed = 0;
}

PeriodicClock::PeriodicClock() noexcept
    : periodic_timer(),
      current_time(0)
{
}

int64_t PeriodicClock::GetTime() noexcept {
    current_time += periodic_timer.GetPassedTime();
    return current_time;
}

