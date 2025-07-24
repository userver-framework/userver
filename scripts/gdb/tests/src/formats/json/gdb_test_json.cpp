#include <userver/formats/json/inline.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/gdb_tests/stub.hpp>

USERVER_NAMESPACE_BEGIN

__attribute__((noinline)) static void TestGdbPrinters() {
    using formats::json::MakeArray, formats::json::MakeObject;

    auto new_value = [](auto&& val) {
        return formats::json::ValueBuilder(std::forward<decltype(val)>(val)).ExtractValue();
    };

    formats::json::Value value{};
    TEST_INIT(value);
    DoNotOptimize(value);

    TEST_EXPR('value', 'null');

    value = new_value(true);
    TEST_EXPR('value', 'true');

    value = new_value(false);
    TEST_EXPR('value', 'false');

    value = new_value(1);
    TEST_EXPR('value', '1');

    value = new_value(1u);
    TEST_EXPR('value', '1');

    value = new_value(1.5);
    TEST_EXPR('value', '1.5');

    value = new_value("");
    TEST_EXPR('value', '""');

    value = new_value("1");
    TEST_EXPR('value', '"1"');

    value = new_value("this is a very long string, unusually long even, definitely not short");
    TEST_EXPR('value', '"this is a very long string, unusually long even, definitely not short"');

    value = MakeArray();
    TEST_EXPR('value', '[]');

    value = MakeArray("a");
    TEST_EXPR('value', '{"a"}');

    value = MakeArray("a", true);
    TEST_EXPR('value', '{"a", true}');

    value = MakeArray("a", true, MakeArray(1, 2, 3, 4));
    TEST_EXPR('value', '{"a", true, {1, 2, 3, 4}}');

    value = MakeArray("a", true, MakeArray(1, 2, 3, 4), 5);
    TEST_EXPR('value', '{"a", true, {1, 2, 3, 4}, 5}');

    value = MakeArray("a", true, MakeArray(1, 2, 3, 4), 5, MakeObject("key1", "value1", "key2", 987));
    TEST_EXPR('value', '{"a", true, {1, 2, 3, 4}, 5, {["key1"] = "value1", ["key2"] = 987}}');

    value = MakeObject();
    TEST_EXPR('value', '{}');

    value = MakeObject("a", "b");
    TEST_EXPR('value', '{["a"] = "b"}');

    value = MakeObject("a", "b", "c", 123);
    TEST_EXPR('value', '{["a"] = "b", ["c"] = 123}');

    value = MakeObject("a", "b", "c", 123, "d", false);
    TEST_EXPR('value', '{["a"] = "b", ["c"] = 123, ["d"] = false}');

    value = MakeObject("a", "b", "c", 123, "d", false, "e", "");
    TEST_EXPR('value', '{["a"] = "b", ["c"] = 123, ["d"] = false, ["e"] = ""}');

    value = MakeObject("a", "b", "c", 123, "d", false, "e", "", "f", MakeObject("key1", "value1", "key2", 987));
    TEST_EXPR(
        'value', '{["a"] = "b", ["c"] = 123, ["d"] = false, ["e"] = "", ["f"] = {["key1"] = "value1", ["key2"] = 987}}'
    );

    value = MakeObject(
        "a", MakeArray(1, 2, 3, 4), "c", 123, "d", false, "e", "", "f", MakeObject("key1", "value1", "key2", 987)
    );
    TEST_EXPR(
        'value',
        '{["a"] = {1, 2, 3, 4}, ["c"] = 123, ["d"] = false, ["e"] = "", ["f"] = {["key1"] = "value1", ["key2"] = 987}}'
    );

    value = formats::json::FromString(
        R"({"a":[1,{},[]],"b":[true,false],"c":{"internal":{"subkey":2}},"i":-1,"u":1,"i64":-18446744073709551614,"u64":18446744073709551614,"d":0.4})"
    );
    TEST_EXPR(
        'value',
        '{["a"] = {1, {}, []}, ["b"] = {true, false}, ["c"] = {["internal"] = {["subkey"] = 2}}, ["i"] = -1, ["u"] = 1, ["i64"] = -1.8446744073709552e+19, ["u64"] = 18446744073709551614, ["d"] = 0.40000000000000002}'
    );

    DoNotOptimize(value);
    TEST_DEINIT(value);
}

USERVER_NAMESPACE_END

int main() { USERVER_NAMESPACE::TestGdbPrinters(); }
