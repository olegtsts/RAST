#pragma once

#include <cstdint>
#include <iostream>

template <int NumberOfInts, typename ... Args>
class Packed {
protected:
    void GetInternal() {}
    void SetInternal() {}
    uint64_t data[(NumberOfInts + 7) / 8];
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

template <int NumberOfInts, typename StoreClass, typename ... Args>
class Packed<NumberOfInts, StoreClass, Args...> : public Packed<NumberOfInts, Args...> {
protected:
    using Packed<NumberOfInts, Args...>::data;
    using Packed<NumberOfInts, Args...>::GetInternal;
    using Packed<NumberOfInts, Args...>::SetInternal;
    typename StoreClass::NameClass::VarType GetInternal(const TypeSpecifier<typename StoreClass::NameClass>&) const {
        uint64_t place_to_read = *reinterpret_cast<uint64_t const *>(reinterpret_cast<char const *>(&data[0]) + StoreClass::store_begin);
        place_to_read &= ~(((1llu << ((8 - StoreClass::store_end + StoreClass::store_begin) * 8)) - 1) << (
                (StoreClass::store_end - StoreClass::store_begin) * 8));
        return *reinterpret_cast<typename StoreClass::NameClass::VarType const *>(&place_to_read);
    }
    void SetInternal(const TypeSpecifier<typename StoreClass::NameClass>&, const typename StoreClass::NameClass::VarType& value) {
        uint64_t& place_to_modify = *reinterpret_cast<uint64_t *>(reinterpret_cast<char *>(data) + StoreClass::store_begin);
        place_to_modify &= ~((1llu << ((StoreClass::store_end - StoreClass::store_begin) * 8)) - 1);
        place_to_modify |= *reinterpret_cast<uint64_t const *>(&value) & ((1llu << ((StoreClass::store_end - StoreClass::store_begin) * 8)) - 1);
    }
public:
    template <typename SomeNameClass>
    typename SomeNameClass::VarType Get() const {
        return GetInternal(TypeSpecifier<SomeNameClass>());
    }
    template <typename SomeNameClass>
    void Set(const typename SomeNameClass::VarType& value) {
        return SetInternal(TypeSpecifier<SomeNameClass>(), value);
    }
};

