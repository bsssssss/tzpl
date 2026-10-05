#ifndef jsonl_hpp
#define jsonl_hpp

#include <expected>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace jsonl {

struct Value {
    using Array = std::vector<Value>;
    using Object = std::unordered_map<std::string, Value>;

    std::variant<std::monostate, std::string, double, bool, Array, Object>
        value;

    Value() = default; // monostate
    Value(bool b) : value(b) {}

    Value(const char* s) : value(std::string(s)) {}
    Value(std::string s) : value(std::move(s)) {}

    // only double
    template <std::floating_point T>
    Value(T n) : value(double(n)) {}
    template <std::integral T>
    Value(T n) : value(double(n)) {}

    Value(Object obj) : value(std::move(obj)) {}
    Value(Array arr) : value(std::move(arr)) {}

    template<typename T> 
    bool is() const {
        return std::holds_alternative<T>(value);
    }

    template<typename T> 
    T const& get() const {
        return std::get<T>(value);
    }

    bool operator==(Value const& that) const {
        return value == that.value;
    }
};

using JsonResult = std::expected<Value, std::string>;

JsonResult parse_json(std::string const& input);
std::string encode_json(Value const& v);

} // namespace jsonl

#endif /* jsonl_hpp */
