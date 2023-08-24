#pragma once

/// @file userver/storages/postgres/query.hpp
/// @brief @copybrief storages::postgres::Query

#include <optional>
#include <string>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}  // namespace tracing

namespace storages::postgres {

/// @brief Holds a query, its name and logging mode
class Query {
 public:
  using Name =
      USERVER_NAMESPACE::utils::StrongTypedef<struct NameTag, std::string>;

  enum class LogMode { kFull, kNameOnly };

  Query() = default;

  Query(const char* statement, std::optional<Name> name = std::nullopt,
        LogMode log_mode = LogMode::kFull);

  Query(std::string statement, std::optional<Name> name = std::nullopt,
        LogMode log_mode = LogMode::kFull);

  const std::optional<Name>& GetName() const;

  const std::string& Statement() const;

  /// @brief Fills provided span with connection info
  void FillSpanTags(tracing::Span&) const;

 private:
  std::string statement_{};
  std::optional<Name> name_{};
  LogMode log_mode_ = LogMode::kFull;
};

}  // namespace storages::postgres

USERVER_NAMESPACE_END
