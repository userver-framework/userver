#pragma once

#include "docs_fwd.hpp"

#include <cstdint>
#include <fmt/core.h>
#include <optional>
#include <string>
#include <userver/chaotic/io/boost/uuids/uuid.hpp>
#include <userver/chaotic/io/std/size_t.hpp>
#include <userver/chaotic/io/std/vector.hpp>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/oneof_with_discriminator.hpp>
#include <userver/formats/json/value.hpp>
#include <userver/utils/box.hpp>
#include <variant>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

struct Circle {
    std::optional<std::string> kind{};
    std::optional<double> radius{};
    USERVER_NAMESPACE::formats::json::Value extra{};
};

bool operator==(const Circle & lhs,const Circle & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Circle& value);

Circle Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Circle>);

Circle Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Circle>);

Circle Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Circle>);

Circle FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Circle>);

std::string ToJsonString(const Circle& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Circle& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Circle& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

using EntityUuid = std::string;

static constexpr auto kLatitudeMinimum = -90.0;
static constexpr auto kLatitudeMaximum = 90.0;
using Latitude = double;

struct Object {
    int foo{};
    std::optional<std::string> bar{};
};

bool operator==(const Object & lhs,const Object & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Object& value);

Object Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Object>);

Object Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Object>);

Object Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Object>);

Object FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Object>);

std::string ToJsonString(const Object& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Object& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Object& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct ObjectCpp {
    std::optional<std::string> some_hyphenated_key{};
};

bool operator==(const ObjectCpp & lhs,const ObjectCpp & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const ObjectCpp& value);

ObjectCpp Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<ObjectCpp>);

ObjectCpp Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<ObjectCpp>);

ObjectCpp Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<ObjectCpp>);

ObjectCpp FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<ObjectCpp>);

std::string ToJsonString(const ObjectCpp& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const ObjectCpp& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const ObjectCpp& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

static constexpr auto kPetCountMinimum = 0;
using PetCount = std::int64_t;

static constexpr auto kPetCountSizeMinimum = 0;
using PetCountSize = std::int64_t;

static constexpr std::string_view kPetNamesAPattern = R"--(\w+)--";
using PetNames = std::vector<std::string>;

struct Rectangle {
    std::optional<std::string> kind{};
    std::optional<double> width{};
    std::optional<double> height{};
    USERVER_NAMESPACE::formats::json::Value extra{};
};

bool operator==(const Rectangle & lhs,const Rectangle & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Rectangle& value);

Rectangle Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Rectangle>);

Rectangle Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Rectangle>);

Rectangle Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Rectangle>);

Rectangle FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<Rectangle>);

std::string ToJsonString(const Rectangle& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Rectangle& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Rectangle& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

[[maybe_unused]] static constexpr USERVER_NAMESPACE::chaotic::OneOfStringSettings kShape_Settings = {
    "kind",
    USERVER_NAMESPACE::utils::TrivialSet(
        [](auto selector) {
            return selector().template Type<std::string_view>()
                .Case("circle")
                .Case("rectangle")
            ;
        }
    )
};

using Shape = std::variant<
    ::ns::Circle,
    ::ns::Rectangle
>;

/* Parse(USERVER_NAMESPACE::formats::yaml::Value, To<Shape>) was not generated: ::ns::Shape has JSON-specific field "extra" */

/* Parse(USERVER_NAMESPACE::yaml_config::Value, To<Shape>) was not generated: ::ns::Shape has JSON-specific field "extra" */

using ShouldGreet = bool;

enum class Status {
    kPending,
    kRunning,
    kDone,
};

static constexpr Status kStatusValues [] = {
    Status::kPending,
    Status::kRunning,
    Status::kDone,
};

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const Status& value);

Status Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<Status>);

Status Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<Status>);

Status Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<Status>);

Status Convert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Status>);

std::optional<Status> TryConvert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<Status>) noexcept;

Status Parse(
    std::string_view value,
    USERVER_NAMESPACE::formats::parse::To<Status>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const Status& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const Status& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw);

std::string ToString(Status value);
struct TreeNode {
    std::optional<std::string> data{};
    std::optional<USERVER_NAMESPACE::utils::Box<::ns::TreeNode>> left{};
    std::optional<USERVER_NAMESPACE::utils::Box<::ns::TreeNode>> right{};
};

bool operator==(const TreeNode & lhs,const TreeNode & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const TreeNode& value);

TreeNode Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<TreeNode>);

TreeNode Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<TreeNode>);

TreeNode Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<TreeNode>);

TreeNode FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<TreeNode>);

std::string ToJsonString(const TreeNode& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const TreeNode& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const TreeNode& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

}  // namespace ns

template<>
struct fmt::formatter<ns::Status> {
    fmt::format_context::iterator format(const ns::Status&, fmt::format_context&) const;

    constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }
};

