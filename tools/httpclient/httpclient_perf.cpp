#include <openssl/err.h>

#include <fstream>
#include <iostream>
#include <list>

#include <boost/program_options.hpp>
#include <logging/spdlog.hpp>

#include <clients/http/client.hpp>
#include <engine/async.hpp>
#include <engine/task/task_context.hpp>
#include <engine/task/task_processor.hpp>
#include <logging/log.hpp>

namespace http = clients::http;

namespace {

struct Config {
  std::string log_level = "error";
  std::string logfile = "";
  size_t count = 1000;
  size_t coroutines = 1;
  size_t worker_threads = 1;
  size_t io_threads = 1;
  long timeout_ms = 1000;
  size_t pipeline_length = 1;
  size_t max_host_connections = 0;
  curl::easy::http_version_t http_version = curl::easy::http_version_1_1;
  std::string url_file;
};

struct WorkerContext {
  std::atomic<uint64_t> counter{0};
  const uint64_t print_each_counter;
  uint64_t response_len;

  http::Client& http_client;
  const Config& config;
  const std::vector<std::string>& urls;
};

Config ParseConfig(int argc, char* argv[]) {
  namespace po = boost::program_options;

  Config config;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "produce help message")(
      "log-level",
      po::value(&config.log_level)->default_value(config.log_level),
      "log level (trace, debug, info, warning, error)")(
      "log-file", po::value(&config.logfile)->default_value(config.logfile),
      "log filename (empty for synchronous stderr)")(
      "url-file,f", po::value(&config.url_file)->default_value(config.url_file),
      "input file")("count,c",
                    po::value(&config.count)->default_value(config.count),
                    "request count")(
      "coroutines",
      po::value(&config.coroutines)->default_value(config.coroutines),
      "client coroutine count")(
      "worker-threads",
      po::value(&config.worker_threads)->default_value(config.worker_threads),
      "worker thread count")(
      "io-threads",
      po::value(&config.io_threads)->default_value(config.io_threads),
      "io thread count")(
      "timeout,t",
      po::value(&config.timeout_ms)->default_value(config.timeout_ms),
      "request timeout in ms")(
      "pipeline",
      po::value(&config.pipeline_length)->default_value(config.pipeline_length),
      "HTTP pipeline length (0 for disable)")(
      "http-version,h", po::value<std::string>(),
      "http version, possible values: 1.0, 1.1, 2.0-prior")(
      "max-host-connections",
      po::value(&config.max_host_connections)
          ->default_value(config.max_host_connections),
      "maximum HTTP connection number to a single host");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  if (vm.count("count")) config.count = vm["count"].as<size_t>();
  if (vm.count("url-file")) config.url_file = vm["url-file"].as<std::string>();
  if (vm.count("pipeline"))
    config.pipeline_length = vm["pipeline"].as<size_t>();
  if (vm.count("max-host-connections"))
    config.max_host_connections = vm["max-host-connections"].as<size_t>();
  if (vm.count("http-version")) {
    auto value = vm["http-version"].as<std::string>();
    if (value == "1.0")
      config.http_version = curl::easy::http_version_1_0;
    else if (value == "1.1")
      config.http_version = curl::easy::http_version_1_1;
    else if (value == "2.0-prior")
      config.http_version = curl::easy::http_version_2_0_prior_knowledge;
    else {
      std::cerr << "--http-version value is unknown" << std::endl;
      exit(1);
    }
  }

  if (config.url_file.empty()) {
    std::cerr << "url-file is undefined" << std::endl;
    std::cout << desc << std::endl;
    exit(1);
  }

  return config;
}

std::vector<std::string> ReadUrls(const Config& config) {
  std::vector<std::string> urls;

  std::ifstream infile(config.url_file);
  if (!infile.is_open()) {
    std::cerr << "failed to open url-file\n";
    exit(1);
  }

  std::string line;
  while (std::getline(infile, line)) urls.emplace_back(std::move(line));

  if (urls.empty()) throw std::runtime_error("No URL in URL file!");

  return urls;
}

