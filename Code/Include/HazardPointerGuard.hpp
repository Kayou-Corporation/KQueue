// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once

/**
 * @file HazardPointerGuard.hpp
 * @brief Provides RAII-style management for hazard pointers.
 */

namespace KQueue
{
    struct HazardPointer;

    /**
     * @class HazardPointerGuard
     * @brief Manages a hazard pointer using RAII to ensure safe memory access in lock-free structures.
     */
    class HazardPointerGuard
    {
    public:
        /**
         * @brief Constructs a HazardPointerGuard and acquires a hazard pointer.
         */
        HazardPointerGuard();

        /**
         * @brief Destructs the HazardPointerGuard and releases the hazard pointer.
         */
        ~HazardPointerGuard();

        /**
         * @brief Gets the underlying hazard pointer.
         * @return A pointer to the managed HazardPointer.
         */
        [[nodiscard]] HazardPointer* Get() const { return m_hp; }

    private:
        HazardPointer* m_hp; ///< Pointer to the hazard-protected data
    };
}
