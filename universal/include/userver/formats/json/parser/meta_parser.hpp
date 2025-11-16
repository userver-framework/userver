#pragma once

#include <string>
#include <type_traits>
#include <userver/formats/json/parser/array_proxy_parser.hpp>
#include <userver/formats/json/parser/json_proxy_parser.hpp>
#include <userver/formats/json/parser/map_proxy_parser.hpp>
#include <userver/formats/json/parser/object_proxy_parser.hpp>
#include <userver/formats/json/parser/parser.hpp>
#include <userver/formats/json/parser/parser_json.hpp>
#include <userver/formats/json/parser/primitive_proxy_parser.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename T, typename Enable = void, auto... Validators>
struct SaxParser;

template <typename ParserType>
struct SaxParserBase {
    using type = ParserType;
    static type Create() { return type(); }
};

template <typename T, auto... Validators>
struct WithValidators {
    using type = T;
};

template <typename T>
struct IsValidated : std::false_type {};
template <typename T, auto... Validators>
struct IsValidated<WithValidators<T, Validators...>> : std::true_type {};
template <typename T>
inline constexpr bool kIsValidated = IsValidated<T>::value;

template <typename T, typename = void>
struct HasDescribeForJsonParsing : std::false_type {};
template <typename T>
struct HasDescribeForJsonParsing<T, std::void_t<decltype(T::DescribeForJsonParsing())>> : std::true_type {};
template <typename T>
inline constexpr bool kHasDescribeForJsonParsing = HasDescribeForJsonParsing<T>::value;

template <typename T>
struct RebindContainer;

template <template <typename...> class Container, typename... Args>
struct RebindContainer<Container<Args...>> {
    template <typename... NewArgs>
    using type = Container<NewArgs...>;
};

template <typename T, typename Enable = void>
struct ExtractInnerTypeRecursive {
    using type = T;
};

template <typename T, auto... Validators>
struct ExtractInnerTypeRecursive<WithValidators<T, Validators...>> {
    using type = typename ExtractInnerTypeRecursive<T>::type;
};

template <typename T>
struct ExtractInnerTypeRecursive<T, std::enable_if_t<meta::kIsRange<T> && !meta::kIsMap<T>>> {
    using value_type = typename T::value_type;
    using clean_value_type = typename ExtractInnerTypeRecursive<value_type>::type;

    using type = typename RebindContainer<T>::template type<clean_value_type>;
};

template <typename T>
struct ExtractInnerTypeRecursive<T, std::enable_if_t<meta::kIsMap<T>>> {
    using key_type = typename T::key_type;
    using mapped_type = typename T::mapped_type;
    using clean_key_type = typename ExtractInnerTypeRecursive<key_type>::type;
    using clean_mapped_type = typename ExtractInnerTypeRecursive<mapped_type>::type;

    using type = typename RebindContainer<T>::template type<clean_key_type, clean_mapped_type>;
};

// base types
template <void (*... Validators)(bool)>
struct SaxParser<bool, void, Validators...> : SaxParserBase<PrimitiveProxyParser<bool, BoolParser, Validators...>> {};

template <void (*... Validators)(double)>
struct SaxParser<double, void, Validators...>
    : SaxParserBase<PrimitiveProxyParser<double, DoubleParser, Validators...>> {};

template <void (*... Validators)(int32_t)>
struct SaxParser<int32_t, void, Validators...>
    : SaxParserBase<PrimitiveProxyParser<int32_t, Int32Parser, Validators...>> {};

template <void (*... Validators)(int64_t)>
struct SaxParser<int64_t, void, Validators...>
    : SaxParserBase<PrimitiveProxyParser<int64_t, Int64Parser, Validators...>> {};

template <void (*... Validators)(const std::string&)>
struct SaxParser<std::string, void, Validators...>
    : SaxParserBase<PrimitiveProxyParser<std::string, StringParser, Validators...>> {};

