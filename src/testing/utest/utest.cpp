#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include <boost/program_options.hpp>

#include <engine/async.hpp>
#include <engine/standalone.hpp>
#include <logging/log.hpp>

void TestInCoro(std::function<void()> user_cb, size_t worker_threads) {
  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          worker_threads, "task_processor",
          engine::impl::MakeTaskProcessorPools());

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_bool done{false};

  auto cb = [&user_cb, &mutex, &done, &cv]() {
    user_cb();

    std::lock_guard<std::mutex> lock(mutex);
    done = true;
    cv.notify_all();
  };
  engine::Async(*task_processor_holder, std::move(cb)).Detach();

  auto timeout = std::chrono::seconds(10);
  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, timeout, [&done]() { return done.load(); });
  }
  EXPECT_EQ(true, done.load());
}

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

}  // namespace

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  const Config& config = ParseTaxiConfig(argc, argv);

  logging::SetDefaultLoggerLevel(config.log_level);
  return RUN_ALL_TESTS();
}
