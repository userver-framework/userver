#pragma once

/// @file userver/storages/postgres/io/geometry_types.hpp
/// @brief Geometry I/O support

#include <array>

#include <userver/storages/postgres/exceptions.hpp>
#include <userver/storages/postgres/io/buffer_io_base.hpp>
#include <userver/storages/postgres/io/field_buffer.hpp>
#include <userver/storages/postgres/io/floating_point_types.hpp>
#include <userver/storages/postgres/io/traits.hpp>
#include <userver/storages/postgres/io/type_mapping.hpp>
#include <userver/storages/postgres/io/type_traits.hpp>
#include <userver/storages/postgres/io/user_types.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::postgres::io {

namespace detail {

//@{
/** @name On-the-wire representation of geometry types */
// These structures are mere helpers for parsing/formatting PostgreSQL buffers
// and are not intended to be used as types for geometry calculations
struct Point {
  constexpr bool operator==(const Point& rhs) const {
    return x == rhs.x && y == rhs.y;
  }
  constexpr bool operator!=(const Point& rhs) const { return !(*this == rhs); }

  double x;
  double y;
};

struct LineSegment {
  constexpr bool operator==(const LineSegment& rhs) const {
    return ends[0] == rhs.ends[0] && ends[1] == rhs.ends[1];
  }
  constexpr bool operator!=(const LineSegment& rhs) const {
    return !(*this == rhs);
  }

  std::array<Point, 2> ends{};
};

// Line is stored as coefficients to equation a*x + b*y + c = 0
struct Line {
  constexpr bool operator==(const Line& rhs) const {
    return a == rhs.a && b == rhs.b && c == rhs.c;
  }
  constexpr bool operator!=(const Line& rhs) const { return !(*this == rhs); }

  double a;
  double b;
  double c;
};

struct Box {
  constexpr bool operator==(const Box& rhs) const {
    return corners[0] == rhs.corners[0] && corners[1] == rhs.corners[1];
  }
  constexpr bool operator!=(const Box& rhs) const { return !(*this == rhs); }

  std::array<Point, 2> corners{};
};

struct Path {
  bool operator==(const Path& rhs) const {
    return is_closed == rhs.is_closed && points == rhs.points;
  }
  bool operator!=(const Path& rhs) const { return !(*this == rhs); }

  bool is_closed{false};
  std::vector<Point> points;
};

struct Polygon {
  bool operator==(const Polygon& rhs) const { return points == rhs.points; }
  bool operator!=(const Polygon& rhs) const { return !(*this == rhs); }

  std::vector<Point> points;
};

struct Circle {
  constexpr bool operator==(const Circle& rhs) const {
    return center == rhs.center && radius == rhs.radius;
  }
  constexpr bool operator!=(const Circle& rhs) const { return !(*this == rhs); }

  Point center;
  double radius;
};
//@}

struct PointParser : BufferParserBase<Point> {
  using BaseType = BufferParserBase<Point>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    buffer.Read(value.x);
    buffer.Read(value.y);
  }
};

struct PointFormatter : BufferFormatterBase<Point> {
  using BaseType = BufferFormatterBase<Point>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    io::WriteBuffer(types, buffer, value.x);
    io::WriteBuffer(types, buffer, value.y);
  }
};

struct LineSegmentParser : BufferParserBase<LineSegment> {
  using BaseType = BufferParserBase<LineSegment>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    buffer.Read(value.ends[0]);
    buffer.Read(value.ends[1]);
  }
};

struct LineSegmentFormatter : BufferFormatterBase<LineSegment> {
  using BaseType = BufferFormatterBase<LineSegment>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    io::WriteBuffer(types, buffer, value.ends[0]);
    io::WriteBuffer(types, buffer, value.ends[1]);
  }
};

struct LineParser : BufferParserBase<Line> {
  using BaseType = BufferParserBase<Line>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    buffer.Read(value.a);
    buffer.Read(value.b);
    buffer.Read(value.c);
  }
};

struct LineFormatter : BufferFormatterBase<Line> {
  using BaseType = BufferFormatterBase<Line>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    io::WriteBuffer(types, buffer, value.a);
    io::WriteBuffer(types, buffer, value.b);
    io::WriteBuffer(types, buffer, value.c);
  }
};

struct BoxParser : BufferParserBase<Box> {
  using BaseType = BufferParserBase<Box>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    buffer.Read(value.corners[0]);
    buffer.Read(value.corners[1]);
  }
};

struct BoxFormatter : BufferFormatterBase<Box> {
  using BaseType = BufferFormatterBase<Box>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    io::WriteBuffer(types, buffer, value.corners[0]);
    io::WriteBuffer(types, buffer, value.corners[1]);
  }
};

