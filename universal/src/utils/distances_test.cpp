#include <userver/utils/distances.hpp>

#include <iterator>
#include <ranges>
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

USERVER_NAMESPACE_BEGIN

namespace {

struct TestData {
    std::string view1;
    std::string view2;
    std::size_t result;
};

class LevenshteinDistanceTest : public ::testing::TestWithParam<TestData> {};

class DamerauLevenshteinDistanceTest : public ::testing::TestWithParam<TestData> {};

struct NearestNameTestData {
    const std::vector<std::string>& objects;
    std::string key;
    std::size_t max_distance;
    std::string result;
};

class NearestNameFixture : public ::testing::TestWithParam<NearestNameTestData> {};

const std::vector<std::string> languages = {"cpp", "python", "ruby", "c_sharp", "java"};

}  // namespace

TEST_P(LevenshteinDistanceTest, LevenshteinAlgorithm) {
    const TestData& data = GetParam();
    EXPECT_EQ(utils::GetLevenshteinDistance(data.view1, data.view2), data.result);
}
INSTANTIATE_TEST_SUITE_P(
    LevenshteinTests,
    LevenshteinDistanceTest,
    ::testing::Values(
        TestData{.view1 = "hello", .view2 = "hello", .result = 0},
        TestData{.view1 = "hellw", .view2 = "hello", .result = 1},
        TestData{.view1 = "hell", .view2 = "hello", .result = 1},
        TestData{.view1 = "hellow", .view2 = "hello", .result = 1},
        TestData{.view1 = "bad", .view2 = "good", .result = 3},
        TestData{.view1 = "apple", .view2 = "happend", .result = 4},
        TestData{.view1 = "a", .view2 = "append", .result = 5},
        TestData{.view1 = "malloc", .view2 = "mlaloc", .result = 2},
        TestData{.view1 = "baccba", .view2 = "abccab", .result = 4}
    )
);

TEST_P(DamerauLevenshteinDistanceTest, DamerauLevenshteinAlgorithm) {
    const TestData& data = GetParam();
    EXPECT_EQ(utils::GetDamerauLevenshteinDistance(data.view1, data.view2), data.result);
}

INSTANTIATE_TEST_SUITE_P(
    DamerauLevenshteinTests,
    DamerauLevenshteinDistanceTest,
    ::testing::Values(
        TestData{.view1 = "hello", .view2 = "hello", .result = 0},
        TestData{.view1 = "hellw", .view2 = "hello", .result = 1},
        TestData{.view1 = "hell", .view2 = "hello", .result = 1},
        TestData{.view1 = "bad", .view2 = "good", .result = 3},
        TestData{.view1 = "apple", .view2 = "happend", .result = 4},
        TestData{.view1 = "a", .view2 = "append", .result = 5},
        TestData{.view1 = "malloc", .view2 = "mlaloc", .result = 1},
        TestData{.view1 = "baccba", .view2 = "abccab", .result = 2}
    )
);

TEST_P(NearestNameFixture, NearestName) {
    const NearestNameTestData& data = GetParam();
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

    EXPECT_EQ(utils::GetNearestString(set_data, data.key, data.max_distance).value_or(""), data.result);
    EXPECT_EQ(
        utils::GetNearestString(map_data | std::views::keys, data.key, data.max_distance).value_or(""),
        data.result
    );
    EXPECT_EQ(utils::GetNearestString(un_set_data, data.key, data.max_distance).value_or(""), data.result);
    EXPECT_EQ(
        utils::GetNearestString(un_map_data | std::views::keys, data.key, data.max_distance).value_or(""),
        data.result
    );
}

INSTANTIATE_TEST_SUITE_P(
    NearestNameTests,
    NearestNameFixture,
    ::testing::Values(
        NearestNameTestData{.objects = languages, .key = "cpang", .max_distance = 3, .result = "cpp"},
        NearestNameTestData{.objects = languages, .key = "cpang", .max_distance = 2, .result = ""},
        NearestNameTestData{.objects = languages, .key = "c", .max_distance = 2, .result = "cpp"},
        NearestNameTestData{.objects = languages, .key = "c", .max_distance = 1, .result = ""},
        NearestNameTestData{.objects = languages, .key = "jemalloc", .max_distance = 6, .result = "java"},
        NearestNameTestData{.objects = languages, .key = "hemalloc", .max_distance = 5, .result = ""}
    )
);

USERVER_NAMESPACE_END
