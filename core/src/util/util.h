#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace Tangram {

inline std::vector<std::string> splitString(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }

    return elems;
}

template <class M, class T>
inline bool tryFind(M& map, const std::string& key, T& out) {
    auto it = map.find(key);

    if (it != map.end()) {
        out = it->second;
        return true;
    }

    return false;
}

}
