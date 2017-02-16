#include "util/jobQueue.h"

namespace Tangram {

JobQueue::~JobQueue() {

    if (!m_jobs.empty()) {
        runJobs();
    }
}

void JobQueue::add(Job job) {

    if (!m_stopped) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_jobs.push_back(job);
    } else {
        job();
    }
}

void JobQueue::runJobs() {

    for (size_t i = 0; i < m_jobs.size(); i++) {
        Job job;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            job.swap(m_jobs[i]);
        }
        job();
    }
    m_jobs.clear();

}

} //namespace Tangram
