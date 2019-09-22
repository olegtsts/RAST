#include "argparser.h"
#include <iostream>
#include <string>

struct FirstArg {
    std::string name = "arg";
    std::string description = "some description";
    using type = int;
};

void FirstFunc() {
    int value = ArgParser::GetValue<FirstArg>();
    std::cout << "value = " << value << std::endl;
}

struct SecondArg {
    std::string name = "arg2";
    std::string description = "some description 2";
    using type = int;
    int default_value = 123;
};

void SecondFunc() {
    int value = ArgParser::GetValue<SecondArg>();
    std::cout << "value2 = " << value << std::endl;
}

struct ThirdArg {
    std::string name = "arg3";
    std::string description = "some description 3";
    using type = bool;
    int default_value = false;
};

void ThirdFunc() {
    int value = ArgParser::GetValue<ThirdArg>();
    std::cout << "value3 = " << value << std::endl;
}

int main(int argc, char** argv) {
    ArgParser::SetArgV(argc, argv);
    SecondFunc();
    ThirdFunc();
    return 0;
}
