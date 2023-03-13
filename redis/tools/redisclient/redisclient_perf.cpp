#include <iostream>

#include <boost/program_options.hpp>

#include <engine/task/task_processor.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>
#include <userver/utest/using_namespace_userver.hpp>

#include <userver/storages/redis/impl/base.hpp>
#include <userver/storages/redis/impl/reply.hpp>

#include <storages/redis/impl/keyshard_impl.hpp>
#include <storages/redis/impl/sentinel.hpp>

struct Config {
  std::string log_level = "error";

  size_t worker_threads = 1;
  size_t ev_threads = 1;
  size_t redis_threads = redis::kDefaultRedisThreadPoolSize;
  size_t sentinel_threads = redis::kDefaultSentinelThreadPoolSize;

  size_t ms = 10;
  size_t requests_per_second = 1000;

  std::vector<std::string> shards;
  std::vector<std::string> sentinels;
};

template <class T>
auto Value(T& var) {
  namespace po = boost::program_options;
  return po::value(&var)->default_value(var);
}

Config ParseConfig(int argc, char* argv[]) {
  namespace po = boost::program_options;

  Config config;
  po::options_description desc("Allowed options");

  /* clang-format off */
  desc.add_options()
    ("help,h", "produce help message")
    ("log-level", Value(config.log_level), "log level (trace, debug, info, warning, error)")
    ("worker-threads", Value(config.worker_threads), "number of coroutine threads")
    ("ev-threads", Value(config.ev_threads), "number of ev threads")
    ("redis-threads", Value(config.redis_threads), "number of data redis threads")
    ("sentinel-threads", Value(config.sentinel_threads), "number of sentinel redis threads")
    ("period-ms", Value(config.ms), "how many ms to fire")
    ("requests-per-second", Value(config.requests_per_second), "how many requests per second to make")
    ("shards", po::value(&config.shards)->required()->multitoken(), "redis shard names")
    ("sentinels", po::value(&config.sentinels)->required()->multitoken(), "redis sentinel hostname:port pairs")
  ;
  /* clang-format on */

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    exit(1);
  }

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  return config;
}

std::vector<secdist::RedisSettings::HostPort> ParseHostPortPairs(
    const std::vector<std::string>& hostports) {
  std::vector<secdist::RedisSettings::HostPort> result;

  for (const auto& hostport : hostports) {
    auto pos = hostport.rfind(':');
    if (pos == std::string::npos)
      throw std::runtime_error("Bad host:port pair: " + hostport);

    secdist::RedisSettings::HostPort hp;
    hp.host = hostport.substr(0, pos);
    hp.port = std::stoi(hostport.substr(pos + 1));
    result.push_back(hp);
  }
  return result;
}

void SetupLogger(const std::string& log_level) {
  logging::SetDefaultLoggerLevel(logging::LevelFromString(log_level));
}

void Work(std::shared_ptr<redis::Sentinel>& sentinel, size_t i) {
  const auto key = "key" + std::to_string(i);
  [[maybe_unused]] auto response =
      sentinel
          ->MakeRequest({"get", key}, key, false,
                        sentinel->GetCommandControl({}))
          .Get();
}

void WaitForStop(engine::TaskProcessor& tp) {
  tp.GetTaskCounter().WaitForExhaustion(std::chrono::milliseconds(100));
}

void PrintStats(std::shared_ptr<redis::Sentinel> sentinel) {
  const auto sentinel_stats = sentinel->GetStatistics(redis::MetricsSettings{});
  const auto total = sentinel_stats.GetShardGroupTotalStatistics();

  const auto& timings = total.timings_percentile;

  std::cout << std::endl;
  std::cout << "timings: ";
  for (auto perc : {0, 50, 90, 95, 99, 100})
    std::cout << " p" << std::to_string(perc) << ": "
              << timings.GetPercentile(perc) << "  ";
  std::cout << std::endl;

  const auto& errors = total.error_count;
  std::cout << "errors: ";
  for (size_t i = 0; i < errors.size(); i++)
    std::cout << redis::ToString(static_cast<redis::ReplyStatus>(i)) << ": "
              << errors[i] << "  ";
  std::cout << std::endl;
}

void Fire(engine::TaskProcessor& task_processor,
          std::shared_ptr<redis::Sentinel> sentinel, size_t per_second,
          std::chrono::milliseconds ms,
          std::chrono::milliseconds show_stats_every) {
  const auto start = std::chrono::steady_clock::now();
  auto last_stats_ts = start;

  const auto per_ms = per_second / 1000.0;
  for (size_t i = 0;; ++i) {
    engine::AsyncNoSpan(task_processor, Work, std::ref(sentinel), i).Detach();

    /* Burn CPU */
    while (true) {
      const auto now = std::chrono::steady_clock::now();
      const auto diff = now - start;
      const double diff_ms =
          std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();

      if (diff > ms) return;

      if (now - last_stats_ts >= show_stats_every) {
        PrintStats(sentinel);
        last_stats_ts = now;
      }

      if (diff_ms >= i / per_ms) break;

      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }
}

void Run(const Config& config) {
  engine::TaskProcessorPoolsConfig tp_config;
  tp_config.ev_threads_num = config.ev_threads;

  engine::RunStandalone(config.worker_threads, tp_config, [&]() {
    auto& task_processor = engine::current_task::GetTaskProcessor();
    auto redis_thread_pools = std::make_shared<redis::ThreadPools>(
        config.redis_threads, config.sentinel_threads);

    secdist::RedisSettings settings;
    settings.shards = config.shards;
    settings.sentinels = ParseHostPortPairs(config.sentinels);

    auto sentinel = redis::Sentinel::CreateSentinel(
        redis_thread_pools, settings, "shard_group_name", "client_name",
        redis::KeyShardFactory(redis::KeyShardCrc32::kName));

    Fire(task_processor, sentinel, config.requests_per_second,
         std::chrono::milliseconds(config.ms), std::chrono::milliseconds(1000));

    LOG_INFO() << "Waiting for responses...";
    WaitForStop(task_processor);
    PrintStats(sentinel);

    LOG_INFO() << "Finished";
  });
}

int main(int argc, char* argv[]) {
  auto config = ParseConfig(argc, argv);
  SetupLogger(config.log_level);
  Run(config);
}
