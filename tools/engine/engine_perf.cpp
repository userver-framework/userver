#include <atomic>
#include <fstream>
#include <iostream>
#include <list>

#include <boost/program_options.hpp>

#include <engine/async.hpp>
#include <engine/sleep.hpp>

#include <engine/task/task_context.hpp>
#include <logging/spdlog.hpp>

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
  desc.add_options()("help,h", "produce help message")(
      "log-level", po::value(&config.log_level),
      "log level (trace, debug, info, warning, error)")(
      "log-file", po::value(&config.logfile),
      "log filename (empty for synchronous stderr)")(
      "count,c", po::value<size_t>(), "request count")(
      "coroutines", po::value<size_t>(&config.coroutines),
      "client coroutine count")("worker-threads",
                                po::value<size_t>(&config.worker_threads),
                                "worker thread count")(
      "io-threads", po::value<size_t>(&config.io_threads), "io thread count")(
      "cycle", po::value<size_t>(&config.cycle), "cycle iterations")(
      "memory,m", po::value<size_t>(&config.memory),
      "memory used in each coro");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  if (vm.count("count")) config.count = vm["count"].as<size_t>();

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
  LOG_INFO() << "Starting thread " << std::this_thread::get_id();

  engine::ev::Thread io_thread("io_thread");
  engine::ev::ThreadControl thread_control(io_thread);
  auto& tp = engine::current_task::GetTaskProcessor();

  WorkerContext worker_context{{0}, 2000, 0, config};

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.resize(config.coroutines);
  auto tp1 = std::chrono::system_clock::now();
  LOG_WARNING() << "Creating workers...";
  for (size_t i = 0; i < config.coroutines; ++i) {
    tasks[i] = engine::Async(tp, &Worker, std::ref(worker_context));
  }
  LOG_WARNING() << "All workers are started " << std::this_thread::get_id();

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
  const Config& config = ParseConfig(argc, argv);

  if (!config.logfile.empty())
    logging::SetDefaultLogger(
        logging::MakeFileLogger("default", config.logfile));
  logging::DefaultLogger()->set_level(static_cast<spdlog::level::level_enum>(
      logging::LevelFromString(config.log_level)));
  LOG_WARNING() << "Starting using requests=" << config.count
                << " coroutines=" << config.coroutines;

  engine::coro::PoolConfig pool_config;
  engine::ev::ThreadPool thread_pool(1, "thread_pool");
  engine::TaskProcessor::CoroPool coro_pool(
      pool_config, &engine::impl::TaskContext::CoroFunc);
  engine::TaskProcessorConfig tp_config;
  tp_config.worker_threads = config.worker_threads;
  tp_config.thread_name = "task_processor";
  engine::TaskProcessor task_processor(tp_config, coro_pool, thread_pool);

  signal(SIGPIPE, SIG_IGN);

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_bool done{false};

  auto cb = [&]() {
    DoWork(config);

    std::lock_guard<std::mutex> lock(mutex);
    done = true;
    cv.notify_all();
  };
  engine::Async(task_processor, std::move(cb)).Detach();
  auto timeout = std::chrono::seconds(1000);
  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait_for(lock, timeout, [&done]() { return done.load(); });
  }
  LOG_WARNING() << "Exit";
}
