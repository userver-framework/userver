#include <storages/postgres/tests/util_pgtest.hpp>
#include <userver/storages/postgres/io/geometry_types.hpp>
#include <userver/storages/postgres/parameter_store.hpp>

USERVER_NAMESPACE_BEGIN

namespace pg = storages::postgres;
namespace io = pg::io;

namespace {

UTEST_P(PostgreConnection, InternalGeometryPointRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  io::detail::Point p{};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1", io::detail::Point{1, 2}));
  UEXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Point{1, 2}), p);
}
//
UTEST_P(PostgreConnection, InternalGeometryLineRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  io::detail::Line l{};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1", io::detail::Line{1, 2, 3}));
  UEXPECT_NO_THROW(res[0][0].To(l));
  EXPECT_EQ((io::detail::Line{1, 2, 3}), l);
}

UTEST_P(PostgreConnection, InternalGeometryLsegRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  io::detail::LineSegment l;
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1",
                               io::detail::LineSegment{{{{-1, 1}, {1, -1}}}}));
  UEXPECT_NO_THROW(res[0][0].To(l));
  EXPECT_EQ((io::detail::LineSegment{{{{-1, 1}, {1, -1}}}}), l);
}

UTEST_P(PostgreConnection, InternalGeometryBoxRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  io::detail::Box b;
  UEXPECT_NO_THROW(res = GetConn()->Execute(
                       "select $1", io::detail::Box{{{{1, 1}, {0, 0}}}}));
  UEXPECT_NO_THROW(res[0][0].To(b));
  EXPECT_EQ((io::detail::Box{{{{1, 1}, {0, 0}}}}), b);
}

UTEST_P(PostgreConnection, InternalGeometryPathRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  io::detail::Path p;
  UEXPECT_NO_THROW(
      res = GetConn()->Execute(
          "select $1", io::detail::Path{true, {{1, 1}, {0, 0}, {-1, 1}}}));
  UEXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Path{true, {{1, 1}, {0, 0}, {-1, 1}}}), p);
  UEXPECT_NO_THROW(
      res = GetConn()->Execute(
          "select $1", io::detail::Path{false, {{1, 1}, {0, 0}, {-1, 1}}}));
  UEXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Path{false, {{1, 1}, {0, 0}, {-1, 1}}}), p);
}

UTEST_P(PostgreConnection, InternalGeometryPolygonRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  io::detail::Polygon p;
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1",
                               io::detail::Polygon{{{1, 1}, {0, 0}, {-1, 1}}}));
  UEXPECT_NO_THROW(res[0][0].To(p));
  EXPECT_EQ((io::detail::Polygon{{{1, 1}, {0, 0}, {-1, 1}}}), p);
}

UTEST_P(PostgreConnection, InternalGeometryCircleRoundtrip) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  io::detail::Circle c{};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute("select $1", io::detail::Circle{{1, 2}, 3}));
  UEXPECT_NO_THROW(res[0][0].To(c));
  EXPECT_EQ((io::detail::Circle{{1, 2}, 3}), c);
}

UTEST_P(PostgreConnection, InternalGeometryStored) {
  CheckConnection(GetConn());

  pg::ResultSet res{nullptr};
  UEXPECT_NO_THROW(
      res = GetConn()->Execute(
          "select $1, $2, $3, $4, $5, $6, $7",
          pg::ParameterStore{}
              .PushBack(io::detail::Point{1, 2})
              .PushBack(io::detail::Line{1, 2, 3})
              .PushBack(io::detail::LineSegment{{{{-1, 1}, {1, -1}}}})
              .PushBack(io::detail::Box{{{{1, 1}, {0, 0}}}})
              .PushBack(io::detail::Path{false, {{1, 1}, {0, 0}, {-1, 1}}})
              .PushBack(io::detail::Polygon{{{1, 1}, {0, 0}, {-1, 1}}})
              .PushBack(io::detail::Circle{{1, 2}, 3})));
  EXPECT_EQ((io::detail::Point{1, 2}), res[0][0].As<io::detail::Point>());
  EXPECT_EQ((io::detail::Line{1, 2, 3}), res[0][1].As<io::detail::Line>());
  EXPECT_EQ((io::detail::LineSegment{{{{-1, 1}, {1, -1}}}}),
            res[0][2].As<io::detail::LineSegment>());
  EXPECT_EQ((io::detail::Box{{{{1, 1}, {0, 0}}}}),
            res[0][3].As<io::detail::Box>());
  EXPECT_EQ((io::detail::Path{false, {{1, 1}, {0, 0}, {-1, 1}}}),
            res[0][4].As<io::detail::Path>());
  EXPECT_EQ((io::detail::Polygon{{{1, 1}, {0, 0}, {-1, 1}}}),
            res[0][5].As<io::detail::Polygon>());
  EXPECT_EQ((io::detail::Circle{{1, 2}, 3}),
            res[0][6].As<io::detail::Circle>());
}

}  // namespace

USERVER_NAMESPACE_END
