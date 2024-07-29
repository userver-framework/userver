#pragma once

#include <vector>

#include <userver/chaotic/convert/to.hpp>

namespace my {

struct Point {
  double lon;
  double lat;
};

bool operator==(const Point& a, const Point& b) {
  return a.lon == b.lon && a.lat == b.lat;
}

inline Point Convert(const std::vector<double>& arr,
                     USERVER_NAMESPACE::chaotic::convert::To<Point>) {
  return Point{arr.at(0), arr.at(1)};
}

inline std::vector<double> Convert(
    const Point& p,
    USERVER_NAMESPACE::chaotic::convert::To<std::vector<double>>) {
  return std::vector{p.lon, p.lat};
}

template <typename T>
Point Convert(const T& obj, USERVER_NAMESPACE::chaotic::convert::To<Point>) {
  return Point{obj.lon, obj.lat};
}

template <typename T>
T Convert(const Point& p, USERVER_NAMESPACE::chaotic::convert::To<T>) {
  T result;
  // designated initializers are not available in C++17 :(
  result.lon = p.lon;
  result.lat = p.lat;
  return result;
}

}  // namespace my
