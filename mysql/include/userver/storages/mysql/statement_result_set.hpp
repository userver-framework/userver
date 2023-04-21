#pragma once

/// @file userver/storages/mysql/statement_result_set.hpp

#include <optional>
#include <vector>

#include <fmt/format.h>

#include <userver/engine/deadline.hpp>
#include <userver/tracing/scope_time.hpp>
#include <userver/utils/fast_pimpl.hpp>

#include <userver/storages/mysql/execution_result.hpp>
#include <userver/storages/mysql/impl/io/extractor.hpp>
#include <userver/storages/mysql/impl/tracing_tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::mysql {

namespace impl {
class StatementFetcher;
}

namespace infra {
class ConnectionPtr;
}

template <typename DbType>
class MappedStatementResultSet;

/// @brief A wrapper for statement execution result.
///
/// This type can't be constructed in user code and is always retrieved from
/// storages::mysql::Cluster or storages::mysql::Transaction methods.
class StatementResultSet final {
 public:
  explicit StatementResultSet(impl::StatementFetcher&& fetcher,
                              tracing::Span&& span);
  StatementResultSet(infra::ConnectionPtr&& connection,
                     impl::StatementFetcher&& fetcher, tracing::Span&& span);
  ~StatementResultSet();

  StatementResultSet(const StatementResultSet& other) = delete;
  StatementResultSet(StatementResultSet&& other) noexcept;

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// `T` is expected to be an aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `T` requirements.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsVector
  // clang-format on
  template <typename T>
  std::vector<T> AsVector() &&;

  // clang-format off
  /// @brief Parse statement result set as std::vector<T>.
  /// Result set is expected to have a single column, `T` is expected to be one
  /// of supported types.
  /// See @ref md_en_userver_mysql_supported_types for supported typed.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsVectorFieldTag
  // clang-format on
  template <typename T>
  std::vector<T> AsVector(FieldTag) &&;

  // clang-format off
  /// @brief Parse statement result set as Container<T>.
  /// `T` is expected to be an aggregate of supported types, `Container` is
  /// expected to meet std::Container requirements.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsContainer
  // clang-format on
  template <typename Container>
  Container AsContainer() &&;

  // clang-format off
  /// @brief Parse statement result as Container<T>.
  /// Result set is expected to have a single column, `T` is expected to be one
  /// of supported types,
  /// `Container` is expected to meed std::Container requirements.
  /// See @ref md_en_userver_mysql_supported_types for supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsContainerFieldTag
  // clang-format on
  template <typename Container>
  Container AsContainer(FieldTag) &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row, `T` is expected to be an
  /// aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `T` requirements.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  /// throws if result set is empty or contains more than one row.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsSingleRow
  // clang-format on
  template <typename T>
  T AsSingleRow() &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have a single row and a single column,
  /// `T` is expected to be one of supported types.
  /// See @ref md_en_userver_mysql_supported_types for supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  /// throws if result set is empty of contains more than one row.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsSingleField
  // clang-format on
  template <typename T>
  T AsSingleField() &&;

  // clang-format off
  /// @brief Parse statement result as std::optional<T>.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be an aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `T` requirements.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch.
  /// throws if result set contains more than one row.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsOptionalSingleRow
  // clang-format on
  template <typename T>
  std::optional<T> AsOptionalSingleRow() &&;

  // clang-format off
  /// @brief Parse statement result as T.
  /// Result set is expected to have not more than one row,
  /// `T` is expected to be one of supported types.
  /// See @ref md_en_userver_mysql_supported_types for supported types.
  ///
  /// UINVARIANTs on columns count not being equal to 1 or type mismatch.
  /// throws if result set contains more than one row.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet AsOptionalSingleField
  // clang-format on
  template <typename T>
  std::optional<T> AsOptionalSingleField() &&;

  // clang-format off
  /// @brief Converts to an interface for on-the-flight mapping
  /// statement result set from `DbType`.
  /// `DbType` is expected to be an aggregate of supported types.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `DbType` requirements.
  ///
  /// @snippet storages/tests/unittests/statement_result_set_mysqltest.cpp uMySQL usage sample - StatementResultSet MapFrom
  // clang-format on
  template <typename DbType>
  MappedStatementResultSet<DbType> MapFrom() &&;

  /// @brief Get statement execution metadata.
  ExecutionResult AsExecutionResult() &&;

 private:
  template <typename T>
  friend class CursorResultSet;

  template <typename DbType>
  friend class MappedStatementResultSet;

  template <typename Container, typename MapFrom, typename ExtractionTag>
  Container DoAsContainerMapped() &&;

  template <typename T, typename ExtractionTag>
  std::optional<T> DoAsOptionalSingleRow() &&;

  bool FetchResult(impl::io::ExtractorBase& extractor);

  struct Impl;
  utils::FastPimpl<Impl, 72, 8> impl_;
};

/// @brief An interface for on-the-flight mapping statement result set from
/// DbType into whatever type you provide without additional allocations.
///
/// `DbType` is expected to be either an aggregate of supported types or a
/// supported type itself for methods taking `FieldTag`; DbType is expected
/// to match DB representation - same amount of columns (1 for `FieldTag`
/// overload) and matching types.
///
/// You are expected to provide a converter function
/// `T Convert(DbType&&, storages::mysql::convert:To<T>)` in either
/// T's namespace or `storages::mysql::convert` namespace
template <typename DbType>
class MappedStatementResultSet final {
 public:
  explicit MappedStatementResultSet(StatementResultSet&& result_set);
  ~MappedStatementResultSet();

  /// @brief Parse statement result set as std::vector<T> using provided
  /// converter function.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `T` requirements.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch for `DbType`.
  template <typename T>
  std::vector<T> AsVector() &&;

