#include <string>
#include <sstream>
#include "argparser.h"
#include <iostream>
#include <algorithm>

void ArgParser::ParseArgValue(const std::string& arg_value, int* value) {
    std::stringstream ss(arg_value);
    ss >> *value;
}

std::string ArgParser::GetHelpString() {
    std::stringstream ss;
    for (const auto& arg_and_processor : arg_processors) {
        ss << arg_and_processor.first << " : " << arg_and_processor.second->GetDescription() << std::endl;
    }
    return ss.str();
}

// TODO: write proper implementation
void ArgParser::SetArgV(int argc, char **argv) {
    std::cout << GetInstance().GetHelpString();
    if (GetInstance().arg_processors.count("arg") > 0) {
        GetInstance().arg_processors["arg"]->Parse("123");
    }
}

