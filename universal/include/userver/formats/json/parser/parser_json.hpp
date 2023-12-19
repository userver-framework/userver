#pragma once

#include <userver/formats/json/parser/typed_parser.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/fast_pimpl.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

/// SAX-parser for formats::json::Value
class JsonValueParser final : public TypedParser<Value> {
 public:
  JsonValueParser();
  ~JsonValueParser() override;

  void Null() override;
  void Bool(bool) override;
  void Int64(int64_t) override;
  void Uint64(uint64_t) override;
  void Double(double) override;
  void String(std::string_view) override;
  void StartObject() override;
  void Key(std::string_view key) override;
  void EndObject(size_t) override;
  void StartArray() override;
  void EndArray(size_t) override;

  std::string Expected() const override;

 private:
  void MaybePopSelf();

  std::string GetPathItem() const override { return {}; }

  struct Impl;
  utils::FastPimpl<Impl, 127, 8> impl_;
};

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
