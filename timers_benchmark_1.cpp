#include <algorithm>
#include <chrono>

int main() {
    for (int i = 0; i < 1000000; ++i) {
        int64_t system_time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        ++system_time;
    }
    return 0;
}
