#pragma once

#include <string>
#include <cassert>
#include "ranked_map.h"
#include "timers.h"

class ExceptionTopKeeper {
private:
    void Dump();
    void Restore();
    void Trim() noexcept;
    void RemoveOld(const int64_t current_time) noexcept;
public:
    ExceptionTopKeeper();

    void SetPath(const std::string& new_path) noexcept;

    template <typename Function>
    void WithCatchingException(Function func) noexcept {
        if (debug_mode) {
            func();
            return;
        }
        assert(!path.empty());
        try {
            func();
        } catch (const std::exception& e) {
            std::string exception_text(e.what());
            auto it = exception_count.Find(exception_text);
            auto current_time = store_clock.GetTime();
            if (it == exception_count.end()) {
                exception_count.Update(exception_text, 1);
            } else {
                exception_count.Update(exception_text, it->second + 1);
            }
            exception_last_time.Update(exception_text, current_time);
            RemoveOld(current_time);
            Trim();
            Dump();
        }
    }
private:
    std::string path;
    RankedMap<std::string, uint64_t> exception_count;
    RankedMap<std::string, int64_t> exception_last_time;
    WaitingTimer dump_timer;
    PeriodicClock store_clock;
    static const size_t top_size;
    static const int64_t keep_time;
    static const bool debug_mode;
};

extern thread_local ExceptionTopKeeper exception_top_keeper;
