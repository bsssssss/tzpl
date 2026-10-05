#include "jsonl.hpp"
#include <expected>
#include <format>

namespace jsonl {

using StringResult = std::expected<std::string, std::string>;

class JsonParser {
  private:
    std::string const& input;
    size_t pos;

    bool at_end()     { return pos >= input.length(); }
    bool peek(char c) { return !at_end() && input[pos] == c; }

    void skip_whitespace() {
        while (pos < input.length() && std::isspace(input[pos])) {
            pos++;
        }
    }

    bool is_delim(char c) {
        return c == '}' || c == ',' || std::isspace(c);
    }

    JsonResult parse_symbol() {
        size_t start = pos;        
        while (!at_end() && !is_delim(input[pos])) { pos++; }

        std::string b_str = input.substr(start, pos - start);
        if (b_str == "true") {
            return Value(true);
        }
        else if (b_str == "false") {
            return Value(false);
        } 
        else if (b_str == "null") {
            return Value();
        } 
        else {
            return std::unexpected(
                std::format("invalid symbol {} at position {}",
                            b_str, start));
        } 
    }

    JsonResult parse_number() {
        size_t start = pos;
        bool has_digits = false;

        if (peek('-') || peek('+')) {
            pos++;
        }

        while (!at_end() && std::isdigit(input[pos])) {
            has_digits = true;
            pos++;
        }

        if (!has_digits) {
            return std::unexpected("invalid number at position " 
                                   + std::to_string(pos));
        }

        if (!at_end() && peek('.')) {
            has_digits = false;
            pos++;
            while (!at_end() && std::isdigit(input[pos])) {
                has_digits = true;
                pos++;
            }
            if (!has_digits) {
                return std::unexpected("invalid number at position " 
                                       + std::to_string(pos));
            }
        }
        
        std::string str = input.substr(start, pos - start);
        return Value(std::stod(str)); 
    }

    StringResult parse_string() {
        if (!peek('"')) {
            return std::unexpected("expected '\"' at position " 
                                   + std::to_string(pos)); 
        }
        pos++;
        std::string str;
        while (pos < input.length() && input[pos] != '"') {
            if (input[pos] == '\\') {
                pos++; // consume '\'
                switch (input[pos]) {
                    case 'n': str += '\n'; break;
                    case 'r': str += '\r'; break;
                    case 't': str += '\t'; break;
                    default : str += input[pos];
                }
            }
            else {
                str += input[pos]; 
            }
            pos++;
        }
        if (at_end()) {
            return std::unexpected("unterminated string"); 
        }
        pos++; // consume closing "
        return str;
    }

    JsonResult parse_object() {
        skip_whitespace();
        if (!peek('{')) {
            return std::unexpected(("expected '{' at position " 
                                   + std::to_string(pos))); 
        }
        size_t start = pos;
        Value::Object obj;
        pos++;

        skip_whitespace();
        // empty object
        if (peek('}')) {
            pos++;
            return Value(obj);
        }

        while (true) {
            skip_whitespace();
            // key
            auto key = parse_string();
            if (!key.has_value()) {
                return std::unexpected(key.error()); 
            }
            // colon
            skip_whitespace();
            if (!peek(':')) {
                return std::unexpected("expected ':' at position " 
                                       + std::to_string(pos));
            }
            pos++;
            // value
            auto value = parse_value();
            if (!value.has_value()) {
                return std::unexpected(value.error()); 
            }
            // append key value pair
            obj.insert_or_assign(std::move(*key), std::move(*value));
            skip_whitespace();
            // closing
            if (peek('}')) {
                pos++;
                return Value(obj);
            }
            // more ?
            if (!peek(',')) {
                return std::unexpected("unterminated object at position " 
                                       + std::to_string(start));
            }

            pos++;
            skip_whitespace();

            if (peek('}')) {
                return std::unexpected("trailing ',' at position " 
                                       + std::to_string(pos));
            }
        }
    }

    // [1, true, "false"]
    JsonResult parse_array() {
        if (!peek('[')) {
            return std::unexpected("expected '[' at position " 
                                   + std::to_string(pos)); 
        }
        size_t start = pos;

        pos++;
        Value::Array arr;

        skip_whitespace();
        if (peek(']')) {
            return Value(arr);
        }

        while (true) {
            skip_whitespace();
            auto r = parse_value();
            if (!r.has_value()) return std::unexpected(r.error());
            arr.push_back(*r);
            skip_whitespace();
            if (peek(']')) {
                pos++;
                return Value(arr);
            }
            if (!peek(',')) {
                return std::unexpected(
                    "Unterminated array at position " 
                    + std::to_string(start));
            }
            pos++;
        }
    }

    JsonResult parse_value() {
        skip_whitespace();
        if (at_end()) {
            return std::unexpected("unexpected end of input."); 
        }

        Value res;
        char c = input[pos];
        if (c == '{') {
            return parse_object(); 
        }
        else if (c == '[') {
            return parse_array(); 
        }
        else if (c == '"') {
            return parse_string().transform([](std::string const& s) { return Value(s); });
        }
        else if (std::isdigit(c) || c == '-' || c == '+') {
            return parse_number();
        }
        else {
            return parse_symbol();
        }
    }

  public:
    explicit JsonParser(std::string const& str) : input(str), pos(0) {};

    JsonResult parse() {
        pos = 0;
        JsonResult res = parse_value();
        return res;
    }
};

// TODO: parse error if 1st element is not a json object
JsonResult parse_json(std::string const& input) {
    JsonParser parser(input);
    return parser.parse();
}

template<class... Ts>
struct overloads : Ts... { using Ts::operator()...; };

std::string encode_json(Value const& v) {
    return std::visit(overloads {
        [](std::monostate) { return std::string("null"); },
        [](std::string const& s) { return std::format("\"{}\"", s); },
        [](double x) { return std::format("{}", x); },
        [](bool b) { return b ? std::string("true") : std::string("false"); },
        [](Value::Array const& a) {
            auto rem = a.size();
            std::string s = "[";
            for (auto i = 0; i < a.size(); i++) {  
                s += encode_json(a[i]);
                if (--rem > 0) {
                    s += ','; 
                }
            }
            s += ']';
            return s; 
        },
        [](Value::Object const& obj) {
            std::string s = "{";
            auto rem = obj.size();
            for (const auto& pair : obj) {
                s += std::format("\"{}\"", pair.first) + ":";
                s += encode_json(pair.second);
                if (--rem > 0) { s += ','; }
            }
            s += '}';
            return s; 
        },
    }, v.value);
}

} // namespace jsonl
