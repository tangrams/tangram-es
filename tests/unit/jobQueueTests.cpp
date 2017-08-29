#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "util/jobQueue.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <memory>

using namespace Tangram;


class Barrier {
private:
    std::mutex m_mutex;
    std::condition_variable m_cond;
    std::size_t m_count;
public:
    explicit Barrier(std::size_t count) : m_count(count) { }
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

    const size_t numThreads = 16;
    Barrier allThreadsGoBarrier(numThreads);

    std::vector<std::unique_ptr<std::thread>> threads;

    for(size_t i=0; i<numThreads; i++) {
        threads.emplace_back(std::make_unique<std::thread>([&]{
            allThreadsGoBarrier.wait();

            for(int k=0; k<100; k++) {
                for(int i=0; i<100; i++) {
                    jobQueue.add([]{});
                    if(i%3 == 0) {
                        std::this_thread::yield();
                    }
                }
                jobQueue.runJobs();
            }
        }));
    }

    for(auto &thread: threads) {
        thread->join();
    }
}

