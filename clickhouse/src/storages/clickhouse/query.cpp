#include <userver/storages/clickhouse/query.hpp>

#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::clickhouse {

Query::Query(const char* text, std::optional<Query::Name> name)
    : text_{text}, name_{std::move(name)} {}

Query::Query(std::string text, std::optional<Query::Name> name)
    : text_{std::move(text)}, name_{std::move(name)} {}

const std::string& Query::QueryText() const& { return text_; }

const std::optional<Query::Name>& Query::QueryName() const& { return name_; }

void Query::FillSpanTags(tracing::Span& span) const {
  if (name_.has_value()) {
    span.AddTag(tracing::kDatabaseStatementName, name_->GetUnderlying());
  }
}

}  // namespace storages::clickhouse

USERVER_NAMESPACE_END
