#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/type_bundle_hpp.hpp>
#include <userver/formats/json/value.hpp>

#include "allof_fwd.hpp"

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

    struct Foo : public AllOf::Foo__P0, public AllOf::Foo__P1 {
        Foo() = default;

        Foo(AllOf::Foo__P0&& a0, AllOf::Foo__P1&& a1) : AllOf::Foo__P0(std::move(a0)), AllOf::Foo__P1(std::move(a1)) {}
    };

    std::optional<::ns::AllOf::Foo> foo{};
};

bool operator==(const AllOf::Foo__P0& lhs, const AllOf::Foo__P0& rhs);

bool operator==(const AllOf::Foo__P1& lhs, const AllOf::Foo__P1& rhs);

bool operator==(const AllOf::Foo& lhs, const AllOf::Foo& rhs);

bool operator==(const AllOf& lhs, const AllOf& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const AllOf::Foo__P0& value
);

USERVER_NAMESPACE::logging::LogHelper& operator<<(
    USERVER_NAMESPACE::logging::LogHelper& lh,
    const AllOf::Foo__P1& value
);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const AllOf::Foo& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const AllOf& value);

AllOf::Foo__P0
Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P0>);

AllOf::Foo__P1
Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<AllOf::Foo__P1>);

AllOf::Foo Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<AllOf::Foo>);

AllOf Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<AllOf>);

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<AllOf>) was not generated: ::ns::AllOf::Foo__P0 has JSON-specific
 * field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<AllOf>) was not generated: ::ns::AllOf::Foo__P0 has JSON-specific
 * field "extra" */

USERVER_NAMESPACE::formats::json::Value
Serialize(const AllOf::Foo__P0& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value
Serialize(const AllOf::Foo__P1& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value
Serialize(const AllOf::Foo& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value
Serialize(const AllOf& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns
