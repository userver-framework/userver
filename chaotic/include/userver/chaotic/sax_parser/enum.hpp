#pragma once

#include <userver/chaotic/convert.hpp>
#include <userver/chaotic/sax_parser/primitive.hpp>
#include <userver/formats/json/parser/int_parser.hpp>
#include <userver/formats/json/parser/string_parser.hpp>
#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::sax {
namespace impl {

template <typename StringEnum>
class StringEnumParser : private formats::json::parser::Subscriber<std::string> {
public:
    using ResultType = StringEnum;

    StringEnumParser()
    {
        parser_.Subscribe(*this);
    }

    void Reset() { parser_.Reset(); }

    void Subscribe(formats::json::parser::Subscriber<StringEnum>& subscriber) { subscriber_ = &subscriber; }

    formats::json::parser::BaseParser& GetParser() { return parser_; }

private:
    void OnSend(std::string&& value) override { subscriber_->OnSend(chaotic::ConvertTo<StringEnum>(value)); }

    formats::json::parser::StringParser parser_;
    formats::json::parser::Subscriber<StringEnum>* subscriber_{nullptr};
};

template <typename IntEnum>
class IntEnumParser : private formats::json::parser::Subscriber<std::int64_t> {
public:
    using ResultType = IntEnum;

    IntEnumParser()
    {
        parser_.Subscribe(*this);
    }

    void Reset() { parser_.Reset(); }

    void Subscribe(formats::json::parser::Subscriber<IntEnum>& subscriber) { subscriber_ = &subscriber; }

    formats::json::parser::BaseParser& GetParser() { return parser_; }

private:
    void OnSend(std::int64_t&& value) override { subscriber_->OnSend(chaotic::ConvertTo<IntEnum>(value)); }

    formats::json::parser::Int64Parser parser_;
    formats::json::parser::Subscriber<IntEnum>* subscriber_{nullptr};
};

template <typename Enum>
concept IsStringEnum = requires { Convert(std::string_view{}, chaotic::convert::To<Enum>{}); };

}  // namespace impl

template <typename Enum>
requires std::is_enum_v<Enum>
auto ParserOf(Type<Enum>)
{
    if constexpr (impl::IsStringEnum<Enum>) {
        return impl::StringEnumParser<Enum>{};
    } else {
        return impl::IntEnumParser<Enum>{};
    }
}

}  // namespace chaotic::sax

USERVER_NAMESPACE_END
