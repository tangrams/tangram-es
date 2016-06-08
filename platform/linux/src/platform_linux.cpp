#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>
#include <list>

#include "urlWorker.h"

#include <libgen.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#include "platform_linux.h"

PFNGLBINDVERTEXARRAYPROC glBindVertexArrayOESEXT = 0;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArraysOESEXT = 0;
PFNGLGENVERTEXARRAYSPROC glGenVertexArraysOESEXT = 0;

static bool s_isContinuousRendering = false;
static std::string s_resourceRoot;

#include "platform_common.h"

void requestRender() {
    glfwPostEmptyEvent();
}

void setContinuousRendering(bool _isContinuous) {
    s_isContinuousRendering = _isContinuous;
}

bool isContinuousRendering() {
    return s_isContinuousRendering;
}

void setCurrentThreadPriority(int priority){
    int tid = syscall(SYS_gettid);
    //int  p1 = getpriority(PRIO_PROCESS, tid);

    setpriority(PRIO_PROCESS, tid, priority);

    //int  p2 = getpriority(PRIO_PROCESS, tid);
    //logMsg("set niceness: %d -> %d\n", p1, p2);
}

void finishUrlRequests() {
    for(auto& worker : s_Workers) {
        worker.join();
    }
}

void initGLExtensions() {
     glBindVertexArrayOESEXT = (PFNGLBINDVERTEXARRAYPROC)glfwGetProcAddress("glBindVertexArray");
     glDeleteVertexArraysOESEXT = (PFNGLDELETEVERTEXARRAYSPROC)glfwGetProcAddress("glDeleteVertexArrays");
     glGenVertexArraysOESEXT = (PFNGLGENVERTEXARRAYSPROC)glfwGetProcAddress("glGenVertexArrays");
}

