#include <storages/postgres/internal_pg_types.hpp>

#include <logging/log.hpp>

namespace storages::postgres {

logging::LogHelper& operator<<(logging::LogHelper& lh, Lsn lsn) {
  lh.PutHexShort(lsn.GetUnderlying() >> 32);
  lh << '/';
  lh.PutHexShort(lsn.GetUnderlying() & 0xFFFFFFFF);
  return lh;
}

}  // namespace storages::postgres
