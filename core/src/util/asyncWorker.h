#pragma once

#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace Tangram {

class AsyncWorker {
public:

    AsyncWorker() {
        thread = std::thread(&AsyncWorker::run, this);
    }

    ~AsyncWorker() {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_running = false;
        }
        m_condition.notify_all();
        thread.join();
    }

    void enqueue(std::function<void()> _task) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_running) { return; }

            m_queue.push_back(std::move(_task));
        }
        m_condition.notify_one();
    }

    void waitForCompletion() {
        m_waitForCompletion = true;
    }
private:

    void run() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [&]{ return !m_running || !m_queue.empty(); });

                if (!m_running) {
                    if (!m_waitForCompletion) {
                        break;
                    } else if (m_queue.empty()) {
                        break;
                    }
                }

                task = std::move(m_queue.front());
                m_queue.pop_front();
            }
            task();
        }
    }

    std::thread thread;
    std::atomic<bool> m_running {true};
    std::atomic<bool> m_waitForCompletion {false};
    std::condition_variable m_condition;
    std::mutex m_mutex;
    std::deque<std::function<void()>> m_queue;
};

}
