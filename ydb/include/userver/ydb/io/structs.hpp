#pragma once

#include <ydb-cpp-sdk/client/result/result.h>
#include <ydb-cpp-sdk/client/value/value.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <boost/pfr/core.hpp>
#include <boost/pfr/core_name.hpp>

#include <userver/utils/assert.hpp>
#include <userver/utils/constexpr_indices.hpp>
#include <userver/utils/enumerate.hpp>
#include <userver/utils/trivial_map.hpp>

#include <userver/ydb/exceptions.hpp>
#include <userver/ydb/impl/cast.hpp>
#include <userver/ydb/io/traits.hpp>

USERVER_NAMESPACE_BEGIN

namespace ydb {

namespace impl {

struct NotStruct final {};

template <typename T>
constexpr decltype(T::kYdbMemberNames) DetectStructMemberNames() noexcept {
  return T::kYdbMemberNames;
}

template <typename T, typename... Args>
constexpr NotStruct DetectStructMemberNames(Args&&...) noexcept {
  return NotStruct{};
}

}  // namespace impl

/// @see ydb::CustomMemberNames
struct CustomMemberName final {
  std::string_view cpp_name;
  std::string_view ydb_name;
};

/// @brief Specifies C++ to YDB struct member names mapping. It's enough to only
/// specify the names that are different between C++ and YDB.
/// @see ydb::kStructMemberNames
template <std::size_t N>
struct StructMemberNames final {
  CustomMemberName custom_names[N];
};

// clang-format off
StructMemberNames() -> StructMemberNames<0>;
// clang-format on

template <std::size_t N>
StructMemberNames(CustomMemberName (&&)[N]) -> StructMemberNames<N>;

/// @brief Customization point for YDB serialization of structs.
///
/// In order to get serialization for a struct, you need to define
/// `kYdbMemberNames` inside it:
///
/// @snippet ydb/small_table.hpp  struct sample
///
/// Field names can be overridden:
///
/// @snippet ydb/tests/struct_io_test.cpp  custom names
///
/// To enable YDB serialization for an external struct, specialize
/// ydb::kStructMemberNames for it:
///
/// @snippet ydb/tests/struct_io_test.cpp  external specialization
///
/// @warning The struct must not reside in an anonymous namespace, otherwise
/// struct member names detection will break.
///
/// For extra fields on C++ side, parsing throws ydb::ParseError.
/// For extra fields on YDB side, parsing throws ydb::ParseError.
template <typename T>
constexpr auto kStructMemberNames = impl::DetectStructMemberNames<T>();

namespace impl {

template <typename T, std::size_t N>
constexpr auto WithDeducedNames(const StructMemberNames<N>& given_names) {
  auto names = boost::pfr::names_as_array<T>();
  for (const CustomMemberName& entry : given_names.custom_names) {
    std::size_t count = 0;
    for (auto& name : names) {
      if (name == entry.cpp_name) {
        name = entry.ydb_name;
        ++count;
        break;
      }
    }
    if (count != 1) {
      throw BaseError(
          "In a StructMemberNames, each cpp_name must match the C++ name of "
          "exactly 1 member of struct T");
    }
  }
  return names;
}

template <typename T, std::size_t... Indices>
constexpr auto MakeTupleOfOptionals(std::index_sequence<Indices...>) {
  return std::tuple<
      std::optional<boost::pfr::tuple_element_t<Indices, T>>...>();
}

template <typename T, typename Tuple, std::size_t... Indices>
constexpr auto MakeFromTupleOfOptionals(Tuple&& tuple,
                                        std::index_sequence<Indices...>) {
  return T{*std::get<Indices>(std::forward<Tuple>(tuple))...};
}

}  // namespace impl

template <typename T>
struct ValueTraits<
    T, std::enable_if_t<!std::is_same_v<decltype(kStructMemberNames<T>),
                                        const impl::NotStruct>>> {
  static_assert(std::is_aggregate_v<T>);
  static constexpr auto kFieldNames =
      impl::WithDeducedNames<T>(kStructMemberNames<T>);
  static constexpr auto kFieldNamesSet = utils::MakeTrivialSet<kFieldNames>();
  static constexpr auto kFieldsCount = kFieldNames.size();

  static T Parse(NYdb::TValueParser& parser, const ParseContext& context) {
    auto parsed_fields =
        impl::MakeTupleOfOptionals<T>(std::make_index_sequence<kFieldsCount>{});
    parser.OpenStruct();

    while (parser.TryNextMember()) {
      const auto& field_name = parser.GetMemberName();
      const auto index = kFieldNamesSet.GetIndex(field_name);
      if (!index.has_value()) {
        throw ColumnParseError(
            context.column_name,
            fmt::format("Unexpected field name '{}' for '{}' struct type, "
                        "expected one of: {}",
                        field_name, compiler::GetTypeName<T>(),
                        fmt::join(kFieldNames, ", ")));
      }
      utils::WithConstexprIndex<kFieldsCount>(*index, [&](auto index_c) {
        auto& field = std::get<decltype(index_c)::value>(parsed_fields);
        using FieldType = typename std::decay_t<decltype(field)>::value_type;
        field.emplace(ydb::Parse<FieldType>(parser, context));
      });
    }

    parser.CloseStruct();

    std::string_view missing_field;
    utils::ForEachIndex<kFieldsCount>([&](auto index_c) {
      if (!std::get<decltype(index_c)::value>(parsed_fields).has_value()) {
        missing_field = kFieldNames[index_c];
      }
    });
    if (!missing_field.empty()) {
      throw ColumnParseError(
          context.column_name,
          fmt::format("Missing field '{}' for '{}' struct type", missing_field,
                      compiler::GetTypeName<T>()));
    }

    return impl::MakeFromTupleOfOptionals<T>(
        std::move(parsed_fields), std::make_index_sequence<kFieldsCount>{});
  }

  template <typename Builder>
  static void Write(NYdb::TValueBuilderBase<Builder>& builder, const T& value) {
    builder.BeginStruct();
    boost::pfr::for_each_field(value, [&](const auto& field, std::size_t i) {
      builder.AddMember(impl::ToString(kFieldNames[i]));
      ydb::Write(builder, field);
    });
    builder.EndStruct();
  }

  static NYdb::TType MakeType() {
    NYdb::TTypeBuilder builder;
    builder.BeginStruct();
    utils::ForEachIndex<kFieldsCount>([&](auto index_c) {
      builder.AddMember(
          impl::ToString(kFieldNames[index_c]),
          ValueTraits<boost::pfr::tuple_element_t<decltype(index_c)::value,
                                                  T>>::MakeType());
    });
    builder.EndStruct();
    return builder.Build();
  }
};

namespace impl {

template <typename T>
struct StructRowParser final {
  static_assert(std::is_aggregate_v<T>);
  static constexpr auto kFieldNames =
      impl::WithDeducedNames<T>(kStructMemberNames<T>);
  static constexpr auto kFieldsCount = kFieldNames.size();

