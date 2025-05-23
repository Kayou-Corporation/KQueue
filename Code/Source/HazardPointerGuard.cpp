// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#include "HazardPointerGuard.hpp"

#include "HazardPointerManager.hpp"

HazardPointerGuard::HazardPointerGuard() : m_hp(HazardPointerManager::GetInstance().Acquire()) {} // Acquire a hazard pointer slot for this thread

HazardPointerGuard::~HazardPointerGuard()
{
    if (m_hp)
    {
        // Clear the hazard pointer's protected pointer before releasing
        m_hp->mPtr.store(nullptr, std::memory_order_release);
        // Release this hazard pointer slot so it can be reused by other threads
        HazardPointerManager::Release(m_hp);
    }
}
