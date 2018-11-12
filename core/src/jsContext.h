//
// Created by Matt Blair on 11/9/18.
//
#pragma once

#include <string>

namespace Tangram {

struct Feature;

namespace JsContext {

struct Instance;

Instance* create();

void destroy(Instance* instance);

void setGlobalString(Instance* instance, const std::string& name, const std::string& value);

void setGlobalNumber(Instance* instance, const std::string& name, double value);

void setCurrentFeature(Instance* instance, Feature* feature);

// Compiles the given JavaScript string into a function, adds it to the function list, and returns its index in the
// list. If the string could not be compiled, sets error to true.
uint32_t addFunction(Instance* instance, const std::string& source, bool& error);

bool evaluateBooleanFunction(Instance* instance, uint32_t index);

struct Value;

Value* getFunctionResult(Instance* instance, uint32_t index);

void releaseValue(Instance* instance, Value* value);

bool valueIsNull(Instance* instance, Value* value);
bool valueIsBool(Instance* instance, Value* value);
bool valueIsNumber(Instance* instance, Value* value);
bool valueIsString(Instance* instance, Value* value);
bool valueIsArray(Instance* instance, Value* value);
bool valueIsObject(Instance* instance, Value* value);
bool valueIsUndefined(Instance* instance, Value* value);

bool valueGetBool(Instance* instance, Value* value);
double valueGetDouble(Instance* instance, Value* value);
std::string valueGetString(Instance* instance, Value* value);
size_t valueGetArraySize(Instance* instance, Value* value);
Value* valueGetArrayElement(Instance* instance, Value* value, size_t index);
// Value* valueGetObjectElement(Instance* instance, Value* value, const std::string& key); // Not currently used.

} // namespace JsContext
} // namespace Tangram
