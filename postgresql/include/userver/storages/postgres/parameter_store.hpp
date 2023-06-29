#pragma once

/// @file userver/storages/postgres/parameter_store.hpp
/// @brief @copybrief storages::postgres::ParameterStore

#include <userver/storages/postgres/detail/query_parameters.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>
#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

/// @ingroup userver_containers
///
/// @brief Class for dynamic PostgreSQL parameter list construction.
///
/// Typical use case for this container is to keep parameters around while the
/// query is being constructed on the fly:
/// @snippet storages/postgres/tests/interval_pgtest.cpp Parameters store sample
///
/// Note that storages::postgres::Cluster::Execute with explicitly provided
/// arguments works slightly faster:
/// @snippet storages/postgres/tests/landing_test.cpp Exec sample
class ParameterStore {
 public:
  ParameterStore() = default;
  ParameterStore(const ParameterStore&) = delete;
  ParameterStore(ParameterStore&&) = default;
  ParameterStore& operator=(const ParameterStore&) = delete;
  ParameterStore& operator=(ParameterStore&&) = default;

  /// @brief Adds a parameter to the end of the parameter list.
  /// @note Currently only built-in/system types are supported.
  template <typename T>
  ParameterStore& PushBack(const T& param) {
    static_assert(
        io::traits::kIsMappedToSystemType<T>,
        "Currently only built-in types can be used in ParameterStore");
    data_.Write(kNoUserTypes, param);
    return *this;
  }

  /// Returns whether the parameter list is empty.
  bool IsEmpty() const { return data_.Size() == 0; }

  /// Returns current size of the list.
  size_t Size() const { return data_.Size(); }

  /// @cond
  const detail::DynamicQueryParameters& GetInternalData() const {
    return data_;
  }
  /// @endcond

 private:
  static UserTypes kNoUserTypes;

  detail::DynamicQueryParameters data_;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
