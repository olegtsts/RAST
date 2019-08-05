#pragma once

#include <cstring>
#include <cstdint>
#include <iostream>

template <int DataSize, typename ... Args>
class Packed {
public:
    Packed() {
        std::memset(data, 0, DataSize);
    }
protected:
    void GetInternal() {}
    void SetInternal() {}
    char data[(DataSize + 7) / 8 * 8];
};

template <typename T>
class TypeSpecifier {};

template <typename NameClassType, int StoreBegin, int StoreEnd>
class Param {
public:
    using NameClass=NameClassType;
    static const int store_begin = StoreBegin;
    static const int store_end = StoreEnd;
};

template <typename NameClass, int StoreBegin, int StoreEnd>
const int Param<NameClass, StoreBegin, StoreEnd>::store_begin;

template <typename NameClass, int StoreBegin, int StoreEnd>
const int Param<NameClass, StoreBegin, StoreEnd>::store_end;

template <int DataSize, typename StoreClass, typename ... Args>
class Packed<DataSize, StoreClass, Args...> : public Packed<DataSize, Args...> {
protected:
    using Packed<DataSize, Args...>::data;
    using Packed<DataSize, Args...>::GetInternal;
    using Packed<DataSize, Args...>::SetInternal;

    typename StoreClass::NameClass::VarType GetInternal(const TypeSpecifier<typename StoreClass::NameClass>&) const {
        char temp_data[sizeof(typename StoreClass::NameClass::VarType)];
        const int data_size = StoreClass::store_end - StoreClass::store_begin;
        std::memcpy(temp_data, data + StoreClass::store_begin, data_size);
        std::memset(temp_data + data_size, 0, sizeof(typename StoreClass::NameClass::VarType) - data_size);
        return *reinterpret_cast<typename StoreClass::NameClass::VarType const *>(temp_data);
    }
    void SetInternal(const TypeSpecifier<typename StoreClass::NameClass>&, const typename StoreClass::NameClass::VarType& value) {
        std::memcpy(data + StoreClass::store_begin, reinterpret_cast<char const *>(&value), StoreClass::store_end - StoreClass::store_begin);
    }
public:
    Packed()
        : Packed<DataSize, Args...>()
    {}

    template <typename SomeNameClass>
    typename SomeNameClass::VarType Get() const {
        return GetInternal(TypeSpecifier<SomeNameClass>());
    }
    template <typename SomeNameClass>
    void Set(const typename SomeNameClass::VarType& value) {
        return SetInternal(TypeSpecifier<SomeNameClass>(), value);
    }
};

