#pragma once

#include <storages/postgres/exceptions.hpp>
#include <storages/postgres/io/buffer_io_base.hpp>
#include <storages/postgres/io/field_buffer.hpp>
#include <storages/postgres/io/traits.hpp>
#include <storages/postgres/io/type_mapping.hpp>
#include <storages/postgres/io/type_traits.hpp>
#include <storages/postgres/io/user_types.hpp>

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
    return ends == rhs.ends;
  }
  constexpr bool operator!=(const LineSegment& rhs) const {
    return !(*this == rhs);
  }

  std::array<Point, 2> ends;
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
    return corners == rhs.corners;
  }
  constexpr bool operator!=(const Box& rhs) const { return !(*this == rhs); }

  std::array<Point, 2> corners;
};

struct Path {
  constexpr bool operator==(const Path& rhs) const {
    return is_closed == rhs.is_closed && points == rhs.points;
  }
  constexpr bool operator!=(const Path& rhs) const { return !(*this == rhs); }

  bool is_closed;
  std::vector<Point> points;
};

struct Polygon {
  constexpr bool operator==(const Polygon& rhs) const {
    return points == rhs.points;
  }
  constexpr bool operator!=(const Polygon& rhs) const {
    return !(*this == rhs);
  }

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
    WriteBinary(types, buffer, value.x);
    WriteBinary(types, buffer, value.y);
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
    WriteBinary(types, buffer, value.ends[0]);
    WriteBinary(types, buffer, value.ends[1]);
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
    WriteBinary(types, buffer, value.a);
    WriteBinary(types, buffer, value.b);
    WriteBinary(types, buffer, value.c);
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
    WriteBinary(types, buffer, value.corners[0]);
    WriteBinary(types, buffer, value.corners[1]);
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
    WriteBinary(types, buffer, value.is_closed);
    Integer points_no = value.points.size();
    WriteBinary(types, buffer, points_no);
    for (auto const& p : value.points) {
      WriteBinary(types, buffer, p);
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
    WriteBinary(types, buffer, points_no);
    for (auto const& p : value.points) {
      WriteBinary(types, buffer, p);
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
    WriteBinary(types, buffer, value.center);
    WriteBinary(types, buffer, value.radius);
  }
};

}  // namespace detail

namespace traits {

template <>
struct Input<io::detail::Point, DataFormat::kBinaryDataFormat> {
  using type = io::detail::PointParser;
};

template <>
struct Output<io::detail::Point, DataFormat::kBinaryDataFormat> {
  using type = io::detail::PointFormatter;
};

template <>
struct Input<io::detail::LineSegment, DataFormat::kBinaryDataFormat> {
  using type = io::detail::LineSegmentParser;
};

template <>
struct Output<io::detail::LineSegment, DataFormat::kBinaryDataFormat> {
  using type = io::detail::LineSegmentFormatter;
};

template <>
struct Input<io::detail::Line, DataFormat::kBinaryDataFormat> {
  using type = io::detail::LineParser;
};

template <>
struct Output<io::detail::Line, DataFormat::kBinaryDataFormat> {
  using type = io::detail::LineFormatter;
};

template <>
struct Input<io::detail::Box, DataFormat::kBinaryDataFormat> {
  using type = io::detail::BoxParser;
};

template <>
struct Output<io::detail::Box, DataFormat::kBinaryDataFormat> {
  using type = io::detail::BoxFormatter;
};

template <>
struct Input<io::detail::Path, DataFormat::kBinaryDataFormat> {
  using type = io::detail::PathParser;
};

template <>
struct Output<io::detail::Path, DataFormat::kBinaryDataFormat> {
  using type = io::detail::PathFormatter;
};

template <>
struct Input<io::detail::Polygon, DataFormat::kBinaryDataFormat> {
  using type = io::detail::PolygonParser;
};

template <>
struct Output<io::detail::Polygon, DataFormat::kBinaryDataFormat> {
  using type = io::detail::PolygonFormatter;
};

template <>
struct Input<io::detail::Circle, DataFormat::kBinaryDataFormat> {
  using type = io::detail::CircleParser;
};

template <>
struct Output<io::detail::Circle, DataFormat::kBinaryDataFormat> {
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
