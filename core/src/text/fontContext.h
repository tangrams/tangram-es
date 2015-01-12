#pragma once

#include "glfontstash.h"
#include <memory>
#include <mutex>

struct FontContext {
    std::unique_ptr<std::mutex> m_contextMutex;
    FONScontext* m_fsContext;
};
