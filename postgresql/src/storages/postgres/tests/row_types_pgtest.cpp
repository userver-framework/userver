#include <userver/utest/utest.hpp>

#include <userver/storages/postgres/io/bytea.hpp>
#include <userver/storages/postgres/io/row_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

namespace pg = storages::postgres;

struct SimpleStructure {
  std::vector<unsigned char> bytes;
};

// Implementation of ByteaWrapper is the same as SimpeStructure but
// it is marked as wrapper so result of kIsRowType is different
using TestByteaWrapper = pg::ByteaWrapper<std::vector<unsigned char>>;

UTEST(RowTypes, SimpeStructure) {
  EXPECT_TRUE(pg::io::traits::kIsRowType<SimpleStructure>);
}

UTEST(RowTypes, TestByteaWrapper) {
  EXPECT_FALSE(pg::io::traits::kIsRowType<TestByteaWrapper>);
}

}  // namespace

USERVER_NAMESPACE_END
