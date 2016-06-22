#include "jobQueue.h"
#include <mutex>
#include <unordered_map>

namespace Tangram {

std::unordered_map<JobQueue::ThreadId, std::vector<JobQueue::Job>> jobsForThreads;
std::mutex jobMutex;

void JobQueue::makeCurrentThreadTarget() {
    m_threadId = std::this_thread::get_id();
}

void JobQueue::add(Job job) {
    std::lock_guard<std::mutex> lock(jobMutex);
    jobsForThreads[m_threadId].push_back(job);
}

void JobQueue::runJobsForCurrentThread() {
    auto currentThreadId = std::this_thread::get_id();

    const auto& entry = jobsForThreads.find(currentThreadId);
    if (entry == jobsForThreads.end()) {
        // Nothing to do here.
        return;
    }

    auto& jobs = entry->second;
    while (!jobs.empty()) {
        Job job;
        {
            std::lock_guard<std::mutex> lock(jobMutex);
            job.swap(jobs.back());
            jobs.pop_back();
        }
        job();
    }

}

} //namespace Tangram
