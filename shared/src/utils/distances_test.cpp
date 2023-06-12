#include <utils/distances.hpp>

#include <iterator>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gtest/gtest-param-test.h>
#include <gtest/gtest.h>
#include <boost/range/adaptors.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

struct TestData {
  std::string view1;
  std::string view2;
  std::size_t result;
};

class LevenshteinDistanceTest : public ::testing::TestWithParam<TestData> {};

class DamerauLevenshteinDistanceTest
    : public ::testing::TestWithParam<TestData> {};

struct NearestNameTestData {
  const std::vector<std::string>& objects;
  std::string key;
  std::size_t max_distance;
  std::string result;
};

class NearestNameFixture
    : public ::testing::TestWithParam<NearestNameTestData> {};

const std::vector<std::string> languages = {"cpp", "python", "ruby", "c_sharp",
                                            "java"};

}  // namespace

TEST_P(LevenshteinDistanceTest, LevenshteinAlgorithm) {
  const TestData data = GetParam();
  EXPECT_EQ(utils::GetLevenshteinDistance(data.view1, data.view2), data.result);
}
INSTANTIATE_TEST_SUITE_P(LevenshteinTests, LevenshteinDistanceTest,
                         ::testing::Values(TestData{"hello", "hello", 0},
                                           TestData{"hellw", "hello", 1},
                                           TestData{"hell", "hello", 1},
                                           TestData{"hellow", "hello", 1},
                                           TestData{"bad", "good", 3},
                                           TestData{"apple", "happend", 4},
                                           TestData{"a", "append", 5},
                                           TestData{"malloc", "mlaloc", 2},
                                           TestData{"baccba", "abccab", 4}));

TEST_P(DamerauLevenshteinDistanceTest, DamerauLevenshteinAlgorithm) {
  const TestData data = GetParam();
  EXPECT_EQ(utils::GetDamerauLevenshteinDistance(data.view1, data.view2),
            data.result);
}

INSTANTIATE_TEST_SUITE_P(DamerauLevenshteinTests,
                         DamerauLevenshteinDistanceTest,
                         ::testing::Values(TestData{"hello", "hello", 0},
                                           TestData{"hellw", "hello", 1},
                                           TestData{"hell", "hello", 1},
                                           TestData{"bad", "good", 3},
                                           TestData{"apple", "happend", 4},
                                           TestData{"a", "append", 5},
                                           TestData{"malloc", "mlaloc", 1},
                                           TestData{"baccba", "abccab", 2}));

TEST_P(NearestNameFixture, NearestName) {
  NearestNameTestData data = GetParam();
  std::map<std::string, int> map_data;
  std::set<std::string> set_data;
  std::unordered_map<std::string, int> un_map_data;
  std::unordered_set<std::string> un_set_data;

  for (const auto& object : data.objects) {
    map_data[object] = 1;
    un_map_data[object] = 1;
    set_data.insert(object);
    un_set_data.insert(object);
  }

  EXPECT_EQ(utils::GetNearestString(set_data, data.key, data.max_distance)
                .value_or(""),
            data.result);
  EXPECT_EQ(utils::GetNearestString(map_data | boost::adaptors::map_keys,
                                    data.key, data.max_distance)
                .value_or(""),
            data.result);
  EXPECT_EQ(utils::GetNearestString(un_set_data, data.key, data.max_distance)
                .value_or(""),
            data.result);
  EXPECT_EQ(utils::GetNearestString(un_map_data | boost::adaptors::map_keys,
                                    data.key, data.max_distance)
                .value_or(""),
            data.result);
}

INSTANTIATE_TEST_SUITE_P(
    NearestNameTests, NearestNameFixture,
    ::testing::Values(NearestNameTestData{languages, "cpang", 3, "cpp"},
                      NearestNameTestData{languages, "cpang", 2, ""},
                      NearestNameTestData{languages, "c", 2, "cpp"},
                      NearestNameTestData{languages, "c", 1, ""},
                      NearestNameTestData{languages, "jemalloc", 6, "java"},
                      NearestNameTestData{languages, "hemalloc", 5, ""}));

USERVER_NAMESPACE_END
