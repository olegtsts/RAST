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
    static bool ParseArgValue(const std::string& arg_value, int* value);
    static bool ParseArgValue(const std::string& arg_value, bool* value);

    ArgParser() {
    }

    static ArgParser& GetInstance() {
        static ArgParser arg_parser;
        return arg_parser;
    }

    class ArgProcessorBase {
    public:
        virtual void Parse(const std::string& raw_value) = 0;
        virtual std::string GetEntityName() const noexcept = 0;
        virtual std::string GetDescription() const noexcept = 0;
        virtual bool IsInitialized() const noexcept = 0;
        virtual bool IsBool() const noexcept = 0;
        virtual bool HasDefaultValue() const noexcept = 0;
    };

    template <typename ArgEntity>
    class ArgProcessorWithoutDefaultValue : public ArgProcessorBase {
    public:
        virtual void Parse(const std::string& raw_value) final {
            if (!ParseArgValue(raw_value, &value)) {
                std::stringstream ss;
                ArgEntity arg_entity;
                ss << "Failed to parse value '" << raw_value <<
                    "' of arg '" << arg_entity.name << "' of type '" <<
                    typeid(typename ArgEntity::type).name() << "'";
                throw ExceptionWithBacktrace(ss.str());
            }
            is_value_initialized = true;
        }

        virtual std::string GetEntityName() const noexcept {
            return typeid(ArgEntity).name();
        }

        virtual std::string GetDescription() const noexcept {
            ArgEntity arg_entity;
            return arg_entity.description;
        }

        virtual bool IsBool() const noexcept {
            return typeid(typename ArgEntity::type).hash_code() == typeid(bool).hash_code();
        }

        virtual bool IsInitialized() const noexcept {
            return is_value_initialized;
        }

        virtual bool HasDefaultValue() const noexcept = 0;

        typename ArgEntity::type value = {};

    private:
        bool is_value_initialized = false;
    };

    template <typename ArgEntity, typename = int>
    class ArgProcessor : public ArgProcessorWithoutDefaultValue<ArgEntity> {
    public:
        virtual bool HasDefaultValue() const noexcept {
            return false;
        }

        std::optional<typename ArgEntity::type> GetDefaultValue() const noexcept {
            return {};
        }
    };

    template <typename ArgEntity>
    class ArgProcessor<ArgEntity, decltype((void) ArgEntity::default_value, 0)> : public ArgProcessorWithoutDefaultValue<ArgEntity> {
    public:
        virtual bool HasDefaultValue() const noexcept {
            return true;
        }

        std::optional<typename ArgEntity::type> GetDefaultValue() const noexcept {
            ArgEntity arg_entity;
            return arg_entity.default_value;
        }
    };

    std::string GetHelpString() const;
    static bool IsStringFlag(const std::string& input_string);
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

    static bool SetArgV(int argc, char **argv);

    template <typename ArgEntity>
    static typename ArgEntity::type GetValue() noexcept {
        AutoRegistrar<ArgParser, ArgEntity>::AutoRegister();
        ArgEntity arg_entity;
        auto it = GetInstance().arg_processors.find(arg_entity.name);
        assert(it != GetInstance().arg_processors.end());
        ArgProcessor<ArgEntity> * arg_processor = dynamic_cast<ArgProcessor<ArgEntity> *>(it->second.get());
        if (arg_processor->IsInitialized()) {
            return arg_processor->value;
        } else {
            std::optional<typename ArgEntity::type> default_value = arg_processor->GetDefaultValue();
            assert(default_value.has_value());
            return default_value.value();
        }
    }
private:
    UnorderedMap<std::string, std::unique_ptr<ArgProcessorBase>, std::hash<std::string>> arg_processors;
};
