#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace Tangram {

namespace UniformEntries {

typedef uint16_t EntryId;

struct UniformEntry {
    EntryId id;
    std::string name;
};

bool entryExistsForName(const std::string& _name);

void genEntry(EntryId* _id, const std::string& _name);

const UniformEntry* getEntry(EntryId _id);

void lazyGenEntry(EntryId* _id, const std::string& _name);

}

}
