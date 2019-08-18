#include "exception_top_proto_storage.h"
#include "exception_top_proto_storage.pb.h"
#include "exception_with_backtrace.h"
#include <string>
#include <utility>
#include <fstream>
#include <cassert>
#include <filesystem>

thread_local ExceptionTopKeeper exception_top_keeper;

void ExceptionTopKeeper::Dump() {
    if (!dump_timer.CheckTime()) {
        return;
    }
    ExceptionTop exception_top;
    for (const std::pair<std::string, uint64_t>& exception_and_count: exception_count) {
        (*exception_top.mutable_exception_count())[exception_and_count.first] = exception_and_count.second;
    }
    for (const std::pair<std::string, int64_t>& exception_and_time: exception_last_time) {
        (*exception_top.mutable_exception_last_time())[exception_and_time.first] = exception_and_time.second;
    }
    std::filesystem::path tmp_path = std::filesystem::temp_directory_path();
    {
        std::ofstream out(tmp_path / "data");
        assert(exception_top.SerializeToOstream(&out));
    }
    std::filesystem::rename(tmp_path / "data", path);
}

void ExceptionTopKeeper::Restore() {
    ExceptionTop exception_top;
    std::ifstream in(path);
    if (exception_top.ParseFromIstream(&in)) {
        for (const auto & exception_and_count : exception_top.exception_count()) {
            exception_count.Update(exception_and_count.first, exception_and_count.second);
        }
        for (const auto & exception_and_time : exception_top.exception_last_time()) {
            exception_last_time.Update(exception_and_time.first, exception_and_time.second);
        }
    }
}

void ExceptionTopKeeper::Trim() noexcept {
    while (exception_count.GetSize() > top_size) {
        std::string rare_exception = exception_count.GetLowest()->first;
        exception_count.Erase(rare_exception);
        exception_last_time.Erase(rare_exception);
    }
}

void ExceptionTopKeeper::RemoveOld(const int64_t current_time) noexcept {
    if (exception_last_time.GetSize() == 0) {
        return;
    }
    while (true) {
        auto it = exception_last_time.GetLowest();
        if (it->second > current_time - keep_time) {
            return;
        }
        exception_count.Erase(it->first);
        exception_last_time.Erase(it->first);
    }
}

ExceptionTopKeeper::ExceptionTopKeeper()
    : path(),
      exception_count(),
      exception_last_time(),
      dump_timer(1000), // 1ms
      store_clock()
{
}

void ExceptionTopKeeper::SetPath(const std::string& new_path) noexcept {
    path = new_path;
    {
        std::ofstream out(path, std::ios_base::app);
    }
    Restore();
}

const size_t ExceptionTopKeeper::top_size(1000);
const int64_t ExceptionTopKeeper::keep_time(600 * 1000 * 1000); // 10 mins
const bool ExceptionTopKeeper::debug_mode(false);
