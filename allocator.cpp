#include "allocator.h"
#include <algorithm>
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
    size_t layer = std::min(static_cast<size_t>(front_control->source_layer), GetLog2(front_control->data_size));
    front_control->local_prev = nullptr;
    front_control->local_next = layers[layer];
    if (layers[layer] != nullptr) {
        layers[layer]->local_prev = front_control;
    }
    layers[layer] = front_control;
}

void FreeListMultiLevelAllocator::Detach(FrontControl* front_control) {
    size_t layer = std::min(static_cast<size_t>(front_control->source_layer), GetLog2(front_control->data_size));
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
    first_block->state |= second_block->state;
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
        second_front_control->state = front_control->state & ~FIRST_BLOCK_BIT;
        second_front_control->source_layer = front_control->source_layer;
        second_back_control->front_control = second_front_control;
        front_control->data_size = first_size;
        front_control->state &= ~LAST_BLOCK_BIT;
        first_back_control->front_control = front_control;
        Attach(front_control);
        Attach(second_front_control);
    }
}

void* FreeListMultiLevelAllocator::Allocate(const size_t size) {
    size_t layer = GetLog2(size);
    if (layers[layer] == nullptr) {
        char* arena = new char[MEM_ALLOCATED_AT_ONCE];
        FrontControl* front_control = reinterpret_cast<FrontControl*>(arena);
        BackControl* back_control = reinterpret_cast<BackControl*>(arena + MEM_ALLOCATED_AT_ONCE - sizeof(BackControl));
        front_control->data_size = MEM_ALLOCATED_AT_ONCE - sizeof(FrontControl) - sizeof(BackControl);
        front_control->local_prev = nullptr;
        front_control->local_next = nullptr;
        front_control->state = FIRST_BLOCK_BIT | LAST_BLOCK_BIT | IS_OWNED_BIT;
        front_control->source_layer = layer;
        back_control->front_control = front_control;
        Attach(front_control);
    }
    FrontControl* front_control = layers[layer];
    SplitBlock(front_control, size);
    front_control->state &= ~IS_OWNED_BIT;
    Detach(front_control);

    void* allocated_pointer = reinterpret_cast<void*>(reinterpret_cast<char*>(front_control) + sizeof(FrontControl));
    return allocated_pointer;
}

void FreeListMultiLevelAllocator::Deallocate(void* pointer) {
    FrontControl* front_control = reinterpret_cast<FrontControl*>(reinterpret_cast<char*>(pointer) - sizeof(FrontControl));
    Attach(front_control);
    front_control->state |= IS_OWNED_BIT;
    if (!(front_control->state & FIRST_BLOCK_BIT)) {
        BackControl* back_control = reinterpret_cast<BackControl*>(
            reinterpret_cast<char*>(pointer) - sizeof(FrontControl) - sizeof(BackControl));
        FrontControl* previous_front_conrol = back_control->front_control;
        if (previous_front_conrol->state & IS_OWNED_BIT) {
            Join(previous_front_conrol, front_control);
            front_control = previous_front_conrol;
        }
    }
    if (!(front_control->state & LAST_BLOCK_BIT)) {
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
                    "(data_size=" << cur->data_size << ", state=" << cur->state << ")";
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
