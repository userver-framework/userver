#pragma once

#include <userver/storages/sqlite/sqlite_fwd.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::sqlite::impl {

using InputBindingsPimpl = StatementPtr;

using InputBindingsFwd = Statement;

using OutputBindingsPimpl = StatementBasePtr;

using OutputBindingsFwd = StatementBase;

}  // namespace storages::sqlite::impl

USERVER_NAMESPACE_END
