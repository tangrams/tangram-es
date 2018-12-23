#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <jni.h>

namespace Tangram {

class JniWorker {
public:

    explicit JniWorker(JavaVM* _jvm) : jvm(_jvm){

        thread = std::thread(&JniWorker::run, this);

        pthread_setname_np(thread.native_handle(), "TangramJNI Worker");
    }

    ~JniWorker() {
       stop();
    }

    void enqueue(std::function<void(JNIEnv *jniEnv)> _task) {
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            if (!m_running) { return; }

            m_queue.push_back(std::move(_task));
        }
        m_condition.notify_one();
    }

    void stop() {
        if (!m_running) { return; }
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_running = false;
        }
        m_condition.notify_all();
        thread.join();
    }

private:

    void run() {
        jvm->AttachCurrentThread(&jniEnv, NULL);

        while (true) {
            std::function<void(JNIEnv *jniEnv)> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_condition.wait(lock, [&]{ return !m_running || !m_queue.empty(); });
                if (!m_running && m_queue.empty()) { break; }

                task = std::move(m_queue.front());
                m_queue.pop_front();
            }
            task(jniEnv);
        }
        jvm->DetachCurrentThread();
    }

    std::thread thread;
    bool m_running = true;
    std::condition_variable m_condition;
    std::mutex m_mutex;
    std::deque<std::function<void(JNIEnv *)>> m_queue;

    JavaVM* jvm;
    JNIEnv *jniEnv;

};

}
