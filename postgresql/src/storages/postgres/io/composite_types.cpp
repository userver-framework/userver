#include <storages/postgres/io/composite_types.hpp>

#include <storages/postgres/io/traits.hpp>

namespace storages::postgres::io {

namespace {

struct Record {
  // must be non-empty to be a composite type
  int dummy_;
};

}  // namespace

template <>
struct CppToSystemPg<Record> : PredefinedOid<PredefinedOids::kRecord> {};

static_assert(traits::kTypeBufferCategory<Record> ==
              BufferCategory::kCompositeBuffer);
static_assert(traits::kHasParser<Record>);

namespace detail {

// here to force linkage
void InitRecordParser() { ForceReference(CppToPg<Record>::init_); }

}  // namespace detail
}  // namespace storages::postgres::io
