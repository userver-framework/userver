#pragma once

#include <userver/chaotic/sax_parser/primitive.hpp>
#include <userver/formats/json/parser/int_parser.hpp>
#include <userver/formats/json/parser/string_parser.hpp>
#include <userver/formats/parse/to.hpp>
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
    void OnSend(std::string&& value) override {
        subscriber_->OnSend(FromString(value, formats::parse::To<StringEnum>()));
    }

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
    void OnSend(std::int64_t&& value) override { subscriber_->OnSend(FromInt(value, formats::parse::To<IntEnum>())); }

    formats::json::parser::Int64Parser parser_;
    formats::json::parser::Subscriber<IntEnum>* subscriber_{nullptr};
};

template <typename Enum, typename = void>
struct IsStringEnum : std::false_type {};

template <typename Enum>
struct IsStringEnum<Enum, utils::void_t<decltype(FromString(std::string_view{}, formats::parse::To<Enum>{}))>>
    : std::true_type {};

}  // namespace impl

template <typename Enum>
std::enable_if_t<
    std::is_enum_v<Enum>,
    std::conditional_t<impl::IsStringEnum<Enum>::value, impl::StringEnumParser<Enum>, impl::IntEnumParser<Enum>>>
    ParserOf(Type<Enum>);

}  // namespace chaotic::sax

USERVER_NAMESPACE_END
