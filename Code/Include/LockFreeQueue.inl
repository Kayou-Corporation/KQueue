// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <optional>

#include "HazardPointerManager.hpp"
#include "HazardPointerGuard.hpp"

namespace KQueue
{
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
        Node* new_node = new Node(value);  // Allocate new node with given value

        while (true)
        {
            // Load the current tail node (last node in the queue)
            Node* last = m_tail.load(std::memory_order_acquire);

            // Load the next pointer of last node to see if someone else inserted after it
            Node* next = last->mNext.load(std::memory_order_acquire);

            // Check if tail pointer is still consistent after loading next
            if (last == m_tail.load(std::memory_order_acquire))
            {
                if (next == nullptr)  // Tail is pointing at actual last node (no next)
                {
                    // Try to link the new node as the next node of last (push)
                    if (last->mNext.compare_exchange_weak(next, new_node,
                        std::memory_order_release,
                        std::memory_order_relaxed))
                    {
                        // Try to swing the tail pointer forward to the new node
                        m_tail.compare_exchange_weak(last, new_node,
                            std::memory_order_release,
                            std::memory_order_relaxed);

                        return;  // Push successful
                    }
                }
                else
                {
                    // Another thread inserted a node after last but tail pointer lagged behind
                    // Move tail forward to catch up
                    m_tail.compare_exchange_weak(last, next,
                        std::memory_order_release,
                        std::memory_order_relaxed);
                }
            }
        }
    }

    template <typename T>
    T* LockFreeQueue<T>::Pop()
    {
        thread_local ThreadCleanup cleanupGuard; // Ensures retired nodes are cleaned up periodically when thread exits

        HazardPointerGuard guard;              // Acquire hazard pointer for current thread
        HazardPointer* hp = guard.Get();       // Get the hazard pointer slot to protect nodes

        while (true)
        {
            Node* first;

            // Load the head pointer, protect it with hazard pointer, then verify no change
            do {
                first = m_head.load(std::memory_order_acquire);
                hp->mPtr.store(first, std::memory_order_release);
            } while (first != m_head.load(std::memory_order_acquire));

            // Load next pointer (the node after the dummy or current head)
            Node* next = first->mNext.load(std::memory_order_acquire);

            if (!next)
            {
                // Queue is empty (only dummy node), return nullptr to signal empty
                return nullptr;
            }

            // Copy the data from next node to return
            T* result = new T(next->mData);

            // Attempt to swing head forward from first to next atomically
            if (m_head.compare_exchange_weak(first, next,
                std::memory_order_release,
                std::memory_order_relaxed))
            {
                // Success: Clear hazard pointer protection on old head node
                hp->mPtr.store(nullptr, std::memory_order_release);

                // Retire the old head node (dummy node) for deferred reclamation
                HazardPointerManager::GetInstance().RetireNode(first, ReclaimNode);

                return result;  // Return copied data pointer
            }

            // CAS failed: some other thread modified head first; rollback
            delete result;
        }
    }

    template <typename T>
    void LockFreeQueue<T>::ReclaimNode(void* node)
    {
        delete static_cast<Node*>(node);        // Custom deleter used by hazard pointer retire list
    }
}
