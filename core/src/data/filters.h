#pragma once

#include "tileData.h"
#include <unordered_map>
#include <vector>
#include <limits>

namespace Tangram {

    struct Value {

        std::string str;
        float num;
        bool numeric;

        bool equals(float f) const { return numeric && num == f; }
        bool equals(const std::string& s) const { return str.size() != 0 && str == s; }
        bool equals(const Value& v) const { return (numeric && v.equals(num)) || v.equals(str); }

        Value() : num(0), numeric(false) {}
        Value(float n) : num(n), numeric(true) {}
        Value(float n, const std::string& s) : str(s), num(n), numeric(true) {}
        Value(const std::string& s) : str(s), num(0), numeric(false) {}

    };

    using ValueList = std::vector<Value>;

    using Context = std::unordered_map<std::string, Value>;

    struct Filter {

        virtual bool eval(const Feature& f, const Context& c) const { return false; };
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

    struct Existence : public Predicate {

        bool exists;

        Existence(const std::string& k, bool e) : Predicate(k), exists(e) {}

        virtual bool eval(const Feature& feat, const Context& ctx) const override {

            bool found = ctx.find(key) != ctx.end() ||
                         feat.props.stringProps.find(key) != feat.props.stringProps.end() ||
                         feat.props.numericProps.find(key) != feat.props.numericProps.end();

            return exists == found;
        }

    };

    struct Equality : public Predicate {

        ValueList values;

        Equality(const std::string& k, const ValueList& v) : Predicate(k), values(v) {}

        virtual bool eval(const Feature& feat, const Context& ctx) const override {

            auto ctxIt = ctx.find(key);
            if (ctxIt != ctx.end()) {
                for (auto& v : values) {
                    if (v.equals(ctxIt->second)) { return true; }
                }
                return false;
            }
            auto strIt = feat.props.stringProps.find(key);
            if (strIt != feat.props.stringProps.end()) {
                for (auto& v : values) {
                    if (v.equals(strIt->second)) { return true; }
                }
            }
            auto numIt = feat.props.numericProps.find(key);
            if (numIt != feat.props.numericProps.end()) {
                for (auto& v : values) {
                    if (v.equals(numIt->second)) { return true; }
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
                const auto& val = ctxIt->second;
                if (!val.numeric) { return false; } // only check range for numbers
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
