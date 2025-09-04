#pragma once

/// @file
/// @brief @copybrief proto_structs::HashMap

#include <type_traits>
#include <unordered_map>

#include <userver/utils/str_icase.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs {

/// @brief The hash map container used in userver proto structs by default.
///
/// Currently implemented as just `std::unordered_map`. Please don't assume it! For example:
///
/// * Don't pass the field to functions as `std::unordered_map`, use `proto_structs::HashMap` instead;
/// * Don't use node and bucket APIs of `std::unordered_map` with these fields.
template <typename Key, typename Value>
using HashMap = std::unordered_map<
    Key,
    Value,
    std::conditional_t<std::is_convertible_v<Key, std::string_view>, utils::StrCaseHash, std::hash<Key>>>;

}  // namespace proto_structs

USERVER_NAMESPACE_END
