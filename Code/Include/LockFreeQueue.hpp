// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <memory>
#include <atomic>

template <typename T>
class LockFreeQueue
{
public:
    LockFreeQueue();
    ~LockFreeQueue();

    void Push(const T& value);
    T* Pop();

private:
    struct Node
    {
        T mData;
        std::atomic<Node*> mNext;

        Node(const T& val) : mData(val), mNext(nullptr) {}
        Node() : mNext(nullptr) {}  // Dummy node
    };

    std::atomic<Node*> m_head;
    std::atomic<Node*> m_tail;

    static void ReclaimNode(void* node);
};

#include "LockFreeQueue.inl"