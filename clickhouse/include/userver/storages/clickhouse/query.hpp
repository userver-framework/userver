#pragma once

/// @file userver/storages/clickhouse/query.hpp
/// @brief @copybrief storages::clickhouse::Query

#include <string>

#include <fmt/format.h>

#include <userver/utils/fmt_compat.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/clickhouse/io/impl/escape.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}

namespace storages::clickhouse {

namespace impl {
class Pool;
}

class Cluster;
class QueryTester;

/// @brief Holds a query and its name.
/// In case query is expected to be executed with parameters,
/// query text should conform to fmt format
class Query final {
 public:
  using Name =
      USERVER_NAMESPACE::utils::StrongTypedef<struct NameTag, std::string>;

  Query(const char* text, std::optional<Name> = std::nullopt);
  Query(std::string text, std::optional<Name> = std::nullopt);

  const std::string& QueryText() const&;

  const std::optional<Name>& QueryName() const&;

  friend class Cluster;
  friend class QueryTester;
  friend class impl::Pool;

 private:
  template <typename... Args>
  Query WithArgs(const Args&... args) const {
    // we should throw on params count mismatch
    // TODO : https://st.yandex-team.ru/TAXICOMMON-5066
    return Query{fmt::format(fmt::runtime(text_), io::impl::Escape(args)...),
                 name_};
  }

  void FillSpanTags(tracing::Span&) const;

  std::string text_;
  std::optional<Name> name_;
};

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
