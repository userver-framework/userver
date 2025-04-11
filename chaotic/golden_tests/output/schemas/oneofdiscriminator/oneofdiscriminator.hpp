#pragma once

#include "oneofdiscriminator_fwd.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/oneof_with_discriminator.hpp>
#include <userver/formats/json/value.hpp>
#include <variant>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct A {
    std::optional<std::string> type{};
    std::optional<int> a_prop{};

    USERVER_NAMESPACE::formats::json::Value extra;
};

bool operator==(const ::ns::A& lhs, const ::ns::A& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const ::ns::A& value);

A Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::A>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<::ns::A>) was not generated: ::ns::A has JSON-specific field
 * "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<::ns::A>) was not generated: ::ns::A has JSON-specific field "extra"
 */

USERVER_NAMESPACE::formats::json::Value
Serialize(const ::ns::A& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct B {
    std::optional<std::string> type{};
    std::optional<int> b_prop{};

    USERVER_NAMESPACE::formats::json::Value extra;
};

bool operator==(const ::ns::B& lhs, const ::ns::B& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const ::ns::B& value);

B Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::B>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<::ns::B>) was not generated: ::ns::B has JSON-specific field
 * "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<::ns::B>) was not generated: ::ns::B has JSON-specific field "extra"
 */

USERVER_NAMESPACE::formats::json::Value
Serialize(const ::ns::B& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct C {
    std::optional<int> version{};
};

bool operator==(const ::ns::C& lhs, const ::ns::C& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const ::ns::C& value);

C Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::C>);

C Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::C>);

C Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::C>);

USERVER_NAMESPACE::formats::json::Value
Serialize(const ::ns::C& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct D {
    std::optional<int> version{};
};

bool operator==(const ::ns::D& lhs, const ::ns::D& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const ::ns::D& value);

D Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::D>);

D Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::D>);

D Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::D>);

USERVER_NAMESPACE::formats::json::Value
Serialize(const ::ns::D& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct IntegerOneOfDiscriminator {
    [[maybe_unused]] static constexpr USERVER_NAMESPACE::chaotic::OneOfIntegerSettings kFoo_Settings = {
        "version",
        USERVER_NAMESPACE::utils::TrivialSet([](auto selector) {
            return selector().template Type<int64_t>().Case(42).Case(52);
        })};

    using Foo = std::variant<::ns::C, ::ns::D>;

    std::optional<::ns::IntegerOneOfDiscriminator::Foo> foo{};
};

bool operator==(const ::ns::IntegerOneOfDiscriminator& lhs, const ::ns::IntegerOneOfDiscriminator& rhs);

USERVER_NAMESPACE::logging::LogHelper&
operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const ::ns::IntegerOneOfDiscriminator& value);

IntegerOneOfDiscriminator
Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::IntegerOneOfDiscriminator>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<::ns::IntegerOneOfDiscriminator>) was not generated:
 * ::ns::IntegerOneOfDiscriminator::Foo has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<::ns::IntegerOneOfDiscriminator>) was not generated:
 * ::ns::IntegerOneOfDiscriminator::Foo has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value
Serialize(const ::ns::IntegerOneOfDiscriminator& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct OneOfDiscriminator {
    [[maybe_unused]] static constexpr USERVER_NAMESPACE::chaotic::OneOfStringSettings kFoo_Settings = {
        "type",
        USERVER_NAMESPACE::utils::TrivialSet([](auto selector) {
            return selector().template Type<std::string_view>().Case("aaa").Case("bbb");
        })};

    using Foo = std::variant<::ns::A, ::ns::B>;

    std::optional<::ns::OneOfDiscriminator::Foo> foo{};
};

bool operator==(const ::ns::OneOfDiscriminator& lhs, const ::ns::OneOfDiscriminator& rhs);

USERVER_NAMESPACE::logging::LogHelper&
operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const ::ns::OneOfDiscriminator& value);

OneOfDiscriminator
Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<::ns::OneOfDiscriminator>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<::ns::OneOfDiscriminator>) was not generated:
 * ::ns::OneOfDiscriminator::Foo has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<::ns::OneOfDiscriminator>) was not generated:
 * ::ns::OneOfDiscriminator::Foo has JSON-specific field "extra" */

USERVER_NAMESPACE::formats::json::Value
Serialize(const ::ns::OneOfDiscriminator& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns
