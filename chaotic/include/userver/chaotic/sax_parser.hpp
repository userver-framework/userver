#pragma once

#include <userver/chaotic/array.hpp>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/oneof_with_discriminator.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/ref.hpp>
#include <userver/chaotic/sax_parser/enum.hpp>
#include <userver/chaotic/sax_parser/object.hpp>
#include <userver/chaotic/sax_parser/primitive.hpp>
#include <userver/chaotic/variant.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/json/parser/dummy_parser.hpp>
#include <userver/formats/json/parser/parser_json.hpp>
#include <userver/utils/box.hpp>
#include <userver/utils/constexpr_indices.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::sax::impl {

template <typename RawParser, typename UserType>
class WithType final : private formats::json::parser::Subscriber<typename RawParser::ResultType> {
public:
    using ResultType = UserType;

    WithType() { parser_.Subscribe(*this); }

    void Reset() { parser_.Reset(); }

    void Subscribe(formats::json::parser::Subscriber<ResultType>& subscriber) { subscriber_ = &subscriber; }

    formats::json::parser::BaseParser& GetParser() { return parser_.GetParser(); }

private:
    void OnSend(typename RawParser::ResultType&& value) override {
        if constexpr (std::is_same_v<typename RawParser::ResultType, UserType>) {
            subscriber_->OnSend(std::move(value));
        } else {
            auto user_value = [this, &value] {
                try {
                    return Convert(std::move(value), convert::To<UserType>{});
                } catch (const std::exception& e) {
                    formats::json::parser::BaseParser& base = parser_.GetParser();
                    chaotic::ThrowForPath<formats::json::Value>(e.what(), base.GetCurrentPath());
                }
            }();
            subscriber_->OnSend(std::move(user_value));
        }
    }

    RawParser parser_;
    formats::json::parser::Subscriber<ResultType>* subscriber_{nullptr};
};

template <typename Parser, typename... Validator>
class WithValidators final : private formats::json::parser::Subscriber<typename Parser::ResultType> {
public:
    using ResultType = typename Parser::ResultType;

    WithValidators() { parser_.Subscribe(*this); }

    void Reset() { parser_.Reset(); }

    void Subscribe(formats::json::parser::Subscriber<ResultType>& subscriber) { subscriber_ = &subscriber; }

    formats::json::parser::BaseParser& GetParser() { return parser_; }

private:
    void OnSend(ResultType&& value) override {
        [[maybe_unused]] const auto error_reporter = [this](std::string_view error) {
            formats::json::parser::BaseParser& base = parser_;
            chaotic::ThrowForPath<formats::json::Value>(error, base.GetCurrentPath());
        };
        (Validator::Validate(value, error_reporter), ...);

        subscriber_->OnSend(std::move(value));
    }

    Parser parser_;
    formats::json::parser::Subscriber<ResultType>* subscriber_{nullptr};
};

template <typename T, typename ResultType = TypeOfDescriptor<T>>
class RefParser final : private formats::json::parser::Subscriber<ResultType> {
public:
    RefParser()
        : parser_(std::make_unique<chaotic::sax::Parser<T>>())
    {
        parser_->Subscribe(*this);
    }

    void Reset() { parser_->Reset(); }

    void Subscribe(formats::json::parser::Subscriber<utils::Box<ResultType>>& subscriber) { subscriber_ = &subscriber; }

    formats::json::parser::BaseParser& GetParser() { return *parser_; }

private:
    void OnSend(ResultType&& value) { subscriber_->OnSend(utils::Box<ResultType>(std::move(value))); }

    std::unique_ptr<formats::json::parser::TypedParser<ResultType>> parser_;
    formats::json::parser::Subscriber<utils::Box<ResultType>>* subscriber_{nullptr};
};

template <typename T>
class JsonDomParser final : private formats::json::parser::Subscriber<formats::json::Value> {
public:
    using ResultType = formats::common::ParseType<formats::json::Value, T>;

    JsonDomParser()
    {
        json_parser_.Subscribe(*this);
    }

    void Reset() { json_parser_.Reset(); }

    void Subscribe(formats::json::parser::Subscriber<ResultType>& subscriber) { subscriber_ = &subscriber; }

    formats::json::parser::BaseParser& GetParser() { return json_parser_.GetParser(); }

private:
    void OnSend(formats::json::Value&& value) override { subscriber_->OnSend(value.template As<T>()); }

    formats::json::parser::JsonValueParser json_parser_;
    formats::json::parser::Subscriber<ResultType>* subscriber_{nullptr};
};

}  // namespace chaotic::sax::impl

namespace chaotic::sax {

template <typename T>
sax::Parser<T> ParserOf(Type<Primitive<T>>);

template <typename RawType, typename UserType>
auto ParserOf(Type<WithType<RawType, UserType>>)
{
    using RawParser = sax::Parser<RawType>;
    return sax::impl::WithType<RawParser, UserType>{};
}

template <typename T>
sax::impl::RefParser<T> ParserOf(Type<Ref<T>>);

template <typename T, typename Validator0, typename... Validator>
auto ParserOf(Type<Primitive<T, Validator0, Validator...>>)
{
    using Subparser = sax::Parser<T>;
    return sax::impl::WithValidators<Subparser, Validator0, Validator...>{};
}

template <typename ItemDescriptor, typename ArrayType>
auto ParserOf(Type<chaotic::Array<ItemDescriptor, ArrayType>>)
{
    using ItemParser = sax::Parser<ItemDescriptor>;
    using ArrayParser = formats::json::parser::ArrayParser<typename ItemParser::ResultType, ItemParser, ArrayType>;
    return sax::JoinedParser<ArrayParser, ItemParser>{};
}

template <typename ItemDescriptor, typename ArrayType, typename Validator0, typename... Validator>
auto ParserOf(Type<chaotic::Array<ItemDescriptor, ArrayType, Validator0, Validator...>>)
{
    using Array = chaotic::Array<ItemDescriptor, ArrayType>;
    using Subparser = sax::Parser<Array>;
    return sax::impl::WithValidators<Subparser, Validator0, Validator...>{};
}

template <typename StructType, typename Unknown, typename... Fields>
sax::impl::ObjectParser<StructType, Unknown, Fields...> ParserOf(Type<Object<StructType, Unknown, Fields...>>);

// TODO: can be optimized to avoid DOM
template <const auto* Settings, typename... T>
sax::impl::JsonDomParser<OneOfWithDiscriminator<Settings, T...>> ParserOf(Type<OneOfWithDiscriminator<Settings, T...>>);

// TODO: can be optimized to avoid DOM
template <typename... Fields>
sax::impl::JsonDomParser<std::variant<Fields...>> ParserOf(Type<std::variant<Fields...>>);

template <typename... Fields>
sax::Parser<std::variant<Fields...>> ParserOf(Type<Variant<Fields...>>);

}  // namespace chaotic::sax

USERVER_NAMESPACE_END
