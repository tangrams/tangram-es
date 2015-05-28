#pragma once

#include "tileData.h"
#include <unordered_map>
#include <vector>
#include <limits>

namespace Tangram {

    struct Value {

        float num;
        std::string str;

        virtual bool equals(float f) const = 0;
        virtual bool equals(const std::string& s) const = 0;
        virtual bool equals(const Value& v) const = 0;

        virtual ~Value() {};

        Value(float n) : num(n) {}
        Value(const std::string& s) : str(s) {}

    };

    struct NumValue : public Value {

        NumValue(float n) : Value(n) {}
        ~NumValue() {}

        // We don't like it! But there is no other way to check for an explicit string value in yaml-cpp
        virtual bool equals(const std::string& s) const override {
            try {
                float f = std::stof(s);
                if( (num-f) < 0.00001) {
                    return true;
                } else {
                    return false;
                }
            } catch(const std::invalid_argument& e) {
                return false;
            }
        }
        virtual bool equals(float f) const override { return num == f; }
        virtual bool equals(const Value& v) const override { return v.equals(num); }

    };

    struct StrValue : public Value {

        StrValue(const std::string& s) : Value(s) {}
        ~StrValue() {}

        virtual bool equals(const std::string& s) const override { return str == s; }
        virtual bool equals(float f) const override { return false; }
        virtual bool equals(const Value& v) const override { return v.equals(str); }

    };

    using ValueList = std::vector<Value*>;

    using Context = std::unordered_map<std::string, Value*>;

    struct Filter {

        virtual bool eval(const Feature& f, const Context& c) const = 0;
        virtual ~Filter() {};

    };

    struct Operator : public Filter {

        std::vector<Filter*> operands;

        Operator(const std::vector<Filter*>& ops) : operands(ops) {}
        ~Operator() { for (auto* f : operands) { delete f; } }

    };

    struct Any : public Operator {

        Any(const std::vector<Filter*>& ops) : Operator(ops) {}
        virtual bool eval(const Feature& feat, const Context& ctx) const override {
            for (const Filter* filt : operands) {
                if (filt->eval(feat, ctx)) { return true; }
            }
            return false;
        }

    };

    struct All : public Operator {

        All(const std::vector<Filter*>& ops) : Operator(ops) {}
        virtual bool eval(const Feature& feat, const Context& ctx) const override {
            for (const Filter* filt : operands) {
                if (!filt->eval(feat, ctx)) { return false; }
            }
            return true;
        }

    };

    struct None : public Operator {

        None(const std::vector<Filter*>& ops) : Operator(ops) {}
        virtual bool eval(const Feature& feat, const Context& ctx) const override {
            for (const Filter* filt : operands) {
                if (filt->eval(feat, ctx)) { return false; }
            }
            return true;
        }

    };

    struct Predicate : public Filter {

        std::string key;

        Predicate(const std::string& k) : key(k) {}
        virtual ~Predicate() {}

    };

    struct Equality : public Predicate {

        ValueList values;

        Equality(const std::string& k, const ValueList& v) : Predicate(k), values(v) {}
        ~Equality() { for (auto* v : values) { delete v; } }

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

        Range(const std::string& k, float mn, float mx) : Predicate(k), min (mn), max(mx) {}

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
