#include <utest/utest.hpp>

#include <string>
#include <vector>

#include <storages/postgres/detail/quorum_commit.hpp>

TEST(Topology, ParseSyncSlaveNames) {
  namespace detail = storages::postgres::detail;
  using Names = std::vector<std::string>;

  EXPECT_EQ(Names{}, detail::ParseSyncStandbyNames(""));
  EXPECT_EQ(Names{"single"}, detail::ParseSyncStandbyNames("single"));
  EXPECT_EQ(Names{"one"}, detail::ParseSyncStandbyNames("one, two"));

  EXPECT_EQ(Names{}, detail::ParseSyncStandbyNames("ANY 1 (one,two )"));
  EXPECT_EQ(Names{}, detail::ParseSyncStandbyNames("any 2 (one, two)"));
  EXPECT_EQ(Names{}, detail::ParseSyncStandbyNames("AnY 1 (one,two)"));

  EXPECT_EQ(Names{"one"}, detail::ParseSyncStandbyNames("1 (one)"));
  EXPECT_EQ(Names{"one"}, detail::ParseSyncStandbyNames("FIRST 1 (one)"));
  EXPECT_EQ(Names{"one"}, detail::ParseSyncStandbyNames("FirST 1 (one)"));
  EXPECT_EQ(Names{"one"}, detail::ParseSyncStandbyNames("1 (one, two)"));
  EXPECT_EQ(Names{"one"},
            detail::ParseSyncStandbyNames(R"(first 1 ("one", two))"));
  EXPECT_EQ((Names{"one", "two"}),
            detail::ParseSyncStandbyNames("2 (one, two)"));
  EXPECT_EQ((Names{"one", "two"}),
            detail::ParseSyncStandbyNames(R"(FIrST 2 ("one", "two"))"));
  EXPECT_EQ((Names{"one", "two"}),
            detail::ParseSyncStandbyNames("2 (one, two, three)"));
  EXPECT_EQ((Names{"one", "two"}),
            detail::ParseSyncStandbyNames("FIRST 2 (one, two, three)"));

  EXPECT_EQ((Names{"any", "first"}),
            detail::ParseSyncStandbyNames(R"( 2 ("any", "first") )"));
}