std::shared_ptr<http::Request> CreateRequest(http::Client& http_client,
                                             const Config& config,
                                             const std::string& url) {
  return http_client.CreateRequest()
      ->get(url)
      ->timeout(config.timeout_ms)
      ->retry()
      ->verify(false)
      ->http_version(config.http_version);
}

void Worker(WorkerContext& context) {
  LOG_DEBUG() << "Worker started";
  while (1) {
    const uint32_t idx = context.counter++;
    if (idx >= context.config.count) break;
    if (idx % context.print_each_counter == 0) std::cerr << ".";

    const std::string& url = context.urls[idx % context.urls.size()];

    try {
      auto ts1 = std::chrono::system_clock::now();
      const std::shared_ptr<http::Request> request =
          CreateRequest(context.http_client, context.config, url);
      auto ts2 = std::chrono::system_clock::now();
      LOG_DEBUG() << "CreateRequest";

      auto response = request->perform();
      context.response_len += response->body().size();
      LOG_DEBUG() << "Got response body_size=" << response->body().size();
      auto ts3 = std::chrono::system_clock::now();
      LOG_INFO() << "timings create="
                 << std::chrono::duration_cast<std::chrono::microseconds>(ts2 -
                                                                          ts1)
                        .count()
                 << "us "
                 << "response="
                 << std::chrono::duration_cast<std::chrono::microseconds>(ts3 -
                                                                          ts2)
                        .count()
                 << "us";
    } catch (const std::exception& e) {
      LOG_ERROR() << "Exception: " << e.what();
    } catch (...) {
      LOG_ERROR() << "Non-std::exception exception";
    }
  }
  LOG_INFO() << "Worker stopped";
}

}  // namespace

void DoWork(const Config& config, const std::vector<std::string>& urls) {
  LOG_INFO() << "Starting thread " << std::this_thread::get_id();

  engine::ev::Thread io_thread("io_thread");
  engine::ev::ThreadControl thread_control(io_thread);
  auto& tp = engine::current_task::GetTaskProcessor();
  auto http_client = http::Client::Create(config.io_threads);
  LOG_INFO() << "Client created";

  if (config.pipeline_length > 1)
    http_client->SetMaxPipelineLength(config.pipeline_length);
  if (config.max_host_connections > 0)
    http_client->SetMaxHostConnections(config.max_host_connections);

  WorkerContext worker_context{{0},    2000, 0, std::ref(*http_client),
                               config, urls};

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
  http_client.reset();
}

int main(int argc, char* argv[]) {
  const Config& config = ParseConfig(argc, argv);

  if (!config.logfile.empty())
    logging::SetDefaultLogger(
        logging::MakeFileLogger("default", config.logfile));
  logging::DefaultLogger()->set_level(static_cast<spdlog::level::level_enum>(
      logging::LevelFromString(config.log_level)));
  LOG_WARNING() << "Starting using requests=" << config.count
                << " coroutines=" << config.coroutines
                << " timeout=" << config.timeout_ms << "ms";
  LOG_WARNING() << "pipeline length =" << config.pipeline_length
                << " max_host_connections=" << config.max_host_connections;

  const std::vector<std::string>& urls = ReadUrls(config);

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
    DoWork(config, urls);

    std::lock_guard<std::mutex> lock(mutex);
    done = true;
    cv.notify_all();
  };

  engine::Async(task_processor, std::move(cb)).Detach();
  auto timeout = std::chrono::seconds(1000);
  {
    std::unique_lock<std::mutex> lock(mutex);
    if (!cv.wait_for(lock, timeout, [&done]() { return done.load(); })) {
      LOG_ERROR() << "Main task has not finished in 1000 seconds, force exit";
    }
  }
  LOG_WARNING() << "Exit";
}
