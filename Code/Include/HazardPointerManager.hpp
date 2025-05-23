// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <unordered_set>
#include <cassert>
#include <mutex>

constexpr int MAX_HAZARD_POINTERS = 100;

struct HazardPointer
{
    std::atomic<std::thread::id> mId;
    std::atomic<void*> mPtr;
};

class HazardPointerManager
{
public:
    static HazardPointerManager& GetInstance();

    HazardPointer* Acquire();

    static void Release(HazardPointer* hp);

    void RetireNode(void* node, void (*deleter)(void*)) const;

    bool IsHazard(void* p) const;

private:
    HazardPointerManager() = default;

    void Scan(void (*deleter)(void*)) const;

    HazardPointer m_hpRecords[MAX_HAZARD_POINTERS];
    thread_local static std::vector<void*> m_retired;
};