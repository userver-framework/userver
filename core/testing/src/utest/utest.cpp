#include <userver/utest/utest.hpp>

#include <boost/program_options.hpp>

#include <logging/dynamic_debug.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/stacktrace_cache.hpp>
#include <userver/utils/mock_now.hpp>
#include <utils/impl/static_registration.hpp>

namespace testing {

void PrintTo(std::chrono::seconds s, std::ostream* os) {
  *os << s.count() << "s";
}

void PrintTo(std::chrono::milliseconds ms, std::ostream* os) {
  *os << ms.count() << "ms";
}

void PrintTo(std::chrono::microseconds us, std::ostream* os) {
  *os << us.count() << "us";
}

void PrintTo(std::chrono::nanoseconds ns, std::ostream* os) {
  *os << ns.count() << "ns";
}

}  // namespace testing

USERVER_NAMESPACE_BEGIN

namespace formats::json {

void PrintTo(const Value& json, std::ostream* out) { Serialize(json, *out); }

}  // namespace formats::json

USERVER_NAMESPACE_END

namespace testing::internal {
// from gtest.cc
extern bool g_help_flag;
}  // namespace testing::internal

USERVER_NAMESPACE_BEGIN

namespace {

struct Config {
  logging::Level log_level = logging::Level::kNone;
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
    config.log_level =
        logging::LevelFromString(vm["log-level"].as<std::string>());

  return config;
}

class ResetMockNowListener : public ::testing::EmptyTestEventListener {
  void OnTestEnd(const ::testing::TestInfo&) override {
    utils::datetime::MockNowUnset();
  }
};

}  // namespace

USERVER_NAMESPACE_END

int main(int argc, char** argv) {
  USERVER_NAMESPACE::utils::impl::FinishStaticRegistration();

  ::testing::InitGoogleTest(&argc, argv);

  const USERVER_NAMESPACE::Config& config =
      USERVER_NAMESPACE::ParseConfig(argc, argv);

  auto& listeners = ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(new USERVER_NAMESPACE::ResetMockNowListener());

  if (config.log_level <= USERVER_NAMESPACE::logging::Level::kTrace) {
    // A hack for avoiding Boost.Stacktrace memory leak
    USERVER_NAMESPACE::logging::stacktrace_cache::GlobalEnableStacktrace(false);
  }

  USERVER_NAMESPACE::logging::SetDefaultLoggerLevel(config.log_level);
  return RUN_ALL_TESTS();
}
