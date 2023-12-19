#include <userver/logging/log_filepath.hpp>

#include <gtest/gtest.h>

#include <userver/utils/algo.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

#ifdef USERVER_IMPL_EXPECTED_USERVER_SOURCE_ROOT
constexpr std::string_view kExpectedUserverSourceRoot =
    USERVER_IMPL_EXPECTED_USERVER_SOURCE_ROOT;
#else
constexpr std::string_view kExpectedUserverSourceRoot = "userver/";
#endif

}  // namespace

TEST(LogFilepath, UserverCroppedCorrectly) {
  EXPECT_EQ(USERVER_FILEPATH,
            utils::StrCat(kExpectedUserverSourceRoot,
                          "universal/src/logging/log_filepath_test.cpp"));
}

USERVER_NAMESPACE_END