template <auto... Validators>
struct SaxParser<formats::json::Value, void, Validators...>
    : SaxParserBase<JsonValueProxyParser<formats::json::Value, Validators...>> {};

// universal for base types WithValidators
template <typename InnerType, auto... Validators>
struct SaxParser<WithValidators<InnerType, Validators...>, void> : SaxParser<InnerType, void, Validators...> {};

// custom struct
template <typename T, auto... Validators>
struct SaxParser<T, std::enable_if_t<kHasDescribeForJsonParsing<T>>, Validators...>
    : SaxParserBase<ObjectProxyParser<T, Validators...>> {};

// custom types
template <typename T, void (*... Validators)(const T&)>
struct SaxParser<
    T,
    std::enable_if_t<
        !kHasDescribeForJsonParsing<T> && !kIsValidated<T> && !meta::kIsRange<T> && !meta::kIsMap<T> &&
        !std::is_same_v<T, bool> && !std::is_same_v<T, double> && !std::is_same_v<T, int32_t> &&
        !std::is_same_v<T, int64_t> && !std::is_same_v<T, std::string>>,
    Validators...> : SaxParserBase<JsonValueProxyParser<T, Validators...>> {
    static_assert(
        !std::is_class_v<T> || formats::common::impl::kHasParse<Value, T>,
        "\n\n"
        "\033[32m"
        "\n\n>>> Parsing a custom struct/class requires a 'DescribeForJsonParsing' function. <<<\n"
        "To enable SAX parsing for your type, add a static constexpr function inside it, like this:\n\n"
        "struct YourType {\n"
        "    int some_field;\n"
        "    std::string another_field;\n\n"
        "    // This allows the parser to map JSON keys to your struct members.\n"
        "    static constexpr auto DescribeForJsonParsing() {\n"
        "        // Use formats::json::parser::Field for each member.\n"
        "        return std::make_tuple(\n"
        "            formats::json::parser::Field(\"json_key_for_some_field\", &YourType::some_field),\n"
        "            formats::json::parser::Field(\"another_field_json\", &YourType::another_field)\n"
        "        );\n"
        "    }\n\n"
        "    // Also, define this to enable parsing of unique field types.\n"
        "};\n\n"
        "Alternatively, if you want to use DOM-based parsing, provide a 'Parse' free function for your type.\n"
        "\033[0m"
    );
};

// ranged types
template <typename T, auto... Validators>
struct SaxParser<T, std::enable_if_t<meta::kIsRange<T> && !meta::kIsMap<T>>, Validators...> {
    using DeclaredElementType = typename T::value_type;
    using ElementParser = typename SaxParser<DeclaredElementType>::type;
    using ResultElementType = typename ElementParser::ResultType;

    using ResultType = typename ExtractInnerTypeRecursive<T>::type;

    using type = ArrayProxyParser<ResultElementType, ElementParser, ResultType, Validators...>;

    static type Create() { return type(SaxParser<DeclaredElementType>::Create()); }
};

// maps
template <typename T, auto... Validators>
struct SaxParser<T, std::enable_if_t<meta::kIsMap<T>>, Validators...> {
    static_assert(
        std::is_same_v<typename ExtractInnerTypeRecursive<typename T::key_type>::type, std::string>,
        "JSON object keys must be strings."
    );

    using DeclaredValueType = typename T::mapped_type;
    using ValueParser = typename SaxParser<DeclaredValueType>::type;
    using ResultValueType = typename ValueParser::ResultType;

    using ResultType = typename ExtractInnerTypeRecursive<T>::type;

    using type = MapProxyParser<ResultType, ValueParser, Validators...>;

    static type Create() { return type(SaxParser<DeclaredValueType>::Create()); }
};

template <typename T, auto... Validators>
auto CreateSaxParserChain() {
    return SaxParser<T, void, Validators...>::Create();
}

}  // namespace formats::json::parser

USERVER_NAMESPACE_END