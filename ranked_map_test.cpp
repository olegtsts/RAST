#include "ranked_map.h"
#include <iostream>

template <typename Map>
void OutputContents(const Map& ranked_map) {
    std::cout << "map contents: ";
    for (auto& pair : ranked_map) {
        std::cout << "(" << pair.first << " " << pair.second << ") ";
    }
    std::cout << std::endl;
    auto lowest_it = ranked_map.GetLowest();
    if (lowest_it != ranked_map.end()) {
        std::cout << "Lowest = (" << lowest_it->first << "," << lowest_it->second << ")" << std::endl;
    } else {
        std::cout << "Container is empty\n";
    }
}

void SimpleTest() {
    RankedMap<int, double> ranked_map;
    for (int i = 0; i < 10; ++i) {
        ranked_map.Update(i, static_cast<double>(10 - i));
        OutputContents(ranked_map);
    }
    for (int i = 0; i < 10; ++i) {
        ranked_map.Update(i, static_cast<double>(5 - i));
        OutputContents(ranked_map);
    }
    for (int i = 0; i < 10; ++i) {
        ranked_map.Erase(i);
        OutputContents(ranked_map);
    }
}

int main() {
    SimpleTest();
    return 0;
}
