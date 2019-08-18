#include "exception_top_proto_storage.h"
#include "exception_top_proto_storage.pb.h"
#include "exception_with_backtrace.h"
#include <iostream>
#include <fstream>

void SimpleTest() {
    exception_top_keeper.SetPath("file");
    for (int i = 0; i < 10; ++i) {
        exception_top_keeper.WithCatchingException([] () {
            throw ExceptionWithBacktrace("test exception");
        });
    }
    for (int i = 0; i < 20; ++i) {
        exception_top_keeper.WithCatchingException([] () {
            throw ExceptionWithBacktrace("second exception");
        });
    }
    std::ifstream in("file");
    ExceptionTop exception_top;
    exception_top.ParseFromIstream(&in);
    std::cout << exception_top.DebugString() << std::endl;
}


int main() {
    SimpleTest();
    return 0;
}
