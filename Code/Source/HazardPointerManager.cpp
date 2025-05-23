// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#include "HazardPointerManager.hpp"

#include <stdexcept>

namespace KQueue
{
    thread_local std::vector<void*> HazardPointerManager::m_retired = { nullptr };

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
            // Atomically assign the current thread ID to a free hazard pointer slot
            if (std::thread::id empty; hpRecord.mId.compare_exchange_strong(empty, this_id))
            {
                return &hpRecord;  // Successfully acquired
            }
        }
        throw std::runtime_error("No hazard pointers available");
    }

    void HazardPointerManager::Release(HazardPointer* hp)
    {
        hp->mPtr.store(nullptr);               // Clear protected pointer
        hp->mId.store(std::thread::id());      // Mark slot as unused
    }

    void HazardPointerManager::RetireNode(void* node, void(*deleter)(void*)) const
    {
        m_retired.push_back(node);             // Add node to thread-local retired list

        if (m_retired.size() >= 10)            // Trigger a scan when list gets large
        {
            Scan(deleter);
        }
    }

    void HazardPointerManager::ForceCleanup(void(*deleter)(void*)) const
    {
        Scan(deleter);
    }

    bool HazardPointerManager::IsHazard(void* p) const
    {
        for (const auto& hpRecord : m_hpRecords)
        {
            if (hpRecord.mPtr.load() == p)
            {
                return true;  // Pointer is still in use
            }
        }
        return false;
    }

    void HazardPointerManager::Scan(void(*deleter)(void*)) const
    {
        std::vector<void*> to_reclaim;

        // Identify nodes safe to reclaim
        for (void* p : m_retired)
        {
            if (!IsHazard(p))
            {
                to_reclaim.push_back(p);
            }
        }

        // Delete safe nodes and remove them from retired list
        for (void* p : to_reclaim)
        {
            deleter(p);
            std::erase(m_retired, p);  // C++20 std::erase — remove from retired list
        }
    }
}
