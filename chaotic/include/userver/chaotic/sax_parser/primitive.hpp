#pragma once

#include <userver/chaotic/sax_parser_join.hpp>
#include <userver/formats/json/parser/array_parser.hpp>
#include <userver/formats/json/parser/bool_parser.hpp>
#include <userver/formats/json/parser/int_parser.hpp>
#include <userver/formats/json/parser/number_parser.hpp>
#include <userver/formats/json/parser/string_parser.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::sax {

/// ADL helper for specialization via ParserOf(Type<X>)
template <typename T>
struct Type {};

formats::json::parser::Int32Parser ParserOf(Type<std::int32_t>);

formats::json::parser::Int64Parser ParserOf(Type<std::int64_t>);

formats::json::parser::BoolParser ParserOf(Type<bool>);

formats::json::parser::FloatParser ParserOf(Type<float>);

formats::json::parser::DoubleParser ParserOf(Type<double>);

formats::json::parser::StringParser ParserOf(Type<std::string>);

template <typename Array>
requires(meta::kIsRange<Array> && !meta::kIsMap<Array>)
auto ParserOf(Type<Array>)
{
    using Value = typename Array::value_type;
    using ItemParser = decltype(ParserOf(Type<Value>{}));
    using Parser = formats::json::parser::ArrayParser<Value, ItemParser, Array>;
    return JoinedParser<Parser, ItemParser>{};
}

template <typename T>
using Parser = decltype(ParserOf(Type<T>{}));

}  // namespace chaotic::sax

USERVER_NAMESPACE_END
