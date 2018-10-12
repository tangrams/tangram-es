#include "catch.hpp"
#include "glm/vector_relational.hpp"
#include "glm/gtc/epsilon.hpp"
#include "glm/gtx/io.hpp"
#include <sstream>

template<typename V>
class IsApproxEqualToVec : public Catch::MatcherBase<V> {
    using T = typename V::value_type;
    V target;
    T epsilon;

public:
    IsApproxEqualToVec(V t, T e) : target(t), epsilon(e) {}

    virtual bool match(const V& v) const override {
        return glm::all(glm::epsilonEqual(v, target, epsilon));
    }

    virtual std::string describe() const override {
        std::ostringstream ss;
        ss << "is equal to " << target << " within " << epsilon;
        return ss.str();
    }
};
