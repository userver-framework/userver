#pragma once

#include "docs.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/array.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/ref.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/validators_pattern.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>

namespace ns {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_CircleFieldNamekind = "kind";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_CircleFieldNameradius = "radius";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::Circle, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson, USERVER_NAMESPACE::chaotic::Field<::ns::Circle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::Circle::kind, k_ns_CircleFieldNamekind>, USERVER_NAMESPACE::chaotic::Field<::ns::Circle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<double>>, &::ns::Circle::radius, k_ns_CircleFieldNameradius>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<Circle>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_ObjectFieldNamefoo = "foo";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_ObjectFieldNamebar = "bar";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::Object, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::Object, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::ns::Object::foo, k_ns_ObjectFieldNamefoo>, USERVER_NAMESPACE::chaotic::Field<::ns::Object, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::Object::bar, k_ns_ObjectFieldNamebar>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<Object>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_ObjectCppFieldNamesome_hyphenated_key = "some-hyphenated-key";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::ObjectCpp, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::ObjectCpp, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::ObjectCpp::some_hyphenated_key, k_ns_ObjectCppFieldNamesome_hyphenated_key>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<ObjectCpp>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_RectangleFieldNamekind = "kind";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_RectangleFieldNamewidth = "width";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_RectangleFieldNameheight = "height";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::Rectangle, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson, USERVER_NAMESPACE::chaotic::Field<::ns::Rectangle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::Rectangle::kind, k_ns_RectangleFieldNamekind>, USERVER_NAMESPACE::chaotic::Field<::ns::Rectangle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<double>>, &::ns::Rectangle::width, k_ns_RectangleFieldNamewidth>, USERVER_NAMESPACE::chaotic::Field<::ns::Rectangle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<double>>, &::ns::Rectangle::height, k_ns_RectangleFieldNameheight>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<Rectangle>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_TreeNodeFieldNamedata = "data";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_TreeNodeFieldNameleft = "left";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_TreeNodeFieldNameright = "right";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::TreeNode, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::TreeNode, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::TreeNode::data, k_ns_TreeNodeFieldNamedata>, USERVER_NAMESPACE::chaotic::Field<::ns::TreeNode, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Ref<USERVER_NAMESPACE::chaotic::Primitive<::ns::TreeNode>>>, &::ns::TreeNode::left, k_ns_TreeNodeFieldNameleft>, USERVER_NAMESPACE::chaotic::Field<::ns::TreeNode, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Ref<USERVER_NAMESPACE::chaotic::Primitive<::ns::TreeNode>>>, &::ns::TreeNode::right, k_ns_TreeNodeFieldNameright>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<TreeNode>);

}  // namespace ns

