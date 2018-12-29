#include "util/yamlPath.h"

#include <cmath>

#define MAP_DELIM '.'
#define SEQ_DELIM '#'

namespace Tangram {

using Node = YAML::Node;

bool YamlPath::get(const Node& root, const std::string& path, Node& out) {
    if (!root) { return false; }

    size_t pathSize = path.size();
    out.reset(root);

    // First token can be either map or seqence
    char delimiter = '-';
    std::string key;
    size_t endToken = 0;

    while (endToken < pathSize) {
        size_t beginToken = endToken;
        endToken = pathSize;
        endToken = std::min(endToken, path.find(SEQ_DELIM, beginToken));
        endToken = std::min(endToken, path.find(MAP_DELIM, beginToken));

        if (out.IsMap() && (delimiter == MAP_DELIM || beginToken == 0)) {
            key = path.substr(beginToken, endToken - beginToken);
            out.reset(out[key]);
            // Get next character as the delimiter.
            delimiter = path[endToken];
            // Move past the delimiter.
            ++endToken;
            continue;

        } else if (out.IsSequence() && (delimiter == SEQ_DELIM ||
                                        beginToken == 0)) {
            size_t end = 0;
            int index = std::stoi(&path[beginToken], &end);
            Node n = out[index];
            if (beginToken + end == endToken) {
                out.reset(n);
                delimiter = path[endToken];
                ++endToken;
                continue;
            }
        }
        return false;
    }
    return true;
}

}
