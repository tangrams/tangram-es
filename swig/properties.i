%include "std_shared_ptr.i"
%include "std_string.i"

%shared_ptr(Tangram::Properties);

// TODO One getter returning both string and numbers, iterator for values
namespace Tangram {
struct Properties {
    Properties() {}
    void clear() { props.clear(); }
    bool contains(const std::string& key) const;
    double getNumber(const std::string& key) const;
    const std::string& getString(const std::string& key) const;
    void set(std::string key, std::string value);
    void set(std::string key, double value);
};
}
