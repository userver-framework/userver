#pragma once

#include <string_view>

#include <libpq-fe.h>

namespace storages::postgres::detail {

/// Returns nonlocalized severity, plain severity or an empty sv, subject to
/// availability.
std::string_view GetMachineReadableSeverity(const PGresult*);

}  // namespace storages::postgres::detail
