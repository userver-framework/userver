#pragma once

#include "allof_fwd.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {
namespace impl {}  // namespace impl
}  // namespace ns

namespace ns {

struct AllOf {
  struct Foo__P0 {
    std::optional<std::string> foo{};

    USERVER_NAMESPACE::formats::json::Value extra;
  };

  struct Foo__P1 {
    std::optional<int> bar{};

    USERVER_NAMESPACE::formats::json::Value extra;
  };

  struct Foo : public ns::AllOf::Foo__P0, public ns::AllOf::Foo__P1 {
    Foo() = default;

    Foo(ns::AllOf::Foo__P0&& a0, ns::AllOf::Foo__P1&& a1)
        : ns::AllOf::Foo__P0(std::move(a0)),
          ns::AllOf::Foo__P1(std::move(a1)) {}
  };

  std::optional<ns::AllOf::Foo> foo{};
};

bool operator==(const ns::AllOf::Foo__P0& lhs, const ns::AllOf::Foo__P0& rhs);

bool operator==(const ns::AllOf::Foo__P1& lhs, const ns::AllOf::Foo__P1& rhs);

bool operator==(const ns::AllOf::Foo& lhs, const ns::AllOf::Foo& rhs);

bool operator==(const ns::AllOf& lhs, const ns::AllOf& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::AllOf::Foo__P0& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::AllOf::Foo__P1& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::AllOf::Foo& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::AllOf& value);

AllOf::Foo__P0 Parse(USERVER_NAMESPACE::formats::json::Value json,
                     USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo__P0>);

AllOf::Foo__P1 Parse(USERVER_NAMESPACE::formats::json::Value json,
                     USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo__P1>);

AllOf::Foo Parse(USERVER_NAMESPACE::formats::json::Value json,
                 USERVER_NAMESPACE::formats::parse::To<ns::AllOf::Foo>);

AllOf Parse(USERVER_NAMESPACE::formats::json::Value json,
            USERVER_NAMESPACE::formats::parse::To<ns::AllOf>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<ns::AllOf>) was not
 * generated: ns::AllOf@Foo__P0 has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<ns::AllOf>) was not
 * generated: ns::AllOf@Foo__P0 has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::AllOf::Foo__P0& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::AllOf::Foo__P1& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::AllOf::Foo& value, USERVER_NAMESPACE::formats::serialize::To<
                                     USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::AllOf& value, USERVER_NAMESPACE::formats::serialize::To<
                                USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns
