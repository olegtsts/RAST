#include <cxxabi.h>
#include <unwind.h>
#include <backtrace.h>
#include <dlfcn.h>
#include <string>
#include <iostream>
#include <cstdlib>
#include <sstream>

struct StackCrowlState {
    unsigned int skip;
    unsigned int total_count = 0;
    unsigned int output_count = 0;
    std::stringstream ss = std::stringstream();
};

struct pc_data {
    std::string function;
    std::string filename;
    std::size_t line;
};

class ExceptionWithBacktrace : public std::exception {
private:
    static std::string GetString(const char * ptr) noexcept;
    static int FillCallback(void *data, uintptr_t /*pc*/, const char *filename, int lineno, const char *function) noexcept;
    static void ErrorCallback(void* /*data*/, const char* /*msg*/, int /*errnum*/) noexcept;
    static _Unwind_Reason_Code TraceFunc(_Unwind_Context* context, void* void_state);
public:
    ExceptionWithBacktrace(const std::string& reason) noexcept;
    const std::string& GetStackStrace() const noexcept;
    const char* what() const noexcept;
    const std::string& GetReasonWithBacktrace() const noexcept;
private:
    std::string reason;
    std::string backtrace;
    std::string reason_with_backtrace;
};

