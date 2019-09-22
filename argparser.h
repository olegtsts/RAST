#pragma once

#include <string>
#include <typeinfo>
#include <sstream>
#include <memory>
#include <cassert>

#include "auto_registrar.h"
#include "types.h"
#include "exception_with_backtrace.h"

class ArgParser {
private:
    static void ParseArgValue(const std::string& arg_value, int* value);

    ArgParser() {
    }

    static ArgParser& GetInstance() {
        static ArgParser arg_parser;
        return arg_parser;
    }

    class ArgProcessorBase {
    public:
        virtual void Parse(const std::string& raw_value) = 0;
        virtual std::string GetEntityName() = 0;
        virtual std::string GetDescription() = 0;
    };

    template <typename ArgEntity>
    class ArgProcessor : public ArgProcessorBase {
    public:
        virtual void Parse(const std::string& raw_value) final {
            ParseArgValue(raw_value, &value);
            is_initialized = true;
        }

        virtual std::string GetEntityName() {
            return typeid(ArgEntity).name();
        }

        virtual std::string GetDescription() {
            ArgEntity arg_entity;
            return arg_entity.description;
        }

        typename ArgEntity::type value = {};
        bool is_initialized = false;
    };

    std::string GetHelpString();
public:
    template <typename ArgEntity>
    static void Register() {
        ArgEntity arg_entity;
        auto it = GetInstance().arg_processors.find(arg_entity.name);
        if (it == GetInstance().arg_processors.end()) {
            GetInstance().arg_processors[arg_entity.name] = std::make_unique<ArgProcessor<ArgEntity>>();
        } else if (it->second->GetEntityName() != typeid(ArgEntity).name()) {
                std::stringstream ss;
                ss << "There are two declarations of arg '" << arg_entity.name <<
                    "', one coming from '" << it->second->GetEntityName() <<
                    "', another coming from '" << typeid(ArgEntity).name() << "'";
                throw ExceptionWithBacktrace(ss.str());
        }
    }

    static void SetArgV(int argc, char **argv);

    template <typename ArgEntity>
    static typename ArgEntity::type GetValue() {
        AutoRegistrar<ArgParser, ArgEntity>::AutoRegister();
        ArgEntity arg_entity;
        auto it = GetInstance().arg_processors.find(arg_entity.name);
        assert(it != GetInstance().arg_processors.end());
        ArgProcessor<ArgEntity> * storage = dynamic_cast<ArgProcessor<ArgEntity> *>(it->second.get());
        if (!storage->is_initialized) {
            std::stringstream ss;
            ss << "Flag '" << arg_entity.name << "' is not initialized";
            throw ExceptionWithBacktrace(ss.str());
        }
        return storage->value;
    }
private:
    UnorderedMap<std::string, std::unique_ptr<ArgProcessorBase>, std::hash<std::string>> arg_processors;
};