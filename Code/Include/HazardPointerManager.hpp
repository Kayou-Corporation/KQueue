// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#pragma once
#include <atomic>
#include <thread>
#include <vector>

/**
 * @file HazardPointerManager.hpp
 * @brief Manages hazard pointers and retired nodes in a lock-free context.
 */

namespace KQueue
{
    constexpr int MAX_HAZARD_POINTERS = 100;

    /**
     * @struct HazardPointer
     * @brief Represents a hazard pointer with thread ID and pointer value.
     */
    struct HazardPointer
    {
        std::atomic<std::thread::id> mId;  ///< ID of the thread owning the hazard pointer
        std::atomic<void*> mPtr;          ///< Pointer protected by the hazard pointer
    };

    /**
     * @class HazardPointerManager
     * @brief Singleton managing hazard pointers and safe memory reclamation.
     */
    class HazardPointerManager
    {
    public:
        /**
         * @brief Gets the singleton instance of the manager.
         * @return Reference to the HazardPointerManager instance.
         */
        static HazardPointerManager& GetInstance();

        /**
         * @brief Acquires a hazard pointer for the current thread.
         * @return Pointer to the acquired HazardPointer.
         */
        HazardPointer* Acquire();

        /**
         * @brief Releases a previously acquired hazard pointer.
         * @param hp The hazard pointer to release.
         */
        static void Release(HazardPointer* hp);

        /**
         * @brief Adds a node to the retired list for future safe deletion.
         * @param node Pointer to the node to retire.
         * @param deleter Function to delete the node when it's safe.
         */
        void RetireNode(void* node, void (*deleter)(void*)) const;

        /**
         * @brief Forces cleanup of all retired nodes.
         * @param deleter Function to delete each node.
         */
        void ForceCleanup(void (*deleter)(void*)) const;

        /**
         * @brief Checks if a pointer is currently protected by any hazard pointer.
         * @param p The pointer to check.
         * @return True if the pointer is hazardous.
         */
        bool IsHazard(void* p) const;

    private:
        HazardPointerManager() = default;

        /**
         * @brief Scans retired nodes and reclaims memory if safe.
         * @param deleter Function to delete reclaimed nodes.
         */
        void Scan(void (*deleter)(void*)) const;

        HazardPointer m_hpRecords[MAX_HAZARD_POINTERS]; ///< Array of hazard pointer records
        thread_local static std::vector<void*> m_retired; ///< List of retired nodes
    };
}