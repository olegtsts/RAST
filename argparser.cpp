#include <string>
#include <sstream>
#include <iostream>
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

bool ArgParser::IsStringFlag(const std::string& input_string) {
    bool first_symbol_after_dash_is_letter = false;
    for (auto ch: input_string) {
        if (ch != '-') {
            first_symbol_after_dash_is_letter = ch >= 'a' && ch <= 'z';
            break;
        }
    }
    return input_string.size() > 0 && input_string[0] == '-' && first_symbol_after_dash_is_letter;
}

bool ArgParser::SetArgV(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        std::string arg_string(argv[i]);
        if (!IsStringFlag(arg_string)) {
            std::stringstream ss;
            ss << "Token '" << arg_string << "' does not look like flag name. Flag name should start with '-'.";
            throw ExceptionWithBacktrace(ss.str());
        }
        std::string flag, value;
        size_t separator_index = arg_string.find('=');
        if (separator_index == std::string::npos) {
            flag = arg_string;
            if (i + 1 != argc && !IsStringFlag(argv[i + 1])) {
                value = argv[++i];
            }
        } else {
            flag = arg_string.substr(1, separator_index - 1);
            value = arg_string.substr(separator_index + 1, arg_string.size() - separator_index - 1);
        }
        auto last_it_after_removal = std::remove_if(flag.begin(), flag.end(), [](char ch) {return ch == '-';});
        flag.resize(last_it_after_removal - flag.begin());
        if (flag == "help") {
            std::cout << GetInstance().GetHelpString();
            return false;
        }
        auto it = GetInstance().arg_processors.find(flag);
        if (it == GetInstance().arg_processors.end()) {
            std::stringstream ss;
            ss << "Encountered unregistered arg '" << flag << "'. Use --help to list registered args.";
            throw ExceptionWithBacktrace(ss.str());
        }
        ArgProcessorBase* arg_processor = it->second.get();
        if (value.empty() && arg_processor->IsBool()) {
            arg_processor->Parse("true");
        } else {
            arg_processor->Parse(value);
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
    return true;
}

