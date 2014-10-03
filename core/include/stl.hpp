/* Include a set of STL items
 *
 * Although it is typically not good practice to "use" namespace items, a few STL items are so frequently used in C++ that
 * it becomes more troublesome to prefix them in every usage. 
 */

#pragma once
#include "stl_util.hpp"

#include <string>
using std::string;

#include <memory>
using std::unique_ptr;
using std::shared_ptr;
using std::make_shared;
using std_patch::make_unique;
using std_patch::to_string;

#include <vector>
using std::vector;

#include <map>
using std::map;

#include <unordered_map>
using std::unordered_map;
