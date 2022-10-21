#pragma once

#include <userver/engine/deadline.hpp>

struct pg_cancel;
// NOLINTNEXTLINE(modernize-use-using)
typedef pg_cancel PGcancel;

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::detail {

void Cancel(PGcancel* cn, engine::Deadline);

}

USERVER_NAMESPACE_END
