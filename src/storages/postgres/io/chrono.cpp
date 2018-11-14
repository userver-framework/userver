#include <storages/postgres/io/chrono.hpp>

namespace storages {
namespace postgres {

namespace {

// Not marked const because mktime requires a non-const pointer
std::tm kPgEpochTm{/* tm_sec */ 0,
                   /* tm_min */ 0,
                   /* tm_hour */ 0,
                   /* tm_mday */ 1,
                   /* tm_mon */ 0,
                   /* tm_year */ 100,
                   /* tm_wday */ 6 /* Saturday */,
                   /* tm_yday */ 0,
                   /* tm_isdst */ 0,
                   /* tm_gmtoff */ 0,
                   "UTC"};

}  // namespace

TimePoint PostgresEpoch() {
  return std::chrono::system_clock::from_time_t(std::mktime(&kPgEpochTm));
}

}  // namespace postgres
}  // namespace storages
