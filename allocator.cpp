#include "allocator.h"
#include <algorithm>
#include <string>
#include <sstream>
#include <memory>

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
    size_t layer = std::min(static_cast<size_t>(front_control->Get<FCSourceLayer>()), GetLog2(front_control->Get<FCDataSize>()));
    front_control->Set<FCLocalPrev>(nullptr);
    front_control->Set<FCLocalNext>(nullptr);
    if (layers[layer] != nullptr) {
        layers[layer]->Set<FCLocalPrev>(front_control);
    }
    layers[layer] = front_control;
}

void FreeListMultiLevelAllocator::Detach(FrontControl* front_control) {
    size_t layer = std::min(static_cast<size_t>(front_control->Get<FCSourceLayer>()), GetLog2(front_control->Get<FCDataSize>()));
    if (front_control->Get<FCLocalPrev>() != nullptr) {
        front_control->Get<FCLocalPrev>()->Set<FCLocalNext>(front_control->Get<FCLocalNext>());
    }
    if (front_control->Get<FCLocalNext>() != nullptr) {
        front_control->Get<FCLocalNext>()->Set<FCLocalPrev>(front_control->Get<FCLocalPrev>());
    }
    if (layers[layer] == front_control) {
        layers[layer] = front_control->Get<FCLocalNext>();
    }
    front_control->Set<FCLocalNext>(nullptr);
    front_control->Set<FCLocalPrev>(nullptr);
}

BackControl* FreeListMultiLevelAllocator::GetBackControl(FrontControl* front_control) {
    return reinterpret_cast<BackControl*>(reinterpret_cast<char*>(front_control) + sizeof(FrontControl) + front_control->Get<FCDataSize>());
}

void FreeListMultiLevelAllocator::Join(FrontControl* first_block, FrontControl* second_block) {
    Detach(first_block);
    Detach(second_block);
    GetBackControl(second_block)->front_control = first_block;
    first_block->Set<FCState>(first_block->Get<FCState>() | second_block->Get<FCState>());
    first_block->Set<FCDataSize>(first_block->Get<FCDataSize>() + sizeof(BackControl) + sizeof(FrontControl) + second_block->Get<FCDataSize>());
    Attach(first_block);
}

void FreeListMultiLevelAllocator::SplitBlock(FrontControl* front_control, size_t first_size) {
    if (front_control->Get<FCDataSize>() > first_size + sizeof(BackControl) + sizeof(FrontControl)) {
        Detach(front_control);
        BackControl* second_back_control = GetBackControl(front_control);
        BackControl* first_back_control = reinterpret_cast<BackControl*>(reinterpret_cast<char*>(front_control) + sizeof(FrontControl) + first_size);
        FrontControl* second_front_control = reinterpret_cast<FrontControl*>(reinterpret_cast<char*>(first_back_control) + sizeof(BackControl));
        second_front_control->Set<FCDataSize>(front_control->Get<FCDataSize>() - first_size - sizeof(BackControl) - sizeof(FrontControl));
        second_front_control->Set<FCLocalNext>(nullptr);
        second_front_control->Set<FCLocalPrev>(nullptr);
        second_front_control->Set<FCState>(front_control->Get<FCState>() & ~FIRST_BLOCK_BIT);
        second_front_control->Set<FCSourceLayer>(front_control->Get<FCSourceLayer>());
        second_back_control->front_control = second_front_control;
        front_control->Set<FCDataSize>(first_size);
        front_control->Set<FCState>(front_control->Get<FCState>() & ~LAST_BLOCK_BIT);
        first_back_control->front_control = front_control;
        Attach(front_control);
        Attach(second_front_control);
    }
}

void* FreeListMultiLevelAllocator::Allocate(size_t size, size_t alignment, size_t struct_size) {
    size_t size_with_alignment = size + alignment - 8;
    size_t layer = GetLog2(size_with_alignment);
    if (layers[layer] == nullptr) {
        char* arena = new char[MEM_ALLOCATED_AT_ONCE];
        FrontControl* front_control = reinterpret_cast<FrontControl*>(arena);
        BackControl* back_control = reinterpret_cast<BackControl*>(arena + MEM_ALLOCATED_AT_ONCE - sizeof(BackControl));
        front_control->Set<FCDataSize>(MEM_ALLOCATED_AT_ONCE - sizeof(FrontControl) - sizeof(BackControl));
        front_control->Set<FCLocalPrev>(nullptr);
        front_control->Set<FCLocalNext>(nullptr);
        front_control->Set<FCState>(FIRST_BLOCK_BIT | LAST_BLOCK_BIT | IS_OWNED_BIT);
        front_control->Set<FCSourceLayer>(layer);
        back_control->front_control = front_control;
        Attach(front_control);
    }
    FrontControl* front_control = layers[layer];
    SplitBlock(front_control, size_with_alignment);
    front_control->Set<FCState>(front_control->Get<FCState>() & ~IS_OWNED_BIT);
    Detach(front_control);

    void* allocated_pointer = reinterpret_cast<void*>(reinterpret_cast<char*>(front_control) + sizeof(FrontControl));
    void* aligned_pointer = allocated_pointer;
    std::align(alignment, struct_size, aligned_pointer, size_with_alignment);
    size_t shift = reinterpret_cast<char*>(aligned_pointer) - reinterpret_cast<char*>(allocated_pointer);
    *(reinterpret_cast<char*>(aligned_pointer) - 1) = static_cast<char>(shift);
    return aligned_pointer;
}

void FreeListMultiLevelAllocator::Deallocate(void* pointer) {
    size_t shift = static_cast<size_t>(*(reinterpret_cast<char*>(pointer) - 1));
    FrontControl* front_control = reinterpret_cast<FrontControl*>(reinterpret_cast<char*>(pointer) - shift - sizeof(FrontControl));
    Attach(front_control);
    front_control->Set<FCState>(front_control->Get<FCState>() | IS_OWNED_BIT);
    if (!(front_control->Get<FCState>() & FIRST_BLOCK_BIT)) {
        BackControl* back_control = reinterpret_cast<BackControl*>(
            reinterpret_cast<char*>(pointer) - shift - sizeof(FrontControl) - sizeof(BackControl));
        FrontControl* previous_front_conrol = back_control->front_control;
        if (previous_front_conrol->Get<FCState>() & IS_OWNED_BIT) {
            Join(previous_front_conrol, front_control);
            front_control = previous_front_conrol;
        }
    }
    if (!(front_control->Get<FCState>() & LAST_BLOCK_BIT)) {
        FrontControl* following_front_control = reinterpret_cast<FrontControl*>(
            reinterpret_cast<char*>(pointer) - shift + front_control->Get<FCDataSize>() + sizeof(BackControl));
        if (following_front_control->Get<FCState>() & IS_OWNED_BIT) {
            Join(front_control, following_front_control);
        }
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
                    "(data_size=" << cur->Get<FCDataSize>() << ", state=" << cur->Get<FCState>() << ")";
                cur = cur->Get<FCLocalNext>();
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
