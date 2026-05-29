#pragma once

#include <array>
#include <unordered_map>

#include <userver/chaotic/object.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/sax_parser/primitive.hpp>
#include <userver/formats/json/parser/array_parser.hpp>
#include <userver/formats/json/parser/dummy_parser.hpp>
#include <userver/formats/json/parser/parser_json.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/utils/constexpr_indices.hpp>
#include <userver/utils/meta.hpp>
#include <userver/utils/overloaded.hpp>
#include <userver/utils/trivial_map.hpp>

USERVER_NAMESPACE_BEGIN

namespace chaotic::sax::impl {

class UnknownSubscriber final : public formats::json::parser::Subscriber<formats::json::Value> {
public:
    void Reset();

    void SetKey(std::string_view key);

    void OnSend(formats::json::Value&& value) override;

    formats::json::Value ExtractValue();

private:
    formats::json::ValueBuilder builder_;
    std::string key_;
};

template <typename DescriptorType>
struct FieldModeSubscriber {};

template <typename DescriptorType>
struct FieldModeSubscriber<Required<DescriptorType>> {
    using Type = formats::json::parser::SubscriberSink<TypeOfDescriptor<DescriptorType>>;
};

template <typename DescriptorType>
struct FieldModeSubscriber<Optional<DescriptorType>> {
    using Type = formats::json::parser::SubscriberSinkOptional<TypeOfDescriptor<DescriptorType>>;
};

template <typename DescriptorType, typename T, const T& Default>
struct FieldModeSubscriber<Defaulted<DescriptorType, T, Default>> {
    using Type = formats::json::parser::SubscriberSink<TypeOfDescriptor<DescriptorType>>;
};

template <typename Field>
using FieldSubscriber = typename FieldModeSubscriber<typename Field::ModeDescriptorType>::Type;

template <typename Unknown>
class UnknownFieldsParser {};

template <>
class UnknownFieldsParser<UnknownFields::Ignore> {
public:
    formats::json::parser::BaseParser& GetParser() { return dummy_parser_; }

    void Key(std::string_view, std::string_view) {}

    void Reset() {}

    template <typename T>
    void SetExtra(T&) {}

private:
    formats::json::parser::DummyParser dummy_parser_;
};

template <>
class UnknownFieldsParser<UnknownFields::Forbid> {
public:
    formats::json::parser::BaseParser& GetParser() { return dummy_parser_; }

    void Key(std::string_view key, std::string_view type) {
        throw formats::json::parser::InternalParseError(fmt::format("unknown field '{}' for type '{}'", key, type));
    }

    template <typename T>
    void SetExtra(T&) {}

    void Reset() {}

private:
    formats::json::parser::DummyParser dummy_parser_;
};

template <>
class UnknownFieldsParser<UnknownFields::StoreJson> {
public:
    UnknownFieldsParser()
    {
        unknown_parser_.Subscribe(unknown_subscriber_);
    }

    formats::json::parser::BaseParser& GetParser() { return unknown_parser_; }

    void Key(std::string_view key, std::string_view) { unknown_subscriber_.SetKey(key); }

    template <typename T>
    void SetExtra(T& s)
    {
        s.extra = unknown_subscriber_.ExtractValue();
    }

    void Reset() {
        unknown_parser_.Reset();
        unknown_subscriber_.Reset();
    }

private:
    formats::json::parser::JsonValueParser unknown_parser_;
    UnknownSubscriber unknown_subscriber_;
};

template <typename T>
class UnknownFieldsParser<UnknownFields::StoreTyped<T>>
    : private formats::json::parser::Subscriber<TypeOfDescriptor<T>> {
public:
    using ResultType = TypeOfDescriptor<T>;

    UnknownFieldsParser()
    {
        parser_.Subscribe(*this);
    }

    void Key(std::string_view key, std::string_view) {
        key_ = key;
        parser_.Reset();
    }

    formats::json::parser::BaseParser& GetParser() { return parser_.GetParser(); }

    void Reset() { parser_.Reset(); }

    template <typename U>
    void SetExtra(U& s)
    {
        using TargetType = decltype(s.extra);
        if constexpr (std::is_same_v<TargetType, MapType>) {
            s.extra = std::move(map_);
        } else if constexpr (meta::kIsMap<TargetType>) {
            s.extra.clear();
            s.extra.insert(std::move_iterator(map_.begin()), std::move_iterator(map_.end()));
        } else {
            s.extra = chaotic::ConvertTo<TargetType>(std::move(map_));
        }
    }

private:
    void OnSend(ResultType&& value) override { map_.emplace(std::move(key_), std::move(value)); }

    using MapType = std::unordered_map<std::string, ResultType>;
    MapType map_;
    Parser<T> parser_;
    std::string key_;
};

template <typename Struct, typename Unknown, typename... Field>
class FieldSubparser {
    static constexpr std::size_t Count = sizeof...(Field);

public:
    FieldSubparser(Struct& object)
        : object_(object)
    {}

    void Key(std::string_view key) {
        auto handler = kKeyHandlersMap.TryFindByFirst(key);
        if (handler) {
            auto method = *handler;
            (this->*method)();
            is_key_known_ = true;
        } else {
            // Unknown field
            unknown_field_parser_.Key(key, compiler::GetTypeName<Struct>());
            is_key_known_ = false;
        }
    }

    formats::json::parser::BaseParser& GetParser() {
        if (!is_key_known_) {
            return unknown_field_parser_.GetParser();
        }

        return std::visit(
            utils::Overloaded{
                [](auto& parser) -> formats::json::parser::BaseParser& { return parser.GetParser(); },
                [](std::monostate) -> formats::json::parser::BaseParser& {
                    throw formats::json::parser::InternalParseError("Trying to get empty parser");
                }
            },
            parser_
        );
    }

