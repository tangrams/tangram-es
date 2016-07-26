#include "jobQueue.h"

namespace Tangram {

JobQueue::~JobQueue() {

    if (!m_jobs.empty()) {
        runJobs();
    }

}

void JobQueue::add(Job job) {

    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.push_back(job);

}

void JobQueue::runJobs() {

    auto it = m_jobs.begin();
    auto end = m_jobs.end();
    while (it != end) {
        Job job;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            job.swap(*it);
            ++it;
        }
        job();
    }
    m_jobs.clear();

}

} //namespace Tangram
