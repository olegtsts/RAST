#include "allocator.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>

thread_local FreeListMultiLevelAllocator global_allocator;

FreeListMultiLevelAllocator::FreeListMultiLevelAllocator() {
    for (size_t i = 0; i < MAX_MEM_LAYERS; ++i) {
        layers[i] = nullptr;
    }
}

size_t FreeListMultiLevelAllocator::GetLog2(size_t number) {
    size_t log_value = 0;
    while (number > 1) {
        ++log_value;
        number >>= 1;
    }
    return log_value;
}

void FreeListMultiLevelAllocator::Attach(FrontControl* front_control) {
    size_t layer = GetLog2(front_control->data_size);
    front_control->local_prev = nullptr;
    front_control->local_next = layers[layer];
    if (layers[layer] != nullptr) {
        layers[layer]->local_prev = front_control;
    }
    layers[layer] = front_control;
}

void FreeListMultiLevelAllocator::Detach(FrontControl* front_control) {
    size_t layer = GetLog2(front_control->data_size);
    if (front_control->local_prev != nullptr) {
        front_control->local_prev->local_next = front_control->local_next;
    }
    if (front_control->local_next != nullptr) {
        front_control->local_next->local_prev = front_control->local_prev;
    }
    if (layers[layer] == front_control) {
        layers[layer] = front_control->local_next;
    }
    front_control->local_next = nullptr;
    front_control->local_prev = nullptr;
}

BackControl* FreeListMultiLevelAllocator::GetBackControl(FrontControl* front_control) {
    return reinterpret_cast<BackControl*>(reinterpret_cast<char*>(front_control) + sizeof(FrontControl) + front_control->data_size);
}

void FreeListMultiLevelAllocator::Join(FrontControl* first_block, FrontControl* second_block) {
    Detach(first_block);
    Detach(second_block);
    GetBackControl(second_block)->front_control = first_block;
    first_block->data_size += sizeof(BackControl) + sizeof(FrontControl) + second_block->data_size;
    Attach(first_block);
}

void FreeListMultiLevelAllocator::SplitBlock(FrontControl* front_control, size_t first_size) {
    if (front_control->data_size > first_size + sizeof(BackControl) + sizeof(FrontControl)) {
        Detach(front_control);
        BackControl* second_back_control = GetBackControl(front_control);
        BackControl* first_back_control = reinterpret_cast<BackControl*>(reinterpret_cast<char*>(front_control) + sizeof(FrontControl) + first_size);
        FrontControl* second_front_control = reinterpret_cast<FrontControl*>(reinterpret_cast<char*>(first_back_control) + sizeof(BackControl));
        second_front_control->data_size = front_control->data_size - first_size - sizeof(BackControl) - sizeof(FrontControl);
        second_front_control->local_next = nullptr;
        second_front_control->local_prev = nullptr;
        second_front_control->offset = front_control->offset + sizeof(FrontControl) + first_size + sizeof(BackControl);
        second_front_control->total_size = front_control->total_size;
        second_front_control->is_owned = true;
        second_back_control->front_control = second_front_control;
        front_control->data_size = first_size;
        first_back_control->front_control = front_control;
        Attach(front_control);
        Attach(second_front_control);
    }
}

void* FreeListMultiLevelAllocator::Allocate(const size_t size) {
    size_t layer = GetLog2(size);
    if (layers[layer] == nullptr) {
        size_t one_block_size = (pow(2, layer + 1) - 1) + sizeof(FrontControl) + sizeof(BackControl);
        size_t blocks_count = std::max(static_cast<size_t>(1), MEM_ALLOCATED_AT_ONCE / one_block_size);
        char* arena = new char[blocks_count * one_block_size];
        for (size_t offset = 0; offset < blocks_count * one_block_size; offset += one_block_size) {
            FrontControl* front_control = reinterpret_cast<FrontControl*>(arena + offset);
            BackControl* back_control = reinterpret_cast<BackControl*>(arena + one_block_size - sizeof(BackControl));
            front_control->data_size = one_block_size - sizeof(FrontControl) - sizeof(BackControl);
            front_control->local_prev = nullptr;
            front_control->local_next = nullptr;
            front_control->offset = offset % one_block_size;
            front_control->total_size = one_block_size;
            front_control->is_owned = true;
            back_control->front_control = front_control;
            Attach(front_control);
        }
    }
    FrontControl* front_control = layers[layer];
    SplitBlock(front_control, size);
    front_control->is_owned = false;
    Detach(front_control);

    void* allocated_pointer = reinterpret_cast<void*>(reinterpret_cast<char*>(front_control) + sizeof(FrontControl));
    return allocated_pointer;
}

void FreeListMultiLevelAllocator::Deallocate(void* pointer) {
    FrontControl* front_control = reinterpret_cast<FrontControl*>(reinterpret_cast<char*>(pointer) - sizeof(FrontControl));
    Attach(front_control);
    front_control->is_owned = true;
    if (front_control->offset > 0) {
        BackControl* back_control = reinterpret_cast<BackControl*>(
            reinterpret_cast<char*>(pointer) - sizeof(FrontControl) - sizeof(BackControl));
        FrontControl* previous_front_conrol = back_control->front_control;
        if (previous_front_conrol->is_owned) {
            Join(previous_front_conrol, front_control);
            front_control = previous_front_conrol;
        }
    }
    if (front_control->offset + sizeof(FrontControl) + front_control->data_size + sizeof(BackControl) < front_control->total_size) {
        FrontControl* following_front_control = reinterpret_cast<FrontControl*>(
            reinterpret_cast<char*>(pointer) + front_control->data_size + sizeof(BackControl));
        Join(front_control, following_front_control);
    }
}

std::string FreeListMultiLevelAllocator::DebugString() const {
    std::stringstream ss;
    bool is_empty = true;
    for (size_t i = 0; i < MAX_MEM_LAYERS; ++i) {
        if (layers[i] != nullptr) {
            ss << "layer " << i << ": ";
            FrontControl* cur = layers[i];
            while (cur != nullptr) {
                ss << " -> " << reinterpret_cast<void*>(reinterpret_cast<char*>(cur) + sizeof(FrontControl)) <<
                    "(data_size=" << cur->data_size << ", block=" <<
                    reinterpret_cast<void*>(reinterpret_cast<char*>(cur) - cur->offset) <<
                    ", offset=" << cur->offset << ", owned=" << cur->is_owned << ")";
                cur = cur->local_next;
            }
            ss << std::endl;
            is_empty = false;
        }
    }
    if (is_empty) {
        ss << "<empty>\n";
    }
    return ss.str();
}
