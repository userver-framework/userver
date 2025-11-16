#pragma once

#include <string_view>
#include <tuple>
#include <array>
#include <optional>
#include <memory>
#include <functional>

#include <userver/formats/json/parser/base_parser.hpp>
#include <userver/formats/json/parser/meta_parser.hpp>
#include <userver/formats/json/parser/typed_parser.hpp>
#include <userver/utils/meta.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename T, typename C>
struct Field {
    using MemberType = T;
    using ClassType = C;

    const char* key;
    MemberType ClassType::* member_ptr;

    constexpr Field(const char* k, MemberType ClassType::* ptr) : key(k), member_ptr(ptr) {}
};

template <typename T, auto... Validators>
auto CreateSaxParserChain();

namespace detail {

template <size_t N>
struct StringViewMap {
    std::array<std::string_view, N> keys{};
    std::array<size_t, N> indices{};
    size_t size{0};

    constexpr std::optional<size_t> Find(std::string_view key) const {
        for (size_t i = 0; i < size; ++i) {
            if (keys[i] == key) return indices[i];
        }
        return std::nullopt;
    }
};

template <typename... Fields>
constexpr auto CreateKeyToIndexMap(const std::tuple<Fields...>& metadata) {
    constexpr size_t size = sizeof...(Fields);
    StringViewMap<size> map{};
    
    size_t index = 0;
    auto assign = [&](const auto& field) {
        map.keys[index] = field.key;
        map.indices[index] = index;
        index++;
    };
    
    std::apply([&](const auto&... fields) { (assign(fields), ...); }, metadata);
    map.size = size;
    return map;
}

template <typename FieldType>
struct FieldTypeExtractor;

template <typename T, typename C>
struct FieldTypeExtractor<Field<T, C>> {
    using type = T;
};

template <typename FieldType>
using FieldTypeExtractorT = typename FieldTypeExtractor<FieldType>::type;

template <typename Tuple>
struct FieldTypesFromFields;

template <typename... Fields>
struct FieldTypesFromFields<std::tuple<Fields...>> {
    using type = std::tuple<typename FieldTypeExtractor<Fields>::type...>;
};

template <typename Tuple>
using FieldTypesFromFieldsT = typename FieldTypesFromFields<Tuple>::type;

template <typename T, typename Class, typename FieldType>
class DirectAssignmentSubscriber final : public Subscriber<FieldType> {
public:
    using MemberPtr = FieldType Class::*;

    DirectAssignmentSubscriber(Class* object, MemberPtr member_ptr) 
        : object_(object), member_ptr_(member_ptr) {}

    void OnSend(FieldType&& value) override {
        object_->*member_ptr_ = std::move(value);
    }

private:
    Class* object_;
    MemberPtr member_ptr_;
};

template <typename FieldTypes, size_t... Is>
auto CreateParsersTupleImpl(std::index_sequence<Is...>) {
    return std::make_tuple(CreateSaxParserChain<std::tuple_element_t<Is, FieldTypes>>()...);
}

template <typename T, typename MetadataTuple, typename FieldTypes, size_t... Is>
auto CreateSubscribersTupleImpl(T* object, const MetadataTuple& metadata, std::index_sequence<Is...>) {
    return std::make_tuple(
        DirectAssignmentSubscriber<T, T, std::tuple_element_t<Is, FieldTypes>>(
            object, 
            std::get<Is>(metadata).member_ptr
        )...
    );
}

template <typename Tuple, typename F, size_t... Is>
void VisitTupleIndexImpl(Tuple& tuple, size_t index, F&& func, std::index_sequence<Is...>) {
    size_t i = 0;
    ((i++ == index ? (func(std::get<Is>(tuple)), void()) : void()), ...);
}

template <typename Tuple, typename F>
void VisitTupleIndex(Tuple& tuple, size_t index, F&& func) {
    VisitTupleIndexImpl(tuple, index, std::forward<F>(func), 
                       std::make_index_sequence<std::tuple_size_v<Tuple>>{});
}

}  // namespace detail

template <typename T>
class ObjectParser final : public TypedParser<T> {
private:
    static constexpr auto kMetadata = T::DescribeForJsonParsing();
    using MetadataTuple = std::decay_t<decltype(kMetadata)>;
    using FieldTypes = detail::FieldTypesFromFieldsT<MetadataTuple>;
    using ParsersTuple = decltype(detail::CreateParsersTupleImpl<FieldTypes>(
        std::make_index_sequence<std::tuple_size_v<FieldTypes>>{}));
    using SubscribersTuple = decltype(detail::CreateSubscribersTupleImpl<T, MetadataTuple, FieldTypes>(
        nullptr, kMetadata, std::make_index_sequence<std::tuple_size_v<FieldTypes>>{}));
    
    static constexpr size_t kFieldCount = std::tuple_size_v<FieldTypes>;
    static constexpr auto kKeyToIndexMap = detail::CreateKeyToIndexMap(kMetadata);

public:
    ObjectParser() 
        : parsers_(detail::CreateParsersTupleImpl<FieldTypes>(
            std::make_index_sequence<kFieldCount>{})),
          subscribers_(detail::CreateSubscribersTupleImpl<T, MetadataTuple, FieldTypes>(
            &result_, kMetadata, std::make_index_sequence<kFieldCount>{}))
    {
        SubscribeParsers(std::make_index_sequence<kFieldCount>{});
    }

    void Reset() override {
        result_ = T{};
        state_ = State::kStart;
        active_field_index_.reset();
        
        std::apply([](auto&... parsers) { (..., parsers.Reset()); }, parsers_);
    }

    void StartObject() override {
        if (state_ == State::kStart) {
            state_ = State::kInside;
        } else {
            this->Throw("{");
        }
    }

    void Key(std::string_view key) override {
        if (state_ != State::kInside) this->Throw("object key");

        if (auto index = kKeyToIndexMap.Find(key)) {
            active_field_index_ = *index;            
            ActivateParser(*index);
        } else {
            this->parser_state_->PopMe(*this);
        }
    }

    void EndObject(size_t) override {
        if (state_ == State::kInside) {
            this->SetResult(std::move(result_));
        } else {
            this->Throw("}");
        }
    }

    std::string Expected() const override { return "object"; }

    void StartArray() override { this->Throw("array"); }
    void EndArray(size_t) override { this->Throw("array"); }
    void String(std::string_view) override { this->Throw("string"); }
    void Int64(int64_t) override { this->Throw("integer"); }
    void Uint64(uint64_t) override { this->Throw("integer"); }
    void Double(double) override { this->Throw("double"); }
    void Bool(bool) override { this->Throw("boolean"); }
    void Null() override { this->Throw("null"); }

private:
    std::string GetPathItem() const override { return {}; }

    template <size_t... Is>
    void SubscribeParsers(std::index_sequence<Is...>) {
        (std::get<Is>(parsers_).Subscribe(std::get<Is>(subscribers_)), ...);
    }

    void ActivateParser(size_t index) {
        detail::VisitTupleIndex(parsers_, index, [this](auto& parser) {
            this->parser_state_->PushParser(parser.GetParser());
        });
    }

    enum class State { kStart, kInside };

    T result_{};
    State state_{State::kStart};
    std::optional<size_t> active_field_index_;

    ParsersTuple parsers_;
    SubscribersTuple subscribers_;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END