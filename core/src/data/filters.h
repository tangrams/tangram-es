#pragma once

#include "tileData.h"
#include <unordered_map>
#include <vector>
#include <limits>

namespace Tangram {

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
                         feat.props.find(key) != feat.props.end();

            return exists == found;
        }

    };

    struct Equality : public Predicate {

        ValueList values;

        Equality(const std::string& k, const ValueList v) : Predicate(k), values(std::move(v)) {}

        virtual bool eval(const Feature& feat, const Context& ctx) const override {
            auto ctxIt = ctx.find(key);
            if (ctxIt != ctx.end()) {
                for (auto& v : values) {
                    if (v == ctxIt->second) { return true; }
                }
                return false;
            }
            auto strIt = feat.props.find(key);
            if (strIt != feat.props.end()) {
                for (auto& v : values) {
                    if (v == strIt->second) { return true; }
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
                if (ctxIt->second.is<float>()) {
                    float val = ctxIt->second.get<float>();
                    return val >= min && val < max;
                }
            }
            auto numIt = feat.props.find(key);
            if (numIt != feat.props.end()) {
                if (numIt->second.is<float>()) {
                    float num = numIt->second.get<float>();
                    return num >= min && num < max;
                }
            }
            return false;
        }

    };
}
