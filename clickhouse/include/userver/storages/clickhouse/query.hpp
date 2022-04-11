#pragma once

#include <string>

#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/clickhouse/io/impl/escape.hpp>

#include <fmt/format.h>

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

  template <typename... Args>
  Query WithArgs(const Args&... args) const {
    // we should throw on params count mismatch
    // TODO : https://st.yandex-team.ru/TAXICOMMON-5066
    return Query{fmt::format(text_, io::impl::Escape(args)...), name_};
  }

 private:
  std::string text_;
  std::optional<Name> name_;
};

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
