#include "util/jobQueue.h"

namespace Tangram {

JobQueue::~JobQueue() {

    if (!m_jobs.empty()) { runJobs(); }
}

void JobQueue::add(Job job) {

    if (!m_stopped) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push_back(std::move(job));
    } else {
        job();
    }
}

void JobQueue::runJobs() {
    std::vector<Job> localJobs;

    {
        // steal contents of m_jobs inside the lock
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.swap(localJobs);
        // now the lock can be released as we won't touch m_jobs anymore
    }

    // execute jobs outside of the lock
    for (auto& jobref : localJobs) {
        Job job = std::move(jobref);
        job();
        // job dtor triggers here
    }


    // try to give back memory to m_jobs
    if (!localJobs.empty()) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_jobs.empty() && m_jobs.capacity() < localJobs.capacity()) {
            // clear does not release capacity/memory
            localJobs.clear();
            m_jobs.swap(localJobs);
        }
    }
}

} // namespace Tangram
