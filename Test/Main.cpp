// Copyright (c) 2025 Kayou Corporation. All Rights Reserved.

#include "LockFreeQueue.hpp"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

int main()
{
	KQueue::LockFreeQueue<int> queue;
    std::atomic<int> produced{ 0 }, consumed{ 0 };

    constexpr int PRODUCERS = 4;
    constexpr int CONSUMERS = 4;
    constexpr int ITEMS = 1000;

    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    auto startTime = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < PRODUCERS; ++i) 
    {
        producers.emplace_back([&]() {

            for (int j = 0; j < ITEMS; ++j) 
            {
                queue.Push(j);
                produced.fetch_add(1);
            }
            });
    }

    for (int i = 0; i < CONSUMERS; ++i) 
    {
        consumers.emplace_back([&]() 
            {
            int* item;
            while (consumed.load() < PRODUCERS * ITEMS) 
            {
                if ((item = queue.Pop()) != nullptr) 
                {
                    consumed.fetch_add(1);
                    delete item;  // Remember: we returned T* from Pop
                }
                else 
                {
                    std::this_thread::yield();
                }
            }
            });
    }

    for (auto& p : producers) p.join();
    for (auto& c : consumers) c.join();

    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();

    std::cout << "Produced: " << produced << ", Consumed: " << consumed << "\n";
    std::cout << "Done in " << duration << " ms" << "\n";

    producers.clear();
	consumers.clear();
    return 0;
}