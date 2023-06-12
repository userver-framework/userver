#include <atomic>
#include <iostream>

#include <boost/program_options.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/logging/log.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace {

struct Config {
  std::string log_level = "error";
  std::string logfile = "";
  size_t count = 1000;
  size_t coroutines = 1;
  size_t worker_threads = 1;
  size_t io_threads = 1;
  size_t cycle = 1000;
  size_t memory = 1000;
};

struct WorkerContext {
  std::atomic<uint64_t> counter{0};
  const uint64_t print_each_counter;
  uint64_t response_len;

  const Config& config;
};

Config ParseConfig(int argc, char* argv[]) {
  namespace po = boost::program_options;

  Config config;
  po::options_description desc("Allowed options");
  desc.add_options()                      //
      ("help,h", "produce help message")  //
      ("log-level",
       po::value(&config.log_level)->default_value(config.log_level),
       "log level (trace, debug, info, warning, error)")  //
      ("log-file", po::value(&config.logfile)->default_value(config.logfile),
       "log filename (empty for synchronous stderr)")  //
      ("count,c", po::value(&config.count)->default_value(config.count),
       "request count")  //
      ("coroutines",
       po::value(&config.coroutines)->default_value(config.coroutines),
       "client coroutine count")  //
      ("worker-threads",
       po::value(&config.worker_threads)->default_value(config.worker_threads),
       "worker thread count")  //
      ("io-threads",
       po::value(&config.io_threads)->default_value(config.io_threads),
       "io thread count")  //
      ("cycle", po::value(&config.cycle)->default_value(config.cycle),
       "cycle iterations")  //
      ("memory,m", po::value(&config.memory)->default_value(config.memory),
       "memory used in each coro")  //
      ;

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  return config;
}

void Worker(WorkerContext& context) {
  LOG_DEBUG() << "Worker started";
  int count = context.config.count;
  size_t cycle = context.config.cycle;

  size_t size = context.config.memory / sizeof(int);
  std::vector<int> arr(size);
  for (int i = 0; i < count; i++) {
    /* some work */
    long x = 56;
    for (size_t j = 0; j < cycle; j++) {
      x = ((x << 2) ^ (x ^ 0x87654)) % (size + i) + 0x345f;
      arr[x % size] = x + i;
    }

    engine::Yield();
  }
  LOG_INFO() << "Worker stopped";
}

}  // namespace

void DoWork(const Config& config) {
  auto& tp = engine::current_task::GetTaskProcessor();

  WorkerContext worker_context{{0}, 2000, 0, config};

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.resize(config.coroutines);
  auto tp1 = std::chrono::system_clock::now();
  LOG_WARNING() << "Creating workers...";
  for (size_t i = 0; i < config.coroutines; ++i) {
    tasks[i] = engine::AsyncNoSpan(tp, &Worker, std::ref(worker_context));
  }
  LOG_WARNING() << "All workers are started ";

  for (auto& task : tasks) task.Get();
  auto tp2 = std::chrono::system_clock::now();
  auto rps = config.count * 1000 /
             (std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1)
                  .count() +
              1);

  std::cerr << std::endl;
  LOG_WARNING() << "counter = " << worker_context.counter.load()
                << " sum response body size = " << worker_context.response_len
                << " average RPS = " << rps;
}

int main(int argc, char* argv[]) {
  const Config config = ParseConfig(argc, argv);

  logging::LoggerPtr logger;
  const auto level = logging::LevelFromString(config.log_level);
  if (!config.logfile.empty()) {
    logger = logging::MakeFileLogger("default", config.logfile,
                                     logging::Format::kTskv, level);
  } else {
    logger =
        logging::MakeStderrLogger("default", logging::Format::kTskv, level);
  }
  logging::DefaultLoggerGuard guard{logger};

  LOG_WARNING() << "Starting using requests=" << config.count
                << " coroutines=" << config.coroutines;

  engine::RunStandalone(config.worker_threads, [&]() { DoWork(config); });
}
