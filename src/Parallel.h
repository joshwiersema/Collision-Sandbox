#pragma once
#include <functional>
#include <thread>
#include <vector>

// Runs `work(threadIndex, begin, end)` on `numThreads` threads.
// The range [0, count) is split into contiguous chunks, one per thread.
// Each thread gets its own chunk, so as long as `work` only writes to
// indices inside its own [begin, end) there are no data races.
inline void parallelFor(int count, int numThreads,
                        const std::function<void(int, int, int)>& work) {
    if (numThreads < 1) {
        numThreads = 1;
    }
    if (numThreads == 1 || count < numThreads) {
        work(0, 0, count);
        return;
    }

    const int chunkSize = (count + numThreads - 1) / numThreads; // ceiling divide
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

    for (int t = 0; t < numThreads; ++t) {
        const int begin = t * chunkSize;
        const int end = (begin + chunkSize < count) ? begin + chunkSize : count;
        if (begin >= end) {
            break;
        }
        threads.emplace_back(work, t, begin, end);
    }

    for (std::thread& thread : threads) {
        thread.join();
    }
}

inline int hardwareThreadCount() {
    const unsigned int count = std::thread::hardware_concurrency();
    return count == 0 ? 1 : static_cast<int>(count);
}
