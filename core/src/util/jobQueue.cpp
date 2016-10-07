#include "jobQueue.h"
#include "log.h"

namespace Tangram {

JobQueue::~JobQueue() {

    LOG("Destructing job queue");

    if (!m_jobs.empty()) {
        runJobs();
    }

    LOG("DONE Destructing job queue");
}

void JobQueue::add(Job job) {

    std::lock_guard<std::mutex> lock(m_mutex);
    m_jobs.push_back(job);

}

void JobQueue::runJobs() {

    for (size_t i = 0; i < m_jobs.size(); i++) {
        Job job;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            LOG("about to run %d job", i);
            job.swap(m_jobs[i]);
        }
        LOG("run %d job", i);
        job();
        LOG("done running %d job", i);
    }
    m_jobs.clear();

    LOG("DONE!");
}

} //namespace Tangram
