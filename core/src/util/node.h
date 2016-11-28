#pragma once

#include "glm/vec4.hpp"
#include "util/json.h"
#include <string>

namespace Tangram {

template<typename NodeType, typename PtrType>
struct SequenceIterator {
    bool operator==(const SequenceIterator& other) { return value == other.value; }
    bool operator!=(const SequenceIterator& other) { return value != other.value; }
    SequenceIterator& operator++() { value++; return *this; }
    SequenceIterator& operator--() { value--; return *this; }
    SequenceIterator operator++(int) { return SequenceIterator{ value++ }; }
    SequenceIterator operator--(int) { return SequenceIterator{ value-- }; }
    NodeType operator*() const { return NodeType(value); }
    PtrType* value = nullptr;
};

template<typename NodeType, typename PtrType>
struct Sequence {
    SequenceIterator<NodeType, PtrType> begin() const;
    SequenceIterator<NodeType, PtrType> end() const;
    PtrType* value = nullptr;
};

template<typename NodeType>
struct MappingMember {
    NodeType name;
    NodeType value;
};

template<typename NodeType, typename PtrType>
struct MappingIterator {
    bool operator==(const MappingIterator& other) { return value == other.value; }
    bool operator!=(const MappingIterator& other) { return value != other.value; }
    MappingIterator& operator++() { value++; return *this; }
    MappingIterator& operator--() { value--; return *this; }
    MappingIterator operator++(int) { return MappingIterator{ value++ }; }
    MappingIterator operator--(int) { return MappingIterator{ value-- }; }
    MappingMember<NodeType> operator*() const { return { NodeType(value), NodeType(value + 1) }; }
    PtrType* value = nullptr;
};

template<typename NodeType, typename PtrType>
struct Mapping {
    MappingIterator<NodeType, PtrType> begin() const;
    MappingIterator<NodeType, PtrType> end() const;
    PtrType* value = nullptr;
};

class Node {

public:

    using SequenceIterator = Sequence<Node, JsonValue>;
    using Sequence = Sequence<Node, JsonValue>;
    using MappingMember = MappingMember<Node>;
    using MappingIterator = MappingIterator<Node, JsonValue>;
    using Mapping = Mapping<Node, JsonValue>;

    Node();
    Node(JsonValue* val);
    Node(const char* str);
    Node(double num);
    Node(int64_t num);

    explicit operator bool() const;

    bool isNull() const;
    bool isFalse() const;
    bool isTrue() const;
    bool isBool() const;
    bool isMapping() const;
    bool isSequence() const;
    bool isNumber() const;
    bool isInt() const;
    bool isDouble() const;
    bool isString() const;

    // Bool.

    bool getBool() const;

    // Mapping.

    size_t getMappingCount() const;

    const Mapping getMapping() const;
    Mapping getMapping();

    const Node operator[](const char* name) const;
    const Node operator[](const std::string& name) const;
    Node operator[](const char* name);
    Node operator[](const std::string name);

    // Sequence.

    size_t getSequenceCount() const;

    const Sequence getSequence() const;
    Sequence getSequence();

    const Node operator[](size_t index) const;
    Node operator[](size_t index);

    // Number.

    int64_t getInt() const;
    int64_t getIntOr(int64_t fallback) const;
    float getFloat() const;
    float getFloatOr(float fallback) const;
    double getDouble() const;
    double getDoubleOr(double fallback) const;

    // String.

    const char* getString() const;
    const char* getStringOr(const char* fallback) const;
    size_t getStringLength() const;

    // Raw value.

    JsonValue* getValue() const;

private:
    JsonValue* value = nullptr;
};

glm::vec4 getColorAsVec4(const Node& node);

std::string parseSequence(const Node& node);

}
