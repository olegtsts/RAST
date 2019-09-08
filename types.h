#pragma once

#include <vector>
#include <deque>
#include <set>

#include "allocator.h"

template <typename T>
using Vector=std::vector<T, FixedFreeListMultiLevelAllocator<T>>;

template <typename T>
using Deque=std::deque<T, FixedFreeListMultiLevelAllocator<T>>;

template <typename TKey, typename TValue, typename THash>
using UnorderedMap=std::unordered_map<TKey, TValue, THash, std::equal_to<TKey>, FixedFreeListMultiLevelAllocator<std::pair<const TKey, TValue>>>;

template <typename T>
using Set=std::set<T, std::less<T>, std::allocator<T>>;
