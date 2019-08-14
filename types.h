#pragma once

#include <vector>
#include <deque>

#include "allocator.h"

template <typename T>
using Vector=std::vector<T, FixedFreeListMultiLevelAllocator<T>>;

template <typename T>
using Deque=std::deque<T, FixedFreeListMultiLevelAllocator<T>>;

