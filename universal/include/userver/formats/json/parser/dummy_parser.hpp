#pragma once

#include <userver/formats/json/parser/base_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

class DummyParser final : public BaseParser {
public:
    DummyParser();
    ~DummyParser() override;

    void Null() override;
    void Bool(bool) override;
    void Int64(int64_t) override;
    void Uint64(uint64_t) override;
    void Double(double) override;
    void String(std::string_view) override;
    void StartObject() override;
    void Key(std::string_view key) override;
    void EndObject() override;
    void StartArray() override;
    void EndArray() override;

    std::string Expected() const override;

    void Reset();

    DummyParser& GetParser();

private:
    void MaybePopSelf();

    std::string GetPathItem() const override { return {}; }

    size_t level_{0};
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
