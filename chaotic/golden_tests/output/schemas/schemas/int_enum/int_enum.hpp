#pragma once

#include "int_enum_fwd.hpp"

#include <optional>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct Enum {
  enum class Foo {
    kInner = 0,  // Inner
    kLeft = 1,   // Left
    kRight = 2,  // Right
    kOuter = 3,  // Outer
    k5 = 5,      // 5
  };

  static constexpr Foo kFooValues[] = {
      Foo::kInner,
      Foo::kLeft,
      Foo::kRight,
      Foo::kOuter,
      Foo::k5,
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

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::Enum::Foo& value, USERVER_NAMESPACE::formats::serialize::To<
                                    USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::Enum& value, USERVER_NAMESPACE::formats::serialize::To<
                               USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns