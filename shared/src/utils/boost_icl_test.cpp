#include <boost/icl/continuous_interval.hpp>

#include <chrono>

#include <gtest/gtest.h>

using TimePoint = std::chrono::system_clock::time_point;
using Interval = boost::icl::continuous_interval<TimePoint>;

bool Intersects(const Interval& what, const Interval& with) {
  return boost::icl::intersects(what, with);
}

// Make sure that the sanitizers do not complain on old versions of Boost:
// `runtime error: left shift of negative value -3`
TEST(BoostIclTest, Intersects) {
  auto now = std::chrono::system_clock::now();
  Interval a{now - std::chrono::seconds{1}, now + std::chrono::seconds{1}};
  Interval b{now - std::chrono::seconds{1}, now};
  EXPECT_TRUE(Intersects(a, b));
}
