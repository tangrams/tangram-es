#pragma once

#include <functional>
#include <mutex>
#include <vector>

namespace Tangram {

// JobQueue allows you to queue a sequence of jobs to run later.
// This is useful for OpenGL resources that must be created and destroyed on the GL thread.

class JobQueue {

public:
    using Job = std::function<void()>;

    JobQueue() = default;

    // Any jobs left in the queue will be run in the destructor. This is thread-safe.
    ~JobQueue();

    // Put a job on the queue. This is thread-safe.
    void add(Job job);

    // Run all jobs on the queue in the order they were added, then remove them. This is thread-safe.
    void runJobs();

    void stop() {
        m_stopped = true;
        runJobs();
    }
private:

    std::vector<Job> m_jobs;
    std::mutex m_mutex;
    bool m_stopped = false;
};

}
