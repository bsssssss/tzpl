#include "jsonl.hpp"
#include <print>
#include <string>

using Obj = jsonl::Value::Object;
using Arr = jsonl::Value::Array;

static int gPassed = 0, gFailed = 0;

static void check(bool cond, std::string_view what) {
    if (cond) { std::print("  PASS: {}\n", what); ++gPassed; }
    else      { std::print("  FAIL: {}\n", what); ++gFailed; }
}

static std::string desc(jsonl::Value v) {
    if (v.is<std::string>())               { return "string"; }
    else if (v.is<double>())               { return "number"; }
    else if (v.is<bool>())                 { return "boolean"; }
    else if (v.is<jsonl::Value::Array>())  { return "array"; }
    else if (v.is<jsonl::Value::Object>()) { return "object"; }
    else                                   { return "null"; }
}

/*
 * Parse
 */

static void expect_parse(std::string input, jsonl::Value expected) {
    auto r = jsonl::parse_json(input);
    if (!r.has_value()) {
        check(false, std::format("{} [PARSE ERROR]: {}", input, r.error()));
        return;
    }
    check(*r == expected, std::format("{} == {}({})", input, desc(expected), jsonl::encode_json(expected)));
}

static void expect_parse_err(std::string input) {
    auto r = jsonl::parse_json(input);
    if (r.has_value()) {
        check(false, std::format("{} -> {}({})", input, desc(*r), jsonl::encode_json(*r)));
        return;
    }
    check(true, std::format("{} [PARSE ERROR]: {}", input, r.error()));
}

static void expect_parse_boolean(bool b) {
    expect_parse(std::format("{}", b), jsonl::Value(b));
}

template<typename T>
static void expect_parse_number(T x) {
    std::string input = std::format("{}", x);
    expect_parse(input, jsonl::Value(x));
}

static void expect_parse_string(std::string s) {
    std::string input = std::format("\"{}\"", s);
    expect_parse(input, jsonl::Value(s));
}

static void expect_parse_object(std::string s, Obj expected) {
    expect_parse(s, expected);
}

static void expect_parse_array(std::string s, Arr expected) {
    expect_parse(s, expected);
}

/*
 * Encode
 */

static void expect_encode(jsonl::Value v, std::string expected) {
    auto r = jsonl::encode_json(v);
    check(r == expected, std::format("{} == \"{}\"", r, expected));
}

static void test_roundtrip(jsonl::Value v) {
    std::string enc = jsonl::encode_json(v);

    auto r = jsonl::parse_json(enc);
    if (!r.has_value()) {
        check(false, "parse error: " + r.error());
        return;
    }

    check(r == v, std::format("roundtrip: {}", enc));
}

int main() {
    std::print("=== Json parser tests ===\n\n");

    std::print("--- Parse ---\n");
    std::print("Accept:\n");
    expect_parse_number(0); expect_parse_number(1.0); expect_parse_number(-0.54321);
    expect_parse_string("hello json !");
    expect_parse_boolean(true); expect_parse_boolean(false);
    expect_parse_object(R"({"key": "value", "num": 1, "bool": false})", { {"key", "value"}, {"num", 1}, {"bool", false} });
    expect_parse_array(R"([0, true, "trois"])", { 0, true, "trois" });
    expect_parse("null", jsonl::Value());
    std::print("Reject:\n");
    expect_parse_err("truee");
    expect_parse_err("\"hello");
    expect_parse_err("world\"");
    expect_parse_err("\"illegal symbol \" \"");

    std::print("\n--- Encode ---\n");
    expect_encode(0, "0");
    expect_encode(true, "true");
    expect_encode("false", R"("false")");
    expect_encode(Arr{0, true, "false"}, R"([0,true,"false"])");

    std::print("\n--- Roundtrip ---\n");
    test_roundtrip(Obj{ {"key", "value"}, {"num", 0}, {"bool", true} });
    test_roundtrip(Obj{ {"key", Obj{{"key", Obj{}}}} });
    test_roundtrip(Obj{ {"key", Obj{ {"key", Obj{ {"key", jsonl::Value()} }} }} });
    test_roundtrip(Obj{ {"array", Arr{0,true,"trois",Arr{jsonl::Value(),false,Obj{{"key",true}}}}} });

    std::print("\n=== {} passed, {} failed ===\n", gPassed, gFailed);
    return gFailed == 0 ? 0 : 1;
}
