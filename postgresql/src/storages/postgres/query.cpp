#include <userver/storages/postgres/query.hpp>
#include <userver/tracing/span.hpp>
#include <userver/tracing/tags.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

Query::Query(const char* statement, std::optional<Name> name, LogMode log_mode)
    : statement_(statement), name_(std::move(name)), log_mode_(log_mode) {}

Query::Query(std::string statement, std::optional<Name> name, LogMode log_mode)
    : statement_(std::move(statement)),
      name_(std::move(name)),
      log_mode_(log_mode) {}

const std::optional<Query::Name>& Query::GetName() const { return name_; }

const std::string& Query::Statement() const { return statement_; }

void Query::FillSpanTags(tracing::Span& span) const {
  switch (log_mode_) {
    case LogMode::kFull:
      span.AddTag(tracing::kDatabaseStatement, statement_);
      [[fallthrough]];
    case LogMode::kNameOnly:
      if (name_) {
        span.AddTag(tracing::kDatabaseStatementName, name_->GetUnderlying());
      }
  }
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
