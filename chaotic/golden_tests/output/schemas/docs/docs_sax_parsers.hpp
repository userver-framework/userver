#pragma once

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/array.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/ref.hpp>
#include <userver/chaotic/sax_parser.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/validators_pattern.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/json/serialize_variant.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "docs.hpp"

namespace ns {

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::Circle, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::Circle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::Circle::kind, ::ns::Circle::kFieldNamekind>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::Circle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<double>>,
        &::ns::Circle::radius, ::ns::Circle::kFieldNameradius>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::Circle>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::Object, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<::ns::Object,
                                      USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<int>>,
                                      &::ns::Object::foo, ::ns::Object::kFieldNamefoo>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::Object, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::Object::bar, ::ns::Object::kFieldNamebar>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::Object>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::ObjectCpp, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::ObjectCpp, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::ObjectCpp::some_hyphenated_key, ::ns::ObjectCpp::kFieldNamesome_hyphenated_key>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::ObjectCpp>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::Rectangle, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::Rectangle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::Rectangle::kind, ::ns::Rectangle::kFieldNamekind>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::Rectangle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<double>>,
        &::ns::Rectangle::width, ::ns::Rectangle::kFieldNamewidth>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::Rectangle, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<double>>,
        &::ns::Rectangle::height, ::ns::Rectangle::kFieldNameheight>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::Rectangle>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::TreeNode, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::TreeNode, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::TreeNode::data, ::ns::TreeNode::kFieldNamedata>,
    USERVER_NAMESPACE::chaotic::Field<::ns::TreeNode,
                                      USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Ref<
                                          USERVER_NAMESPACE::chaotic::Primitive<::ns::TreeNode>>>,
                                      &::ns::TreeNode::left, ::ns::TreeNode::kFieldNameleft>,
    USERVER_NAMESPACE::chaotic::Field<::ns::TreeNode,
                                      USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Ref<
                                          USERVER_NAMESPACE::chaotic::Primitive<::ns::TreeNode>>>,
                                      &::ns::TreeNode::right, ::ns::TreeNode::kFieldNameright>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::TreeNode>);

}  // namespace ns

