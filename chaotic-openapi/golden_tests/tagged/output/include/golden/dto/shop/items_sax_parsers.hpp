#pragma once

#include "golden/dto/shop/items.hpp"

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>
#include <userver/chaotic/sax_parser.hpp>
#include <golden/errors/errors_sax_parsers.hpp>

namespace golden {namespace dto {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_golden_dto_ItemFieldNameid = "id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_golden_dto_ItemFieldNametitle = "title";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_golden_dto_ItemFieldNamelast_error = "last_error";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::golden::dto::Item, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::golden::dto::Item, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<int>>, &::golden::dto::Item::id, k_golden_dto_ItemFieldNameid>, USERVER_NAMESPACE::chaotic::Field<::golden::dto::Item, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::golden::dto::Item::title, k_golden_dto_ItemFieldNametitle>, USERVER_NAMESPACE::chaotic::Field<::golden::dto::Item, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<::golden::errors::Error>>, &::golden::dto::Item::last_error, k_golden_dto_ItemFieldNamelast_error>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<Item>);

}  // namespace dto
}  // namespace golden