  /// @brief Parse statement result set as std::vector<T> using provided
  /// converter function.
  /// See @ref md_en_userver_mysql_supported_types for supported types.
  ///
  /// UINVARIANTs on columns count not being 1 or types mismatch for DbType.
  template <typename T>
  std::vector<T> AsVector(FieldTag) &&;

  /// @brief Parse statement result set as Container<T> using provided
  /// converter function.
  /// See @ref md_en_userver_mysql_supported_types for better understanding of
  /// `Container::value_type` requirements.
  ///
  /// UINVARIANTs on columns count mismatch or types mismatch for `DbType`.
  template <typename Container>
  Container AsContainer() &&;

  /// @brief Parse statement result set as Container<T> using provided
  /// converter function.
  /// See @ref md_en_userver_mysql_supported_types for supported types.
  ///
  /// UINVARIANTs on columns count not being 1 or types mismatch for DbType.
  template <typename Container>
  Container AsContainer(FieldTag) &&;

  template <typename T>
  T AsSingleRow() && {
    static_assert(!sizeof(T),
                  "Not implemented, just use StatementResultSet version and "
                  "convert yourself.");
  }

  template <typename T>
  std::optional<T> AsOptionalSingleRow() && {
    static_assert(!sizeof(T),
                  "Not implemented, just use StatementResultSet version and "
                  "convert yourself.");
  }

 private:
  StatementResultSet result_set_;
};

template <typename T>
std::vector<T> StatementResultSet::AsVector() && {
  return std::move(*this).AsContainer<std::vector<T>>();
}

template <typename T>
std::vector<T> StatementResultSet::AsVector(FieldTag) && {
  return std::move(*this).AsContainer<std::vector<T>>(kFieldTag);
}

template <typename Container>
Container StatementResultSet::AsContainer() && {
  static_assert(meta::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;

  return std::move(*this).DoAsContainerMapped<Container, Row, RowTag>();
}

template <typename Container>
Container StatementResultSet::AsContainer(FieldTag) && {
  static_assert(meta::kIsRange<Container>,
                "The type isn't actually a container");
  using Row = typename Container::value_type;

  return std::move(*this).DoAsContainerMapped<Container, Row, FieldTag>();
}

template <typename T>
T StatementResultSet::AsSingleRow() && {
  auto optional_data = std::move(*this).AsOptionalSingleRow<T>();

  if (!optional_data.has_value()) {
    throw std::runtime_error{"Result set is empty"};
  }

  return std::move(*optional_data);
}

template <typename T>
T StatementResultSet::AsSingleField() && {
  auto optional_data = std::move(*this).AsOptionalSingleField<T>();

  if (!optional_data.has_value()) {
    throw std::runtime_error{"Result set is empty"};
  }

  return std::move(*optional_data);
}

template <typename T>
std::optional<T> StatementResultSet::AsOptionalSingleRow() && {
  return std::move(*this).DoAsOptionalSingleRow<T, RowTag>();
}

template <typename T>
std::optional<T> StatementResultSet::AsOptionalSingleField() && {
  return std::move(*this).DoAsOptionalSingleRow<T, FieldTag>();
}

template <typename Container, typename MapFrom, typename ExtractionTag>
Container StatementResultSet::DoAsContainerMapped() && {
  static_assert(meta::kIsRange<Container>,
                "The type isn't actually a container");
  using Extractor = impl::io::TypedExtractor<Container, MapFrom, ExtractionTag>;

  Extractor extractor{};

  tracing::ScopeTime fetch{impl::tracing::kFetchScope};
  std::move(*this).FetchResult(extractor);
  fetch.Reset();

  return Container{extractor.ExtractData()};
}

template <typename T, typename ExtractionTag>
std::optional<T> StatementResultSet::DoAsOptionalSingleRow() && {
  auto rows = [this] {
    if constexpr (std::is_same_v<RowTag, ExtractionTag>) {
      return std::move(*this).AsVector<T>();
    } else {
      return std::move(*this).AsVector<T>(kFieldTag);
    }
  }();

  if (rows.empty()) {
    return std::nullopt;
  }

  if (rows.size() > 1) {
    throw std::runtime_error{fmt::format(
        "There is more than one row in result set ({} rows)", rows.size())};
  }

  return {std::move(rows.front())};
}

template <typename DbType>
MappedStatementResultSet<DbType>::MappedStatementResultSet(
    StatementResultSet&& result_set)
    : result_set_{std::move(result_set)} {}

template <typename DbType>
MappedStatementResultSet<DbType>::~MappedStatementResultSet() = default;

template <typename DbType>
template <typename T>
std::vector<T> MappedStatementResultSet<DbType>::AsVector() && {
  return std::move(*this).template AsContainer<std::vector<T>>();
}

template <typename DbType>
template <typename T>
std::vector<T> MappedStatementResultSet<DbType>::AsVector(FieldTag) && {
  return std::move(*this).template AsContainer<std::vector<T>>(kFieldTag);
}

template <typename DbType>
template <typename Container>
Container MappedStatementResultSet<DbType>::AsContainer() && {
  return std::move(result_set_)
      .DoAsContainerMapped<Container, DbType, RowTag>();
}

template <typename DbType>
template <typename Container>
Container MappedStatementResultSet<DbType>::AsContainer(FieldTag) && {
  return std::move(result_set_)
      .DoAsContainerMapped<Container, DbType, FieldTag>();
}

template <typename DbType>
MappedStatementResultSet<DbType> StatementResultSet::MapFrom() && {
  return MappedStatementResultSet<DbType>{std::move(*this)};
}

}  // namespace storages::mysql

USERVER_NAMESPACE_END
