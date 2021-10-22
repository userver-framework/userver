#include <storages/postgres/detail/pg_message_severity.hpp>

#include <userver/logging/log.hpp>

// Added in 9.6
#ifndef PG_DIAG_SEVERITY_NONLOCALIZED
#define PG_DIAG_SEVERITY_NONLOCALIZED 'V'
#endif

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

std::string_view GetMachineReadableSeverity(const PGresult* result) {
  if (!result) {
    LOG_DEBUG() << "Got null result";
    return {};
  }

  const char* severity_field =
      PQresultErrorField(result, PG_DIAG_SEVERITY_NONLOCALIZED);
  if (severity_field) return severity_field;

  LOG_TRACE() << "Nonlocalized severity unavailable";
  severity_field = PQresultErrorField(result, PG_DIAG_SEVERITY);
  if (severity_field) return severity_field;

  LOG_DEBUG() << "Result has no severity (not an error/notice?)";
  return {};
}

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
