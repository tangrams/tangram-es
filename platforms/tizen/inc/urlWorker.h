#pragma once

#include "platform.h"

#include <memory>
#include <vector>
#include <condition_variable>
#include <thread>
#include <mutex>
#include <atomic>


struct UrlTask {
    UrlCallback callback;
    const std::string url;

    UrlTask(const std::string& _url, const UrlCallback& _callback) :
        callback(_callback),
        url(_url) {
    }
};

class UrlWorker {
public:
    void enqueue(std::unique_ptr<UrlTask> _task);

    void cancel(const std::string& _url);

    void stop();

    void start(int _numWorker, const char* _proxyAddress = "");

     ~UrlWorker();

private:

    std::mutex m_mutex;
    std::mutex m_mutexInitCurl;

    std::condition_variable m_condition;

    bool m_running;

    std::string m_proxyAddress;

    struct Thread {
        std::unique_ptr<std::thread> thread;
        std::string activeUrl;
        bool canceled;
    };

    void run(Thread* thread);

    std::vector<Thread> m_workers;

    std::vector<std::unique_ptr<UrlTask>> m_queue;

};
