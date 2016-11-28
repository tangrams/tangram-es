#include "util/node.h"
#include "util/y2j.h"
#include "csscolorparser.hpp"

namespace Tangram {

Node::SequenceIterator Node::Sequence::begin() const {
    return value ? { value->Begin() } : { nullptr };
}

Node::SequenceIterator Node::Sequence::end() const {
    return value ? { value->End() } : { nullptr };
}

Node::MappingIterator Node::Mapping::begin() const {
    return value ? value->MemberBegin() : nullptr;
}

Node::MappingIterator Node::Mapping::end() const {
    return value ? value->MemberEnd() : nullptr;
}

Node::Node() {
}

Node::Node(JsonValue* val) :
    value(val) {
}

Node::Node(const char* str) {
    // TODO
}

Node::Node(double num) {
    // TODO
}

Node::Node(int64_t num) {
    // TODO
}

explicit Node::operator bool() const {
    return (value != nullptr);
}

bool Node::isNull() const {
    return value->IsNull();
}

bool Node::isFalse() const {
    return value->IsFalse();
}

bool Node::isTrue() const {
    return value->IsTrue();
}

bool Node::isBool() const {
    return value->IsBool();
}

bool Node::isMapping() const {
    return value->IsObject();
}

bool Node::isSequence() const {
    return value->IsArray();
}

bool Node::isNumber() const {
    return value->IsNumber();
}

bool Node::isInt() const {
    return value->IsInt64();
}

bool Node::isDouble() const {
    return value->IsDouble();
}

bool Node::isString() const {
    return value->IsString();
}

// Bool.

bool Node::getBool() const {
    return value->GetBool();
}

// Mapping.

size_t Node::getMappingCount() const {
    if (value && value->IsObject()) {
        return value->MemberCount();
    }
    return 0;
}

const Mapping Node::getMapping() const {
    return { value };
}

Mapping Node::getMapping() {
    return { value };
}

const Node Node::operator[](const char* name) const {
    if (value && value->IsObject()) {
        auto it = value->FindMember(name);
        if (it != value->MemberEnd()) {
            return Node(&(it->value));
        }
    }
    return Node();
}

const Node Node::operator[](const std::string& name) const {
    if (value && value->IsObject()) {
        auto it = value->FindMember(name.c_str());
        if (it != value->MemberEnd()) {
            return Node(&(it->value));
        }
    }
    return Node();
}

Node Node::operator[](const char* name) {
    if (value && value->IsObject()) {
        auto it = value->FindMember(name);
        if (it != value->MemberEnd()) {
            return Node(&(it->value));
        }
    }
    return Node();
}

Node Node::operator[](const std::string name) {
    if (value && value->IsObject()) {
        auto it = value->FindMember(name.c_str());
        if (it != value->MemberEnd()) {
            return Node(&(it->value));
        }
    }
    return Node();
}

// Sequence.

size_t Node::getSequenceCount() const {
    if (value && value->IsArray()) {
        return value->Size();
    }
    return 0;
}

const Sequence Node::getSequence() const {
    return { value };
}

Sequence Node::getSequence() {
    return { value };
}

const Node Node::operator[](size_t index) const {
    if (value && value->IsArray()) {
        return Node(&(*value)[index]);
    }
    return Node();
}

Node Node::operator[](size_t index) {
    if (value && value->IsArray() && index) {
        return Node(&(*value)[index]);
    }
    return Node();
}

// Number.

int64_t Node::getInt() const {
    return value->GetInt64();
}

int64_t Node::getIntOr(int64_t fallback) const {
    if (value->IsInt64()) {
        return value->GetInt64();
    }
    return fallback;
}

float Node::getFloat() const {
    return value->GetFloat();
}

float Node::getFloatOr(float fallback) const {
    return value->IsNumber() ? value->GetFloat() : fallback;
}
double Node::getDouble() const {
    return value->GetDouble();
}

double Node::getDoubleOr(double fallback) const {
    return value->IsNumber() ? value->GetDouble() : fallback;
}

// String.

const char* Node::getString() const {
    return value->GetString();
}

const char* Node::getStringOr(const char* fallback) const {
    return value->IsString() ? value->GetString() : fallback;
}

size_t Node::getStringLength() const {
    return value->GetStringLength();
}

JsonValue* Node::getValue() const {
    return value;
}

glm::vec4 getColorAsVec4(const Node& node) {
    if (node.isDouble()) {
        double val = node.getDouble();
        return glm::vec4(val, val, val, 1.0);
    }
    if (node.isSequence()) {
        glm::vec4 result;
        for (size_t i = 0; i < 4 && i < node.getSequenceCount(); i++) {
            result[i] = node[i].getFloatOr(0.f);
        }
        return result;
    }
    if (node.isString()) {
        auto c = CSSColorParser::parse(node.getString());
        return glm::vec4(c.r / 255.f, c.g / 255.f, c.b / 255.f, c.a);
    }
    return glm::vec4();
}

std::string parseSequence(const Node& node) {
    std::string result;
    for (const auto& val : node.getSequence()) {
        if (val.isDouble()) {
            result += std::to_string(val.getDouble());
        } else if (val.isString()) {
            result += val.getString();
        }
        result.push_back(',');
    }
    return result;
}

} // namespace Tangram
