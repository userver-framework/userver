#pragma once

#include <userver/chaotic/convert/to.hpp>

namespace my {

struct CustomNumber final {
    double s;
};

inline bool operator==(const CustomNumber& lhs, const CustomNumber& rhs) { return lhs.s == rhs.s; }

inline CustomNumber Convert(const double& s, USERVER_NAMESPACE::chaotic::convert::To<CustomNumber>) { return {s}; }

inline double Convert(const CustomNumber& num, USERVER_NAMESPACE::chaotic::convert::To<double>) { return num.s; }

}  // namespace my
