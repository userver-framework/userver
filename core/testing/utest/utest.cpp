#include <gtest/gtest.h>

#include <boost/program_options.hpp>

#include <engine/run_in_coro.hpp>
#include <logging/log.hpp>
#include <utils/mock_now.hpp>

namespace testing {
namespace internal {
// from gtest.cc
extern bool g_help_flag;
}  // namespace internal
}  // namespace testing

namespace {

struct Config {
  logging::Level log_level = logging::Level::kNone;
};

Config ParseTaxiConfig(int argc, char** argv) {
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
    config.log_level =
        logging::LevelFromString(vm["log-level"].as<std::string>());

  return config;
}

class ResetMockNowListener : public ::testing::EmptyTestEventListener {
  virtual void OnTestEnd(const ::testing::TestInfo&) {
    utils::datetime::MockNowUnset();
  }
};

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  const Config& config = ParseTaxiConfig(argc, argv);

  auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new ResetMockNowListener());

  logging::SetDefaultLoggerLevel(config.log_level);
  return RUN_ALL_TESTS();
}
