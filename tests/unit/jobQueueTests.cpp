#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "util/jobQueue.h"

#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

using namespace Tangram;


class Barrier {
private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::size_t m_count;

public:
    explicit Barrier(std::size_t count) : m_count(count) {}
    void wait() {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (--m_count == 0) {
            m_cond.notify_all();
        } else {
            m_cond.wait(lock, [this] { return m_count == 0; });
        }
    }
};


TEST_CASE("stress test JobQueue", "[JobQueue]") {

    JobQueue jobQueue;

    const int numThreads = 16;
    const int addJobRepeats = 100;
    const int runJobsRepeats = 100;

    Barrier allThreadsGoBarrier(numThreads);
    std::atomic<int> globalCounter{0};

    std::vector<std::unique_ptr<std::thread>> threads;

    for (int j = 0; j < numThreads; j++) {
        threads.emplace_back(std::make_unique<std::thread>([=, &jobQueue, &globalCounter, &allThreadsGoBarrier] {
            allThreadsGoBarrier.wait();

            for (int k = 0; k < runJobsRepeats; k++) {
                for (int i = 0; i < addJobRepeats; i++) {

                    jobQueue.add([&] { globalCounter++; });

                    if ((k + i) % 3 == 0) { std::this_thread::yield(); }
                }
                jobQueue.runJobs();
            }
        }));
    }

    for (auto& thread : threads) { thread->join(); }

    CHECK(globalCounter == (numThreads * runJobsRepeats * addJobRepeats));
}
