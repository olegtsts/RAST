#include "exception_with_backtrace.h"
#include <iostream>

void Func2() {
    throw ExceptionWithBacktrace("Reason of exception");
}

void Func() {
    std::cout << "Calling Func2\n";
    Func2();
    //
    //
    std::cout << "Output something\n";
}

void TestException() {
    try {
        Func();
    } catch (const ExceptionWithBacktrace& e) {
        std::cout << " == what == \n";
        std::cout << e.what() << std::endl;
        std::cout << " == GetStackStrace == \n";
        std::cout << e.GetStackStrace() << std::endl;
        std::cout << " == GetReasonWithBacktrace == \n";
        std::cout << e.GetReasonWithBacktrace() << std::endl;
    }
}


int main() {
    TestException();
    return 0;
}
