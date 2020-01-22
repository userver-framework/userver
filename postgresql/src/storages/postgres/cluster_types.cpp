#include <storages/postgres/cluster_types.hpp>

#include <iostream>

#include <storages/postgres/dsn.hpp>

namespace storages {
namespace postgres {

namespace {

const char* ToStringRaw(const ClusterHostType& ht) {
#define CHT_TO_STR(kType)      \
  case ClusterHostType::kType: \
    return "ClusterHostType::" #kType

  switch (ht) {
    CHT_TO_STR(kUnknown);
    CHT_TO_STR(kMaster);
    CHT_TO_STR(kSyncSlave);
    CHT_TO_STR(kSlave);
    CHT_TO_STR(kAny);
  }

#undef CHT_TO_STR
}

}  // namespace

std::ostream& operator<<(std::ostream& os, const ClusterHostType& ht) {
  return os << ToStringRaw(ht);
}

std::string ToString(const ClusterHostType& ht) { return ToStringRaw(ht); }

}  // namespace postgres
}  // namespace storages
