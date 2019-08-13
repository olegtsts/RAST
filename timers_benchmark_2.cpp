#include "timers.h"

int main() {
    PeriodicTimer timer;
    for (int i = 0; i < 1000000; ++i) {
        auto passed_time = timer.GetPassedTime();
        ++passed_time;
    }
    return 0;
}
