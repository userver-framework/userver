#include <span>

#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/span.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;
namespace tt = io::traits;

namespace static_test {

using dynamic_span = std::span<const int>;
using fixed_span = std::span<const int, 3>;

static_assert(tt::kIsCompatibleContainer<dynamic_span>);
static_assert(tt::kIsCompatibleContainer<fixed_span>);

static_assert(tt::kDimensionCount<dynamic_span> == 1);

static_assert(std::is_same_v<tt::ContainerFinalElement<dynamic_span>::type, int>);

static_assert(tt::kIsMappedToPg<dynamic_span>);
static_assert(tt::kIsMappedToPg<fixed_span>);

static_assert(tt::HasFormatter<dynamic_span>);
static_assert(tt::HasFormatter<fixed_span>);

static_assert(!tt::HasParser<dynamic_span>);
static_assert(!tt::HasParser<fixed_span>);

static_assert(tt::kTypeBufferCategory<dynamic_span> == io::BufferCategory::kNoParser);
static_assert(tt::kTypeBufferCategory<fixed_span> == io::BufferCategory::kNoParser);

// A span serializes to the same array OID as the owning vector.
const pg::UserTypes types;
static_assert(
    io::CppToPg<dynamic_span>::GetOid(types) ==
    static_cast<unsigned int>(io::PredefinedOid<io::PredefinedOids::kInt4Array>::value)
);

}  // namespace static_test

namespace {

UTEST_P(PostgreConnection, SpanRoundtrip) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};
    {
        const std::vector<int> src{-3, -2, 0, 1, 2, 3};
        const std::span<const int> span{src};
        UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as int_array", span));
        std::vector<int> tgt;
        UEXPECT_NO_THROW(res[0][0].To(tgt));
        EXPECT_EQ(src, tgt);
    }
    {
        const std::vector<int> src{10, 20, 30, 40, 50};
        const std::span<const int> span{src};
        UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as int_array", span.subspan(1, 2)));
        std::vector<int> tgt;
        UEXPECT_NO_THROW(res[0][0].To(tgt));
        EXPECT_EQ((std::vector<int>{20, 30}), tgt);
    }
    {
        const std::array<int, 3> src{7, 8, 9};
        const std::span<const int, 3> span{src};
        UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as int_array", span));
        std::vector<int> tgt;
        UEXPECT_NO_THROW(res[0][0].To(tgt));
        EXPECT_EQ((std::vector<int>{7, 8, 9}), tgt);
    }
}

UTEST_P(PostgreConnection, SpanEmpty) {
    CheckConnection(GetConn());
    pg::ResultSet res{nullptr};

    const std::vector<int> empty;
    const std::span<const int> span{empty};
    UEXPECT_NO_THROW(res = GetConn()->Execute("select $1 as int_array", span));
    std::vector<int> tgt{1, 2, 3};
    UEXPECT_NO_THROW(res[0][0].To(tgt));
    EXPECT_TRUE(tgt.empty());
}

}  // namespace

USERVER_NAMESPACE_END
