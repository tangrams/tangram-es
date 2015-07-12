#pragma once

// Based on example in https://channel9.msdn.com/Shows/Going+Deep/C-and-Beyond-2012-Herb-Sutter-Concurrency-and-Parallelism

#include <mutex>

namespace util {

template<class T> class monitor {
private:
    mutable T t;
    mutable std::mutex m;

public:
    // Add default constructor when monitored type is default constructible
    monitor(typename std::enable_if<std::is_default_constructible<T>::value>::type* = 0) {}

    monitor(T t_) : t{t_} {}
    //monitor( T&& t_ ) : t{std::move(t_)} {}

    template<typename F>
    auto operator()(F f) const -> decltype(f(t)) {
        std::lock_guard<std::mutex> lock(m);
        return f(t);
    }
};
};
