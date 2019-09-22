#include "argparser.h"
#include <iostream>
#include <string>

struct SomeArg {
    std::string name = "arg";
    std::string description = "some description";
    using type = int;
};

void Func() {
    int value = ArgParser::GetValue<SomeArg>();
    std::cout << "value = " << value << std::endl;
}

int main(int argc, char** argv) {
    ArgParser::SetArgV(argc, argv);
    return 0;
}
