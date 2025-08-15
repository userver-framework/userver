#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include <userver/proto-structs/impl/traits_light.hpp>
#include <userver/utils/meta_light.hpp>

USERVER_NAMESPACE_BEGIN

namespace proto_structs::impl::traits {

template <typename T>
concept Optional = meta::kIsCvInstantiationOf<std::optional, T>;

template <typename T>
concept Vector = meta::kIsCvInstantiationOf<std::vector, T>;

template <typename T>
concept OrderedMap = meta::kIsCvInstantiationOf<std::map, T>;

template <typename T>
concept UnorderedMap = meta::kIsCvInstantiationOf<std::unordered_map, T>;

template <typename T>
concept Map = OrderedMap<T> || UnorderedMap<T>;

template <typename T>
concept ProtoScalar =
    std::is_same_v<std::remove_cv_t<T>, bool> || std::is_same_v<std::remove_cv_t<T>, std::int32_t> ||
    std::is_same_v<std::remove_cv_t<T>, std::int64_t> || std::is_same_v<std::remove_cv_t<T>, std::uint32_t> ||
    std::is_same_v<std::remove_cv_t<T>, std::uint64_t> || std::is_same_v<std::remove_cv_t<T>, float> ||
    std::is_same_v<std::remove_cv_t<T>, double> || std::is_same_v<std::remove_cv_t<T>, std::string> ||
    std::is_enum_v<T>;

}  // namespace proto_structs::impl::traits

USERVER_NAMESPACE_END