struct PathParser : BufferParserBase<Path> {
  using BaseType = BufferParserBase<Path>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    buffer.Read(value.is_closed);
    Integer point_no{0};
    buffer.Read(point_no);
    value.points.resize(point_no);
    for (auto i = 0; i < point_no; ++i) {
      buffer.Read(value.points[i]);
    }
  }
};

struct PathFormatter : BufferFormatterBase<Path> {
  using BaseType = BufferFormatterBase<Path>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    io::WriteBuffer(types, buffer, value.is_closed);
    Integer points_no = value.points.size();
    io::WriteBuffer(types, buffer, points_no);
    for (const auto& p : value.points) {
      io::WriteBuffer(types, buffer, p);
    }
  }
};

struct PolygonParser : BufferParserBase<Polygon> {
  using BaseType = BufferParserBase<Polygon>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    Integer point_no{0};
    buffer.Read(point_no);
    value.points.resize(point_no);
    for (auto i = 0; i < point_no; ++i) {
      buffer.Read(value.points[i]);
    }
  }
};

struct PolygonFormatter : BufferFormatterBase<Polygon> {
  using BaseType = BufferFormatterBase<Polygon>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    Integer points_no = value.points.size();
    io::WriteBuffer(types, buffer, points_no);
    for (const auto& p : value.points) {
      io::WriteBuffer(types, buffer, p);
    }
  }
};

struct CircleParser : BufferParserBase<Circle> {
  using BaseType = BufferParserBase<Circle>;
  using BaseType::BaseType;

  void operator()(FieldBuffer buffer) {
    buffer.Read(value.center);
    buffer.Read(value.radius);
  }
};

struct CircleFormatter : BufferFormatterBase<Circle> {
  using BaseType = BufferFormatterBase<Circle>;
  using BaseType::BaseType;

  template <typename Buffer>
  void operator()(const UserTypes& types, Buffer& buffer) const {
    io::WriteBuffer(types, buffer, value.center);
    io::WriteBuffer(types, buffer, value.radius);
  }
};

}  // namespace detail

namespace traits {

template <>
struct Input<io::detail::Point> {
  using type = io::detail::PointParser;
};

template <>
struct ParserBufferCategory<io::detail::PointParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<io::detail::Point> {
  using type = io::detail::PointFormatter;
};

template <>
struct Input<io::detail::LineSegment> {
  using type = io::detail::LineSegmentParser;
};

template <>
struct ParserBufferCategory<io::detail::LineSegmentParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<io::detail::LineSegment> {
  using type = io::detail::LineSegmentFormatter;
};

template <>
struct Input<io::detail::Line> {
  using type = io::detail::LineParser;
};

template <>
struct ParserBufferCategory<io::detail::LineParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<io::detail::Line> {
  using type = io::detail::LineFormatter;
};

template <>
struct Input<io::detail::Box> {
  using type = io::detail::BoxParser;
};

template <>
struct ParserBufferCategory<io::detail::BoxParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<io::detail::Box> {
  using type = io::detail::BoxFormatter;
};

template <>
struct Input<io::detail::Path> {
  using type = io::detail::PathParser;
};

template <>
struct ParserBufferCategory<io::detail::PathParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<io::detail::Path> {
  using type = io::detail::PathFormatter;
};

template <>
struct Input<io::detail::Polygon> {
  using type = io::detail::PolygonParser;
};

template <>
struct ParserBufferCategory<io::detail::PolygonParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<io::detail::Polygon> {
  using type = io::detail::PolygonFormatter;
};

template <>
struct Input<io::detail::Circle> {
  using type = io::detail::CircleParser;
};

template <>
struct ParserBufferCategory<io::detail::CircleParser>
    : std::integral_constant<BufferCategory, BufferCategory::kPlainBuffer> {};

template <>
struct Output<io::detail::Circle> {
  using type = io::detail::CircleFormatter;
};

}  // namespace traits

template <>
struct CppToSystemPg<detail::Point> : PredefinedOid<PredefinedOids::kPoint> {};
template <>
struct CppToSystemPg<detail::LineSegment>
    : PredefinedOid<PredefinedOids::kLseg> {};
template <>
struct CppToSystemPg<detail::Line> : PredefinedOid<PredefinedOids::kLine> {};
template <>
struct CppToSystemPg<detail::Box> : PredefinedOid<PredefinedOids::kBox> {};
template <>
struct CppToSystemPg<detail::Path> : PredefinedOid<PredefinedOids::kPath> {};
template <>
struct CppToSystemPg<detail::Polygon>
    : PredefinedOid<PredefinedOids::kPolygon> {};
template <>
struct CppToSystemPg<detail::Circle> : PredefinedOid<PredefinedOids::kCircle> {
};

}  // namespace storages::postgres::io

USERVER_NAMESPACE_END
