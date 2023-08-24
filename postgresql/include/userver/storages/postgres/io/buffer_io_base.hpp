#pragma once

#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/utils/void_t.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io::detail {

template <typename T, typename = USERVER_NAMESPACE::utils::void_t<>>
struct ShouldInitMapping : std::false_type {};

template <typename T>
struct ShouldInitMapping<T,
                         USERVER_NAMESPACE::utils::void_t<decltype(T::init_)>>
    : std::true_type {};

template <typename T>
struct BufferParserBase {
  using ValueType = T;

  ValueType& value;
  explicit BufferParserBase(ValueType& v) : value{v} {
    using PgMapping = CppToPg<ValueType>;
    if constexpr (ShouldInitMapping<PgMapping>{}) {
      ForceReference(PgMapping::init_);
    }
  }
};

template <typename T>
struct BufferParserBase<T&&> {
  using ValueType = T;

  ValueType value;
  explicit BufferParserBase(ValueType&& v) : value{std::move(v)} {
    using PgMapping = CppToPg<ValueType>;
    if constexpr (ShouldInitMapping<PgMapping>{}) {
      ForceReference(PgMapping::init_);
    }
  }
};

template <typename T>
struct BufferFormatterBase {
  using ValueType = T;
  const ValueType& value;
  explicit BufferFormatterBase(const ValueType& v) : value{v} {}
};

}  // namespace storages::postgres::io::detail

USERVER_NAMESPACE_END
