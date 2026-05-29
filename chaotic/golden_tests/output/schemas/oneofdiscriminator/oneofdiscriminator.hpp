#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/oneof_with_discriminator.hpp>
#include <userver/chaotic/type_bundle_hpp.hpp>
#include <userver/formats/json/value.hpp>
#include <variant>

#include "oneofdiscriminator_fwd.hpp"

namespace ns {

struct A {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNametype = "type";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamea_prop = "a_prop";
  std::optional<std::string> type{};
  std::optional<int> a_prop{};

  USERVER_NAMESPACE::formats::json::Value extra;
};

bool operator==(const A& lhs, const A& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const A& value);

A Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<A>);

A Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<A>);

A Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<A>);

A FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<A>);

std::string ToJsonString(const A& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const A& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(const ::ns::A& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   bool hide_brackets = false, std::string_view hide_field_name = {});

struct B {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNametype = "type";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameb_prop = "b_prop";
  std::optional<std::string> type{};
  std::optional<int> b_prop{};

  USERVER_NAMESPACE::formats::json::Value extra;
};

bool operator==(const B& lhs, const B& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const B& value);

B Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<B>);

B Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<B>);

B Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<B>);

B FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<B>);

std::string ToJsonString(const B& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const B& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(const ::ns::B& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   bool hide_brackets = false, std::string_view hide_field_name = {});

struct C {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameversion = "version";
  std::optional<int> version{};
};

bool operator==(const C& lhs, const C& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const C& value);

C Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<C>);

C Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<C>);

C Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<C>);

C FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<C>);

std::string ToJsonString(const C& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const C& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(const ::ns::C& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   bool hide_brackets = false, std::string_view hide_field_name = {});

struct D {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameversion = "version";
  std::optional<int> version{};
};

bool operator==(const D& lhs, const D& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const D& value);

D Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<D>);

D Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<D>);

D Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<D>);

D FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<D>);

std::string ToJsonString(const D& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const D& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(const ::ns::D& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   bool hide_brackets = false, std::string_view hide_field_name = {});

struct IntegerOneOfDiscriminator {
  [[maybe_unused]] static constexpr USERVER_NAMESPACE::chaotic::OneOfIntegerSettings kFoo_Settings = {
      "version", USERVER_NAMESPACE::utils::TrivialSet(
                     [](auto selector) { return selector().template Type<int64_t>().Case(42).Case(52); })};

  using Foo = std::variant<::ns::C, ::ns::D>;

  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamefoo = "foo";
  std::optional<::ns::IntegerOneOfDiscriminator::Foo> foo{};
};

bool operator==(const IntegerOneOfDiscriminator& lhs, const IntegerOneOfDiscriminator& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const IntegerOneOfDiscriminator& value);

IntegerOneOfDiscriminator Parse(USERVER_NAMESPACE::formats::json::Value json,
                                USERVER_NAMESPACE::formats::parse::To<IntegerOneOfDiscriminator>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<IntegerOneOfDiscriminator>) was not generated:
 * ::ns::IntegerOneOfDiscriminator::Foo has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<IntegerOneOfDiscriminator>) was not generated:
 * ::ns::IntegerOneOfDiscriminator::Foo has JSON-specific field "extra" */

IntegerOneOfDiscriminator FromJsonString(std::string_view json,
                                         USERVER_NAMESPACE::formats::parse::To<IntegerOneOfDiscriminator>);

std::string ToJsonString(const IntegerOneOfDiscriminator& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const IntegerOneOfDiscriminator& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(const ::ns::IntegerOneOfDiscriminator& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   bool hide_brackets = false, std::string_view hide_field_name = {});

struct OneOfDiscriminator {
  [[maybe_unused]] static constexpr USERVER_NAMESPACE::chaotic::OneOfStringSettings kFoo_Settings = {
      "type", USERVER_NAMESPACE::utils::TrivialSet(
                  [](auto selector) { return selector().template Type<std::string_view>().Case("aaa").Case("bbb"); })};

  using Foo = std::variant<::ns::A, ::ns::B>;

  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamefoo = "foo";
  std::optional<::ns::OneOfDiscriminator::Foo> foo{};
};

bool operator==(const OneOfDiscriminator& lhs, const OneOfDiscriminator& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const OneOfDiscriminator& value);

OneOfDiscriminator Parse(USERVER_NAMESPACE::formats::json::Value json,
                         USERVER_NAMESPACE::formats::parse::To<OneOfDiscriminator>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<OneOfDiscriminator>) was not generated:
 * ::ns::OneOfDiscriminator::Foo has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<OneOfDiscriminator>) was not generated: ::ns::OneOfDiscriminator::Foo
 * has JSON-specific field "extra" */

OneOfDiscriminator FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<OneOfDiscriminator>);

std::string ToJsonString(const OneOfDiscriminator& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const OneOfDiscriminator& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(const ::ns::OneOfDiscriminator& value, USERVER_NAMESPACE::formats::json::StringBuilder& sw,
                   bool hide_brackets = false, std::string_view hide_field_name = {});

}  // namespace ns

