#pragma once

#include "enum_fwd.hpp"

#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct Enum {
  enum class Foo {
    kOne,
    kTwo,
    kThree,
  };

  static constexpr Foo kFooValues[] = {
      Foo::kOne,
      Foo::kTwo,
      Foo::kThree,
  };

  std::optional<ns::Enum::Foo> foo{};
};

bool operator==(const ns::Enum& lhs, const ns::Enum& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::Enum::Foo& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::Enum& value);

Enum::Foo Parse(USERVER_NAMESPACE::formats::json::Value json,
                USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo>);

Enum Parse(USERVER_NAMESPACE::formats::json::Value json,
           USERVER_NAMESPACE::formats::parse::To<ns::Enum>);

Enum::Foo Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo>);

Enum Parse(USERVER_NAMESPACE::formats::yaml::Value json,
           USERVER_NAMESPACE::formats::parse::To<ns::Enum>);

Enum::Foo Parse(USERVER_NAMESPACE::yaml_config::Value json,
                USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo>);

Enum Parse(USERVER_NAMESPACE::yaml_config::Value json,
           USERVER_NAMESPACE::formats::parse::To<ns::Enum>);

Enum::Foo FromString(std::string_view value,
                     USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo>);

Enum::Foo Parse(std::string_view value,
                USERVER_NAMESPACE::formats::parse::To<ns::Enum::Foo>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::Enum::Foo& value, USERVER_NAMESPACE::formats::serialize::To<
                                    USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::Enum& value, USERVER_NAMESPACE::formats::serialize::To<
                               USERVER_NAMESPACE::formats::json::Value>);

std::string ToString(ns::Enum::Foo value);

}  // namespace ns
