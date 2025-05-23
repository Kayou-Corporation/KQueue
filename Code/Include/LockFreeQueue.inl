// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <HazardPointerManager.hpp>

template <typename T>
LockFreeQueue<T>::LockFreeQueue()
{
    Node* dummy = new Node();                // Create a dummy node to simplify push/pop logic
    m_head.store(dummy);                     // Both head and tail initially point to the dummy
    m_tail.store(dummy);
}

template <typename T>
LockFreeQueue<T>::~LockFreeQueue()
{
    while (Pop()) {}                         // Drain remaining elements to safely reclaim memory
    delete m_head.load();                    // Delete the final dummy node
}

template <typename T>
void LockFreeQueue<T>::Push(const T& value)
{
    Node* new_node = new Node(value);        // Allocate a new node containing the value
    while (true)
    {
        Node* last = m_tail.load(std::memory_order_acquire);     // Get the current tail
        Node* next = last->next.load(std::memory_order_acquire); // Get the next pointer

        if (last == m_tail.load(std::memory_order_acquire))      // Recheck consistency
        {
            if (next == nullptr)             // If tail is at the end
            {
                // Try to link the new node to the current tail
                if (last->next.compare_exchange_weak(
                    next, new_node,
                    std::memory_order_release, std::memory_order_relaxed))
                {
                    // Try to move tail to the new node (optional for performance)
                    m_tail.compare_exchange_weak(
                        last, new_node,
                        std::memory_order_release, std::memory_order_relaxed);
                    return;
                }
            }
            else
            {
                // Tail was lagging behind, try to help advance it
                m_tail.compare_exchange_weak(
                    last, next,
                    std::memory_order_release, std::memory_order_relaxed);
            }
        }
    }
}

template <typename T>
T* LockFreeQueue<T>::Pop()
{
    HazardPointerManager& hpm = HazardPointerManager::GetInstance();
    HazardPointer* hp = hpm.Acquire();  // Acquire a hazard pointer for safety

    while (true)
    {
        Node* first = m_head.load(std::memory_order_acquire);
        hp->mPtr.store(first);                          // Protect this node from deletion
        if (first != m_head.load(std::memory_order_acquire))  // Check for consistency
        {
            continue;
        }

        Node* next = first->next.load(std::memory_order_acquire);
        if (next == nullptr)
        {
            HazardPointerManager::Release(hp);          // Nothing to pop
            return nullptr;
        }

        T* result = next->data;                         // Read the data (safe now)
        if (m_head.compare_exchange_weak(
            first, next,
            std::memory_order_release, std::memory_order_relaxed))
        {
            hp->mPtr.store(nullptr);                    // Clear hazard
            hpm.Release(hp);                            // Release hazard slot
            hpm.RetireNode(first, ReclaimNode);         // Safe to retire the old head
            return result;
        }
    }
}

template <typename T>
void LockFreeQueue<T>::ReclaimNode(void* node)
{
    delete static_cast<Node*>(node);        // Custom deleter used by hazard pointer retire list
}
