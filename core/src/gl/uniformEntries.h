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

/* Checks whether a uniform entry exists for a given uniform name */
bool entryExistsForName(const std::string& _name);

/* Generate a uniform entry (0 being a non valid EntryId) */
void genEntry(EntryId* _id, const std::string& _name);

/* Get a uniform entry for the given EntryId
 * Returns nullptr if the entry has not been found
 */
const UniformEntry* getEntry(EntryId _id);

/* Generates an entry only if the uniform entry with the given name _name
 * has not been found in the set of uniform entries
 */
void lazyGenEntry(EntryId* _id, const std::string& _name);

}

}
