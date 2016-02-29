#include "uniformEntries.h"
#include "platform.h"
#include <vector>

namespace Tangram {

namespace UniformEntries {

std::vector<std::unique_ptr<UniformEntry>> entries;
EntryId ids = 0;

bool entryExistsForName(const std::string& _name) {
    for (const auto& entry : entries) {
        if (entry->name == _name) {
            return true;
        }
    }
    return false;
}

void genEntry(EntryId* _id, const std::string& _name) {
    *_id = ++ids;

    if (entryExistsForName(_name)) {
        LOGW("Uniform entry with name %s already exists", _name.c_str());
    }

    std::unique_ptr<UniformEntry> entry = std::unique_ptr<UniformEntry>(new UniformEntry{*_id, _name});
    entries.push_back(std::move(entry));
}

const UniformEntry* getEntry(EntryId _id) {
    if (_id > entries.size()) {
        LOGW("Accessing out of bound entry id");
        return nullptr;
    }

    return entries[_id - 1].get();
}

void lazyGenEntry(EntryId* _id, const std::string& _name) {
    if (!entryExistsForName(_name)) {
        genEntry(_id, _name);
    }
}

}

}
