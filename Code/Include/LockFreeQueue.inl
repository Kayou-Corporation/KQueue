// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <HazardPointerManager.hpp>
#include <optional>

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

    HazardPointerManager& hpm = HazardPointerManager::GetInstance();
    hpm.ForceCleanup(ReclaimNode);

    delete m_head.load();                    // Delete the final dummy node
}

template <typename T>
void LockFreeQueue<T>::Push(const T& value)
{
    Node* new_node = new Node(value);
    while (true) 
    {
        Node* last = m_tail.load(std::memory_order_acquire);
        Node* next = last->mNext.load(std::memory_order_acquire);
        if (last == m_tail.load(std::memory_order_acquire)) 
        {
            if (next == nullptr) 
            {
                if (last->mNext.compare_exchange_weak(next, new_node,
                    std::memory_order_release,
                    std::memory_order_relaxed)) 
                {
                    m_tail.compare_exchange_weak(last, new_node,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                    return;
                }
            }
            else 
            {
                m_tail.compare_exchange_weak(last, next,
                    std::memory_order_release,
                    std::memory_order_relaxed);
            }
        }
    }
}

template <typename T>
T* LockFreeQueue<T>::Pop() {
    HazardPointerManager& hpm = HazardPointerManager::GetInstance();
    HazardPointer* hp = hpm.Acquire();

    while (true) 
    {
        Node* first;
        do 
        {
            first = m_head.load(std::memory_order_acquire);
            hp->mPtr.store(first, std::memory_order_release);
        } while (first != m_head.load(std::memory_order_acquire));

        Node* next = first->mNext.load(std::memory_order_acquire);
        if (next == nullptr) 
        {
            HazardPointerManager::Release(hp);
            return nullptr;
        }

        T* result = new T(next->mData);  // Allocate copy
        if (m_head.compare_exchange_weak(first, next, std::memory_order_release)) 
        {
            hp->mPtr.store(nullptr);
            hpm.Release(hp);
            hpm.RetireNode(first, ReclaimNode);
            return result;
        }
        hpm.Release(hp);
    	delete result;  // rollback
    }
}

template <typename T>
void LockFreeQueue<T>::ReclaimNode(void* node)
{
    delete static_cast<Node*>(node);        // Custom deleter used by hazard pointer retire list
}
