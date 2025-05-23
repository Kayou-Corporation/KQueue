// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <atomic>

#include "HazardPointerManager.hpp"

/**
 * @file LockFreeQueue.hpp
 * @brief Declaration of a lock-free multi-producer, multi-consumer queue using hazard pointers.
 */

namespace KQueue
{
    /**
 * @class LockFreeQueue
 * @brief A lock-free queue supporting multiple producers and consumers, using hazard pointers for safe memory reclamation.
 *
 * @tparam T Type of elements stored in the queue.
 */
    template <typename T>
    class LockFreeQueue
    {
    public:
        /**
         * @brief Constructs an empty lock-free queue.
         */
        LockFreeQueue();

        /**
         * @brief Destroys the queue, ensuring safe memory cleanup.
         */
        ~LockFreeQueue();

        /**
         * @brief Enqueues a value into the queue.
         * @param value The value to be added.
         */
        void Push(const T& value);

        /**
         * @brief Dequeues a value from the queue.
         * @return Pointer to the dequeued value, or nullptr if the queue is empty.
         * @note The caller is responsible for deleting the returned pointer.
         */
        T* Pop();

        /**
         * @class ThreadCleanup
         * @brief Helper class to trigger hazard pointer cleanup on thread exit.
         */
        class ThreadCleanup
        {
        public:
            /**
             * @brief Destructor performs forced cleanup of retired nodes.
             */
            ~ThreadCleanup()
            {
                HazardPointerManager::GetInstance().ForceCleanup([](void* p)
                    {
                        delete static_cast<typename LockFreeQueue<T>::Node*>(p);
                    });
            }
        };

    private:
        /**
         * @struct Node
         * @brief Internal structure representing a queue node.
         */
        struct Node
        {
            T mData;                      ///< Stored data
            std::atomic<Node*> mNext;    ///< Pointer to the next node

            /**
             * @brief Constructs a node with a value.
             * @param val The value to store.
             */
            Node(const T& val) : mData(val), mNext(nullptr) {}

            /**
             * @brief Constructs a dummy node with no value.
             */
            Node() : mNext(nullptr) {}
        };

        std::atomic<Node*> m_head; ///< Pointer to the head node
        std::atomic<Node*> m_tail; ///< Pointer to the tail node

        /**
         * @brief Reclaims memory for a retired node.
         * @param node Pointer to the node to delete.
         */
        static void ReclaimNode(void* node);
    };
}

#include "LockFreeQueue.inl"