    void ValidateSeen() { ValidateSeen(std::make_index_sequence<Count>()); }

    template <std::size_t... Idx>
    void ValidateSeen(std::index_sequence<Idx...>) {
        // An ugly way to get `Idx` in pack expansion
        (DoValidateSeen<Idx, Field>(), ...);
    }

    template <std::size_t Idx, typename FieldType>
    void DoValidateSeen() {
        using ModeDescriptorType = typename FieldType::ModeDescriptorType;

        if constexpr (meta::kIsInstantiationOf<Required, ModeDescriptorType>) {
            static_assert(Idx < Count);
            if (!seen_[Idx]) {
                throw formats::json::parser::InternalParseError(
                    fmt::format("missing required field '{}'", FieldType::Name)
                );
            }
        } else if constexpr (IsDefaulted<ModeDescriptorType>::value) {
            using TargetType = std::decay_t<decltype(object_.*FieldType::kField)>;
            using DefaultType = std::decay_t<decltype(ModeDescriptorType::DefaultValue)>;
            if constexpr (std::is_same_v<TargetType, DefaultType>) {
                object_.*FieldType::kField = ModeDescriptorType::DefaultValue;
            } else if constexpr (std::is_arithmetic_v<TargetType> && std::is_arithmetic_v<DefaultType>) {
                object_.*FieldType::kField = TargetType{ModeDescriptorType::DefaultValue};
            } else {
                object_.*FieldType::kField = chaotic::ConvertTo<TargetType>(ModeDescriptorType::DefaultValue);
            }
        }
    }

    void Reset() {
        seen_ = {};

        unknown_field_parser_.Reset();
    }

    void SetExtra(Struct& s) { unknown_field_parser_.SetExtra(s); }

private:
    std::array<bool, sizeof...(Field)> seen_ = {};
    std::variant<std::monostate, Parser<typename Field::Descriptor>...> parser_{};
    std::variant<std::monostate, FieldSubscriber<Field>...> subscriber_{};
    Struct& object_;

    UnknownFieldsParser<Unknown> unknown_field_parser_;
    bool is_key_known_{false};

    template <std::size_t Idx, typename FieldType>
    void HandleKey() {
        static_assert(Idx < Count);
        if (seen_[Idx]) {
            throw formats::json::parser::InternalParseError(fmt::format("duplicate key '{}'", FieldType::Name));
        }

        // Mark key name as seen
        seen_[Idx] = true;

        // Create field parser, use Idx instead of FieldType due to possible duplicates
        parser_.template emplace<Idx + 1>();
        auto& p = std::get<Idx + 1>(parser_);
        p.Reset();

        // Subscribe on field
        subscriber_.template emplace<Idx + 1>(object_.*FieldType::kField);
        auto& s = std::get<Idx + 1>(subscriber_);
        p.Subscribe(s);
    }

    template <std::size_t... Idx>
    static constexpr std::array<void (FieldSubparser::*)(), Count> MakeHandlers(std::index_sequence<Idx...>) {
        return {&FieldSubparser::HandleKey<Idx, Field>...};
    }

    static constexpr std::string_view kKeys[] = {Field::Name...};
    static constexpr auto kKeyHandlers = MakeHandlers(std::make_index_sequence<Count>{});
    static constexpr utils::TrivialBiMap kKeyHandlersMap = utils::MakeTrivialBiMap<kKeys, kKeyHandlers>();
};

template <typename Struct, typename Unknown>
class FieldSubparser<Struct, Unknown> {
public:
    FieldSubparser(Struct& object)
        : object_(object)
    {}

    void Key(std::string_view key) { unknown_field_parser_.Key(key, compiler::GetTypeName<Struct>()); }

    formats::json::parser::BaseParser& GetParser() { return unknown_field_parser_.GetParser(); }

    void ValidateSeen() {}

    void Reset() { unknown_field_parser_.Reset(); }

    void SetExtra(Struct& s) { unknown_field_parser_.SetExtra(s); }

private:
    Struct& object_;

    UnknownFieldsParser<Unknown> unknown_field_parser_;
};

template <typename StructType, typename Unknown, typename... Fields>
class ObjectParser final : public formats::json::parser::TypedParser<StructType> {
public:
    using formats::json::parser::TypedParser<StructType>::TypedParser;

    void Reset() override {
        state_ = State::kStart;
        result_ = {};
        field_subparser_.Reset();
    }

    void StartObject() override {
        switch (state_) {
            case State::kStart:
                state_ = State::kInside;
                break;

            case State::kInside:
                this->Throw("{");
        }
    }

    void Key(std::string_view key) override {
        key_ = key;
        field_subparser_.Key(key);
        this->parser_state_->PushParser(field_subparser_.GetParser());
    }

    void EndObject() override {
        switch (state_) {
            case State::kStart:
                this->Throw("}");

            case State::kInside:
                // If an exception is thrown below, we must not set .old_key
                key_.clear();

                field_subparser_.ValidateSeen();
                field_subparser_.SetExtra(result_);
                this->SetResult(std::move(result_));
                break;
        }
    }

private:
    std::string Expected() const override {
        switch (state_) {
            case State::kStart:
                return "object";

            case State::kInside:
                return "field name";
        }

        UINVARIANT(false, "Invalid state");
    }

    std::string GetPathItem() const override { return key_; }

    enum class State {
        kStart,
        kInside,
    };
    State state_;

    StructType result_;
    std::string key_;

    FieldSubparser<StructType, Unknown, Fields...> field_subparser_{result_};
};

}  // namespace chaotic::sax::impl

USERVER_NAMESPACE_END
