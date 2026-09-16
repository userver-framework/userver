#pragma once

/// @file userver/formats/json/parser/raw_string_parser.hpp
/// @brief @copybrief formats::json::parser::RawStringParser
/// @ingroup userver_universal

#include <cstdint>
#include <string>
#include <string_view>

#include <userver/formats/json/parser/typed_parser.hpp>
#include <userver/formats/json/raw_string.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

/// @brief SAX parser for arbitrary JSON values stored as RawString.
class JsonRawStringParser final : public TypedParser<RawString> {
public:
    JsonRawStringParser();
    ~JsonRawStringParser() override;

    JsonRawStringParser(const JsonRawStringParser&) = delete;
    JsonRawStringParser(JsonRawStringParser&&) = delete;

    void Reset() override;

protected:
    void Null() override;
    void Bool(bool value) override;
    void Int64(std::int64_t value) override;
    void Uint64(std::uint64_t value) override;
    void Double(double value) override;
    void String(std::string_view value) override;
    void StartObject() override;
    void Key(std::string_view key) override;
    void EndObject() override;
    void StartArray() override;
    void EndArray() override;

    std::string GetPathItem() const override { return {}; }

    std::string Expected() const override;

private:
    void MaybeSetResult();

    struct Impl;
    utils::FastPimpl<Impl, 120, 8> impl_;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
