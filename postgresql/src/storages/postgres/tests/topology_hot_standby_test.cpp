#include <userver/utest/utest.hpp>

#include <string>
#include <vector>

#include <storages/postgres/detail/topology/hot_standby.hpp>

USERVER_NAMESPACE_BEGIN

TEST(PostgreTopology, ParseSyncSlaveNames) {
  namespace topology = storages::postgres::detail::topology;
  using Names = std::vector<std::string>;

  EXPECT_EQ(Names{}, topology::ParseSyncStandbyNames(""));
  EXPECT_EQ(Names{"single"}, topology::ParseSyncStandbyNames("single"));
  EXPECT_EQ(Names{"one"}, topology::ParseSyncStandbyNames("one, two"));

  EXPECT_EQ(Names{}, topology::ParseSyncStandbyNames("ANY 1 (one,two )"));
  EXPECT_EQ(Names{}, topology::ParseSyncStandbyNames("any 2 (one, two)"));
  EXPECT_EQ(Names{}, topology::ParseSyncStandbyNames("AnY 1 (one,two)"));

  EXPECT_EQ(Names{"one"}, topology::ParseSyncStandbyNames("1 (one)"));
  EXPECT_EQ(Names{"one"}, topology::ParseSyncStandbyNames("FIRST 1 (one)"));
  EXPECT_EQ(Names{"one"}, topology::ParseSyncStandbyNames("FirST 1 (one)"));
  EXPECT_EQ(Names{"one"}, topology::ParseSyncStandbyNames("1 (one, two)"));
  EXPECT_EQ(Names{"one"},
            topology::ParseSyncStandbyNames(R"(first 1 ("one", two))"));
  EXPECT_EQ((Names{"one", "two"}),
            topology::ParseSyncStandbyNames("2 (one, two)"));
  EXPECT_EQ((Names{"one", "two"}),
            topology::ParseSyncStandbyNames(R"(FIrST 2 ("one", "two"))"));
  EXPECT_EQ((Names{"one", "two"}),
            topology::ParseSyncStandbyNames("2 (one, two, three)"));
  EXPECT_EQ((Names{"one", "two"}),
            topology::ParseSyncStandbyNames("FIRST 2 (one, two, three)"));

  EXPECT_EQ((Names{"any", "first"}),
            topology::ParseSyncStandbyNames(R"( 2 ("any", "first") )"));
}

USERVER_NAMESPACE_END
