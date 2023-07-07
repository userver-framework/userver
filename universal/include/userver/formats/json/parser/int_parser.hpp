#pragma once

#include <cstdint>

#include <userver/formats/json/parser/typed_parser.hpp>

USERVER_NAMESPACE_BEGIN

namespace formats::json::parser {

template <typename T = int>
class IntegralParser;

template <>
class IntegralParser<std::int32_t> final : public TypedParser<std::int32_t> {
 public:
  using TypedParser<std::int32_t>::TypedParser;

 private:
  void Int64(std::int64_t i) override;

  void Uint64(std::uint64_t i) override;

  void Double(double value) override;

  std::string GetPathItem() const override { return {}; }

  std::string Expected() const override { return "integer"; }
};

template <>
class IntegralParser<std::int64_t> final : public TypedParser<std::int64_t> {
 public:
  using TypedParser<std::int64_t>::TypedParser;

 private:
  void Int64(std::int64_t i) override;

  void Uint64(std::uint64_t i) override;

  void Double(double value) override;

  std::string GetPathItem() const override { return {}; }

  std::string Expected() const override { return "integer"; }
};

using IntParser = IntegralParser<std::int32_t>;
using Int32Parser = IntegralParser<std::int32_t>;
using Int64Parser = IntegralParser<std::int64_t>;

}  // namespace formats::json::parser

USERVER_NAMESPACE_END
