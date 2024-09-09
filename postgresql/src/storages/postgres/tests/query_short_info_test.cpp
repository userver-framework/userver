#include <gtest/gtest.h>

USERVER_NAMESPACE_BEGIN

// Can not include connection_impl.hpp here, so we have to redeclare the
// FindQueryShortInfo function.
namespace storages::postgres::detail {
std::string FindQueryShortInfo(std::string_view, std::string_view);
}

inline std::string FindQueryShortInfo(std::string_view sw) {
  return storages::postgres::detail::FindQueryShortInfo("pg_query", sw);
}

TEST(PostgreFindQueryShortInfo, Basic) {
  EXPECT_EQ(FindQueryShortInfo("SELECT foo, bar FROM table"),
            "pg_query:select");
  EXPECT_EQ(FindQueryShortInfo("select foo, bar FROM table"),
            "pg_query:select");
  EXPECT_EQ(FindQueryShortInfo("sElEct foo, bar FROM table"),
            "pg_query:select");

  EXPECT_EQ(FindQueryShortInfo("UPDATE MY_TABLE SET A = 5"), "pg_query:update");
  EXPECT_EQ(FindQueryShortInfo("UPDATE my_table SET A = 5"), "pg_query:update");
  EXPECT_EQ(FindQueryShortInfo("update table set x = 7"), "pg_query:update");
  EXPECT_EQ(FindQueryShortInfo("upDate table set x = 7"), "pg_query:update");

  EXPECT_EQ(FindQueryShortInfo("INSERT INTO foo(bar) VALUES (42)"),
            "pg_query:insert");
  EXPECT_EQ(FindQueryShortInfo("insert INTO foo(bar) VALUES (42)"),
            "pg_query:insert");

  EXPECT_EQ(FindQueryShortInfo("begin"), "pg_query:begin");
  EXPECT_EQ(FindQueryShortInfo("commit"), "pg_query:commit");
  EXPECT_EQ(FindQueryShortInfo("rollback"), "pg_query:rollback");

  EXPECT_EQ(FindQueryShortInfo(R"~~(
  CREATE FUNCTION dept(text) RETURNS dept
    AS $$ SELECT * FROM dept WHERE name = $1 $$
    LANGUAGE SQL;)~~"),
            "pg_query:create");

  EXPECT_EQ(FindQueryShortInfo(R"~~(
  CREATE TABLE my_first_table (
    first_column text,
    second_column integer
  ))~~"),
            "pg_query:create");
}

TEST(PostgreFindQueryShortInfo, With) {
  EXPECT_EQ(FindQueryShortInfo(R"~~(
  WITH regional_sales AS (
    SELECT region, SUM(amount) AS total_sales
    FROM orders
    GROUP BY region
), top_regions AS (
    SELECT region
    FROM regional_sales
    WHERE total_sales > (SELECT SUM(total_sales)/10 FROM regional_sales)
))~~"),
            "pg_query:with");

  EXPECT_EQ(FindQueryShortInfo(R"~~(
  WITH RECURSIVE t(n) AS (
    VALUES (1)
  UNION ALL
    SELECT n+1 FROM t WHERE n < 100
)
SELECT sum(n) FROM t)~~"),
            "pg_query:with");

  EXPECT_EQ(FindQueryShortInfo(R"~~(
  WITH RECURSIVE included_parts(sub_part, part, quantity) AS (
    SELECT sub_part, part, quantity FROM parts WHERE part = 'our_product'
  UNION ALL
    SELECT p.sub_part, p.part, p.quantity * pr.quantity
    FROM included_parts pr, parts p
    WHERE p.part = pr.sub_part
)
SELECT sub_part, SUM(quantity) as total_quantity
FROM included_parts
GROUP BY sub_part)~~"),
            "pg_query:with");
}

TEST(PostgreFindQueryShortInfo, Comments) {
  EXPECT_EQ(FindQueryShortInfo(R"~~(
  -- this does bla-vla-vla in v1
  SELECT foo, bar FROM table)~~"),
            "pg_query");

  EXPECT_EQ(FindQueryShortInfo(R"~~(
  /* this does bla-vla-vla in v1 */
  SELECT foo, bar FROM table)~~"),
            "pg_query");

  EXPECT_EQ(FindQueryShortInfo(R"~~(
  /* this does bla-vla-vla in v1 */

  -- v1


  /* params
   *  - nope
   */
  SELECT foo, bar FROM table)~~"),
            "pg_query");
}

TEST(PostgreFindQueryShortInfo, Empty) {
  EXPECT_EQ(FindQueryShortInfo(""), "pg_query");
  EXPECT_EQ(FindQueryShortInfo("\n\n"), "pg_query");
  EXPECT_EQ(FindQueryShortInfo("\n\r\n\r"), "pg_query");
  EXPECT_EQ(FindQueryShortInfo("\t"), "pg_query");
}

USERVER_NAMESPACE_END
