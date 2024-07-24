#pragma once

#include "oneofdiscriminator_fwd.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/oneof_with_discriminator.hpp>
#include <variant>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {
namespace impl {}  // namespace impl
}  // namespace ns

namespace ns {

struct A {
  std::optional<std::string> type{};
  std::optional<int> a_prop{};

  USERVER_NAMESPACE::formats::json::Value extra;
};

bool operator==(const ns::A& lhs, const ns::A& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::A& value);

A Parse(USERVER_NAMESPACE::formats::json::Value json,
        USERVER_NAMESPACE::formats::parse::To<ns::A>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<ns::A>) was not generated:
 * ns::A has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<ns::A>) was not generated:
 * ns::A has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::A& value, USERVER_NAMESPACE::formats::serialize::To<
                            USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns

namespace ns {
namespace impl {}  // namespace impl
}  // namespace ns

namespace ns {

struct B {
  std::optional<std::string> type{};
  std::optional<int> b_prop{};

  USERVER_NAMESPACE::formats::json::Value extra;
};

bool operator==(const ns::B& lhs, const ns::B& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh, const ns::B& value);

B Parse(USERVER_NAMESPACE::formats::json::Value json,
        USERVER_NAMESPACE::formats::parse::To<ns::B>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<ns::B>) was not generated:
 * ns::B has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<ns::B>) was not generated:
 * ns::B has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::B& value, USERVER_NAMESPACE::formats::serialize::To<
                            USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns

namespace ns {
namespace impl {

[[maybe_unused]] static constexpr USERVER_NAMESPACE::chaotic::OneOfSettings
    kns__OneOfDiscriminator__Foo_Settings = {
        "type", USERVER_NAMESPACE::utils::TrivialSet([](auto selector) {
          return selector().template Type<std::string>().Case("aaa").Case(
              "bbb");
        })};

}  // namespace impl
}  // namespace ns

namespace ns {

struct OneOfDiscriminator {
  using Foo = std::variant<ns::A, ns::B>;

  std::optional<ns::OneOfDiscriminator::Foo> foo{};
};

bool operator==(const ns::OneOfDiscriminator& lhs,
                const ns::OneOfDiscriminator& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const ns::OneOfDiscriminator& value);

OneOfDiscriminator Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<ns::OneOfDiscriminator>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<ns::OneOfDiscriminator>)
 * was not generated: ns::OneOfDiscriminator@Foo has JSON-specific field "extra"
 */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<ns::OneOfDiscriminator>) was
 * not generated: ns::OneOfDiscriminator@Foo has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ns::OneOfDiscriminator& value,
    USERVER_NAMESPACE::formats::serialize::To<
        USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns
