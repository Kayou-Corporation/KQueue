// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#include "HazardPointerManager.hpp"

thread_local std::vector<void*> HazardPointerManager::m_retired;

HazardPointerManager& HazardPointerManager::GetInstance()
{
    static HazardPointerManager s_instance;
    return s_instance;
}

HazardPointer* HazardPointerManager::Acquire()
{
    const std::thread::id this_id = std::this_thread::get_id();
    for (auto& hpRecord : m_hpRecords)
    {
        if (std::thread::id empty; hpRecord.mId.compare_exchange_strong(empty, this_id)) {
            return &hpRecord;
        }
    }
    throw std::runtime_error("No hazard pointers available");
}

void HazardPointerManager::Release(HazardPointer* hp)
{
    hp->mPtr.store(nullptr);
    hp->mId.store(std::thread::id());
}

void HazardPointerManager::RetireNode(void* node, void(* deleter)(void*)) const
{
    m_retired.push_back(node);
    if (m_retired.size() >= 10)  // Scan threshold
        Scan(deleter);
}

bool HazardPointerManager::IsHazard(void* p) const
{
    for (auto hpRecord : m_hpRecords)
    {
        if (hpRecord.mPtr.load() == p)
            return true;
    }
    return false;
}

void HazardPointerManager::Scan(void(* deleter)(void*)) const
{
    std::vector<void*> to_reclaim;
    for (void* p : m_retired) {
        if (!IsHazard(p)) {
            to_reclaim.push_back(p);
        }
    }
    for (void* p : to_reclaim) {
        deleter(p);
        std::erase(m_retired, p);
    }
}
