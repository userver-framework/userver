#pragma once

#include <string_view>

#include <libpq-fe.h>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

/// Returns nonlocalized severity, plain severity or an empty sv, subject to
/// availability.
std::string_view GetMachineReadableSeverity(const PGresult*);

}  // namespace storages::postgres::detail

USERVER_NAMESPACE_END
