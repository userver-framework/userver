#include <utest/gtest_hooks.hpp>

#include <cstdlib>
#include <iostream>
#include <string>

#include <gtest/gtest.h>
#include <boost/program_options.hpp>

#include <userver/logging/level.hpp>
#include <userver/utils/fast_scope_guard.hpp>

namespace testing::internal {
// from gtest.cc
extern bool g_help_flag;
}  // namespace testing::internal

namespace {

struct Config {
  USERVER_NAMESPACE::logging::Level log_level =
      USERVER_NAMESPACE::logging::Level::kNone;
};

Config ParseConfig(int argc, char** argv) {
  namespace po = boost::program_options;

  po::options_description desc("Allowed options");
  desc.add_options()("log-level,l",
                     po::value<std::string>()->default_value("none"),
                     "logging level");

  if (testing::internal::g_help_flag) {
    std::cout << desc << std::endl;
    return {};
  }

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  Config config;
  if (vm.count("log-level"))
    config.log_level = USERVER_NAMESPACE::logging::LevelFromString(
        vm["log-level"].as<std::string>());

  return config;
}

class PhdrCacheScope final {
 public:
  PhdrCacheScope() { USERVER_NAMESPACE::utest::impl::InitPhdrCache(); }

  ~PhdrCacheScope() { USERVER_NAMESPACE::utest::impl::TeardownPhdrCache(); }
};

}  // namespace

int main(int argc, char** argv) {
  USERVER_NAMESPACE::utest::impl::FinishStaticInit();

  ::testing::InitGoogleTest(&argc, argv);

  const auto config = ParseConfig(argc, argv);

  USERVER_NAMESPACE::utest::impl::InitMockNow();

  USERVER_NAMESPACE::utest::impl::SetLogLevel(config.log_level);

  const PhdrCacheScope phdr_cache_scope{};
  return RUN_ALL_TESTS();
}
