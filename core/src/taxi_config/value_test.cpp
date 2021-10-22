#include <userver/taxi_config/value.hpp>

#include <gtest/gtest.h>
#include <boost/range/adaptor/map.hpp>

USERVER_NAMESPACE_BEGIN

TEST(ValueDict, UseAsRange) {
  using ValueDict = taxi_config::ValueDict<int>;

  ValueDict m_dict{
      "name",
      {{"one", 1}, {"two", 2}, {"three", 3}},
  };
  const ValueDict& c_dict = m_dict;

  EXPECT_TRUE((std::is_same_v<decltype(m_dict.begin()), ValueDict::iterator>));
  EXPECT_TRUE(
      (std::is_same_v<decltype(c_dict.begin()), ValueDict::const_iterator>));

  // ValueDict has no mutable iterator: iterator == const_iterator.
  EXPECT_TRUE(
      (std::is_same_v<decltype(m_dict.begin()), decltype(c_dict.begin())>));

  for (const auto& each : (m_dict | boost::adaptors::map_keys)) {
    EXPECT_TRUE(m_dict.HasValue(each));
  }

  for (const auto& each : (c_dict | boost::adaptors::map_keys)) {
    EXPECT_TRUE(c_dict.HasValue(each));
  }
}

USERVER_NAMESPACE_END
