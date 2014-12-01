#include <stdio.h>
#include <stdarg.h>
#include <iostream>
#include <fstream>
#include <string>

#include "platform.h"

void logMsg(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

std::string stringFromResource(const char* _path) {
    std::string into;

    std::ifstream file;
    std::string buffer;

    file.open(_path);
    if(!file.is_open()) return false;
    while(!file.eof()) {
        getline(file, buffer);
        into += buffer + "\n";
    }

    file.close();
    return into;
}
