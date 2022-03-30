#pragma once

#include <string>

#include <userver/utils/strong_typedef.hpp>

USERVER_NAMESPACE_BEGIN

namespace tracing {
class Span;
}

namespace storages::clickhouse {

class Query final {
 public:
  using Name =
      USERVER_NAMESPACE::utils::StrongTypedef<struct NameTag, std::string>;

  Query(const char* text, std::optional<Name> = std::nullopt);
  Query(std::string text, std::optional<Name> = std::nullopt);

  const std::string& QueryText() const&;

  const std::optional<Name>& QueryName() const&;

  void FillSpanTags(tracing::Span&) const;

 private:
  std::string text_;
  std::optional<Name> name_;
};

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