  static std::unique_ptr<std::size_t[]> MakeCppToYdbFieldMapping(
      NYdb::TResultSetParser& parser) {
    auto result = std::make_unique<std::size_t[]>(kFieldsCount);
    for (const auto [pos, field_name] : utils::enumerate(kFieldNames)) {
      const auto column_index = parser.ColumnIndex(impl::ToString(field_name));
      if (column_index == -1) {
        throw ParseError(fmt::format("Missing column '{}' for '{}' struct type",
                                     field_name, compiler::GetTypeName<T>()));
      }
      result[pos] = static_cast<std::size_t>(column_index);
    }

    const auto columns_count = parser.ColumnsCount();
    UASSERT(columns_count >= kFieldsCount);
    if (columns_count != kFieldsCount) {
      throw ParseError(fmt::format(
          "Unexpected extra columns while parsing row to '{}' struct type",
          compiler::GetTypeName<T>()));
    }

    return result;
  }

  static T ParseRow(NYdb::TResultSetParser& parser,
                    const std::unique_ptr<std::size_t[]>& cpp_to_ydb_mapping) {
    return ParseRowImpl(parser, cpp_to_ydb_mapping,
                        std::make_index_sequence<kFieldsCount>{});
  }

  template <std::size_t... Indices>
  static T ParseRowImpl(
      NYdb::TResultSetParser& parser,
      const std::unique_ptr<std::size_t[]>& cpp_to_ydb_mapping,
      std::index_sequence<Indices...>) {
    return T{
        Parse<boost::pfr::tuple_element_t<Indices, T>>(
            parser.ColumnParser(cpp_to_ydb_mapping[Indices]),
            ParseContext{/*column_name=*/kFieldNames[Indices]})...,
    };
  }
};

}  // namespace impl

}  // namespace ydb

USERVER_NAMESPACE_END
