#include <string>
#include <cstdlib>

#include "exception_with_backtrace.h"

std::string ExceptionWithBacktrace::GetString(const char * ptr) noexcept {
    if (ptr != nullptr) {
        return std::string(ptr);
    } else {
        return std::string();
    }
}

void ExceptionWithBacktrace::ErrorCallback(void* /*data*/, const char* /*msg*/, int /*errnum*/) noexcept {
}

int ExceptionWithBacktrace::FillCallback(void *data, uintptr_t /*pc*/, const char *filename, int lineno, const char *function) noexcept {
    pc_data& d = *static_cast<pc_data*>(data);
    d.filename = GetString(filename);
    d.function = GetString(function);
    d.line = lineno;
    return 0;
}

_Unwind_Reason_Code ExceptionWithBacktrace::TraceFunc(_Unwind_Context* context, void* void_state) {
    StackCrowlState *state = (StackCrowlState *)void_state;
    auto ip = _Unwind_GetIP(context);
    if (state->total_count++ >= state->skip && ip) {
        ::backtrace_state* bt_state = ::backtrace_create_state(0, 0, ErrorCallback, 0);
        pc_data data;
        ::backtrace_pcinfo(bt_state, reinterpret_cast<uintptr_t>(ip - 5), FillCallback, ErrorCallback, &data);
        if (data.filename.size() > 0) {
            int status = 0;
            std::size_t size = 0;

            std::string demangled_function = GetString(abi::__cxa_demangle( data.function.c_str(), NULL, &size, &status ));
            if (demangled_function.size() == 0) {
                demangled_function = data.function;
            }
            state->ss << state->output_count++ << ": " << demangled_function << " at " << data.filename << ":" << data.line << std::endl;
        } else {
            ::Dl_info dl_info;
            ::dladdr((void *)ip, &dl_info);
            state->ss << state->output_count++ << ": " << (void *)ip << " in " << dl_info.dli_fname << std::endl;
        }
    }
    return(_URC_NO_REASON);
}

ExceptionWithBacktrace::ExceptionWithBacktrace(const std::string& reason) noexcept
    : reason(reason),
      backtrace(),
      reason_with_backtrace()
{
    try {
        StackCrowlState state{1};
        _Unwind_Backtrace(TraceFunc, &state);
        backtrace = state.ss.str();
    } catch (...) {
        backtrace = "Not available";
    }
    reason_with_backtrace = reason + "\n" + backtrace;
}

const std::string& ExceptionWithBacktrace::GetStackStrace() const noexcept {
    return backtrace;
}

const char* ExceptionWithBacktrace::what() const noexcept {
    return reason_with_backtrace.c_str();
}

const std::string& ExceptionWithBacktrace::GetReasonWithBacktrace() const noexcept {
    return reason_with_backtrace;
}

