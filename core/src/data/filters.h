#pragma once

#include "tileData.h"
#include <unordered_map>
#include <vector>
#include <limits>

namespace Tangram {

    struct Value {

        union {
            std::string str;
            float num;
        };

        virtual bool equals(float f) const = 0;
        virtual bool equals(const std::string& s) const = 0;
        virtual bool equals(const Value& v) const = 0;

        Value(std::string s) : str(s) {}
        Value(float val) : num(val) {}

        virtual ~Value() {}

    };

    struct NumValue : Value {

        NumValue(float val) : Value(val) {}

        virtual bool equals(const std::string& s) const override { return false; }
        virtual bool equals(float f) const override { return num == f; }
        virtual bool equals(const Value& v) const override { return v.equals(num); }

        virtual ~NumValue() {}

    };

    struct StrValue : Value {

        StrValue(std::string s) : Value(s) {}

        virtual bool equals(const std::string& s) const override { return str == s; }
        virtual bool equals(float f) const override { return false; }
        virtual bool equals(const Value& v) const override { return v.equals(str); }

        virtual ~StrValue() {
            // Need to call std::string destructor explicitly, however this is a bug in clang implementation and works in
            // gcc (refer https://llvm.org/bugs/show_bug.cgi?id=12350)
            // str.std::string::~string();
        }

    };

    using ValueList = std::vector<Value*>;

    using Context = std::unordered_map<std::string, Value*>;

    struct Filter {

        virtual bool eval(const Feature& f, const Context& c) const = 0;

    };

    struct Operator : public Filter {

        std::vector<Filter*> operands;

    };

    struct Any : public Operator {

        virtual bool eval(const Feature& feat, const Context& ctx) const override {
            for (const Filter* filt : operands) {
                if (filt->eval(feat, ctx)) { return true; }
            }
            return false;
        }

    };

    struct All : public Operator {

        virtual bool eval(const Feature& feat, const Context& ctx) const override {
            for (const Filter* filt : operands) {
                if (!filt->eval(feat, ctx)) { return false; }
            }
            return true;
        }

    };

    struct None : public Operator {

        virtual bool eval(const Feature& feat, const Context& ctx) const override {
            for (const Filter* filt : operands) {
                if (filt->eval(feat, ctx)) { return false; }
            }
            return true;
        }

    };

    struct Predicate : public Filter {

        std::string key;

    };

    struct Equality : public Predicate {

        ValueList values;

        virtual bool eval(const Feature& feat, const Context& ctx) const override {

            auto ctxIt = ctx.find(key);
            if (ctxIt != ctx.end()) {
                for (auto* v : values) {
                    if (v->equals(*ctxIt->second)) { return true; }
                }
                return false;
            }
            auto strIt = feat.props.stringProps.find(key);
            if (strIt != feat.props.stringProps.end()) {
                for (auto* v : values) {
                    if (v->equals(strIt->second)) { return true; }
                }
            }
            auto numIt = feat.props.numericProps.find(key);
            if (numIt != feat.props.numericProps.end()) {
                for (auto* v : values) {
                    if (v->equals(numIt->second)) { return true; }
                }
            }
            return false;
        }

    };

    struct Range : public Predicate {

        float min = -std::numeric_limits<float>::infinity();
        float max = +std::numeric_limits<float>::infinity();

        virtual bool eval(const Feature& feat, const Context& ctx) const override {

            auto ctxIt = ctx.find(key);
            if (ctxIt != ctx.end()) {
                const auto& val = *ctxIt->second;
                if (!val.equals(val.num)) { return false; } // only check range for numbers
                return val.num >= min && val.num < max;
            }
            auto numIt = feat.props.numericProps.find(key);
            if (numIt != feat.props.numericProps.end()) {
                const auto& num = numIt->second;
                return num >= min && num < max;
            }
            return false;
        }

    };
}
