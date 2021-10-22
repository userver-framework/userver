#include <storages/postgres/internal_pg_types.hpp>

#include <userver/logging/log.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres {

logging::LogHelper& operator<<(logging::LogHelper& lh, Lsn lsn) {
  lh << logging::HexShort{lsn.GetUnderlying() >> 32};
  lh << '/';
  lh << logging::HexShort{lsn.GetUnderlying() & 0xFFFFFFFF};
  return lh;
}

}  // namespace storages::postgres

USERVER_NAMESPACE_END
