#include <string>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

#include "argparser.h"


bool ArgParser::ParseArgValue(const std::string& arg_value, int* value) {
    std::stringstream ss(arg_value);
    ss >> *value;
    return ss.eof();
}

bool ArgParser::ParseArgValue(const std::string& arg_value, bool* value) {
    if (arg_value == "true" || arg_value == "1") {
        *value = true;
        return true;
    } else if (arg_value == "false" || arg_value == "0") {
        *value = false;
        return true;
    }
    return false;
}

std::string ArgParser::GetHelpString() const {
    std::stringstream ss;
    for (const auto& arg_and_processor : arg_processors) {
        ss << arg_and_processor.first << " : " << arg_and_processor.second->GetDescription() << std::endl;
    }
    return ss.str();
}

void ArgParser::SetArgV(int argc, char **argv) {
    std::string joined_string;
    for (int i = 1; i < argc; ++i) {
        joined_string += argv[i];
        joined_string += ' ';
    }
    for (char symbol : {'-', '='}) {
        std::replace(joined_string.begin(), joined_string.end(), symbol, ' ');
    }
    std::stringstream ss(joined_string);
    std::vector<std::string> tokens;
    while (!ss.eof()) {
        std::string token;
        ss >> token;
        tokens.push_back(token);
    }
    size_t index = 0;
    std::string arg_name;
    while (index < tokens.size()) {
        if (arg_name.empty()) {
            arg_name = tokens[index];
            ++index;
        } else {
            if (arg_name == "help") {
                std::cout << GetInstance().GetHelpString();
                return;
            }
            auto it = GetInstance().arg_processors.find(arg_name);
            if (it == GetInstance().arg_processors.end()) {
                std::stringstream ss;
                ss << "Encountered unregistered arg '" << arg_name << "'. Use --help to list registered args.";
                throw ExceptionWithBacktrace(ss.str());
            }
            ArgProcessorBase* arg_processor = it->second.get();
            if (arg_processor->IsBool() && tokens[index] != "0" && tokens[index] != "1" &&
                tokens[index] != "true" && tokens[index] != "false") {
                arg_processor->Parse("true");
                arg_name = tokens[index];
                continue;
            }
            arg_processor->Parse(tokens[index]);
            arg_name.clear();
            ++index;
        }
    }
    for (const auto& arg_and_processor : GetInstance().arg_processors) {
        ArgProcessorBase* arg_processor = arg_and_processor.second.get();
        if (!arg_processor->IsInitialized() && !arg_processor->HasDefaultValue()) {
            std::stringstream ss;
            ss << "Flag '" << arg_and_processor.first << "' is not initialized and does not have default value";
            throw ExceptionWithBacktrace(ss.str());
        }
    }
}

