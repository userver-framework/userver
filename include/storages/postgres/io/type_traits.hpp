#pragma once

#include <type_traits>

#include <storages/postgres/detail/is_decl_complete.hpp>
#include <storages/postgres/io/io_fwd.hpp>
#include <storages/postgres/io/traits.hpp>

/// Namespace with metafunctions detecting different type traits needed for
/// postgres io
namespace storages::postgres::io::traits {

template <typename T>
struct IsMappedToSystemType : utils::IsDeclComplete<CppToSystemPg<T>> {};
template <typename T>
constexpr bool kIsMappedToSystemType = IsMappedToSystemType<T>::value;

template <typename T>
struct IsMappedToUserType : utils::IsDeclComplete<CppToUserPg<T>> {};
template <typename T>
constexpr bool kIsMappedToUserType = IsMappedToUserType<T>::value;

template <typename T>
struct IsMappedToPg
    : std::integral_constant<bool, kIsMappedToUserType<T> ||
                                       kIsMappedToSystemType<T>> {};
template <typename T>
constexpr bool kIsMappedToPg = IsMappedToPg<T>::value;

template <typename T>
struct IsSpecialMapping : std::false_type {};
template <typename T>
constexpr bool kIsSpecialMapping = IsSpecialMapping<T>::value;

}  // namespace storages::postgres::io::traits
