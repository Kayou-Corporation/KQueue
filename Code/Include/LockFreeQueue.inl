// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <HazardPointerManager.hpp>

template <typename T>
LockFreeQueue<T>::LockFreeQueue()
{
    Node* dummy = new Node();
    m_head.store(dummy);
    m_tail.store(dummy);
}

template <typename T>
LockFreeQueue<T>::~LockFreeQueue()
{
    while (Pop()) {}
    delete m_head.load();
}

template <typename T>
void LockFreeQueue<T>::Push(const T& value)
{
    Node* new_node = new Node(value);
    while (true) 
    {
        Node* last = m_tail.load(std::memory_order_acquire);
        Node* next = last->next.load(std::memory_order_acquire);
        if (last == m_tail.load(std::memory_order_acquire)) 
        {
            if (next == nullptr) 
            {
                if (last->next.compare_exchange_weak(
                    next, new_node,
                    std::memory_order_release, std::memory_order_relaxed)) 
                {
                    m_tail.compare_exchange_weak(
                        last, new_node,
                        std::memory_order_release, std::memory_order_relaxed);
                    return;
                }
            }
            else 
            {
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
    HazardPointer* hp = hpm.Acquire();
    while (true) 
    {
        Node* first = m_head.load(std::memory_order_acquire);
        hp->mPtr.store(first);
        if (first != m_head.load(std::memory_order_acquire))
        {
            continue;
        }

        Node* next = first->next.load(std::memory_order_acquire);
        if (next == nullptr) 
        {
            HazardPointerManager::Release(hp);
            return nullptr;
        }

        T* result = next->data;
        if (m_head.compare_exchange_weak(
            first, next,
            std::memory_order_release, std::memory_order_relaxed))
        {
            hp->mPtr.store(nullptr);
            hpm.Release(hp);
            hpm.RetireNode(first, ReclaimNode);
            return result;
        }
    }
}

template <typename T>
void LockFreeQueue<T>::ReclaimNode(void* node) 
{
    delete static_cast<Node*>(node);
}
