#include <fstream>
#include <iostream>
#include <list>
#include <thread>

#include <boost/program_options.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>

#include <userver/utest/using_namespace_userver.hpp>

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
  bool multiplexing = false;
  size_t max_host_connections = 0;
  http::HttpVersion http_version = http::HttpVersion::k11;
  std::string url_file;
  bool defer_events = false;
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
      "request timeout in ms")("multiplexing", "enable HTTP/2 multiplexing")(
      "http-version,V", po::value<std::string>(),
      "http version, possible values: 1.0, 1.1, 2.0-prior")(
      "max-host-connections",
      po::value(&config.max_host_connections)
          ->default_value(config.max_host_connections),
      "maximum HTTP connection number to a single host")(
      "defer-events",
      po::value(&config.defer_events)->default_value(config.defer_events),
      "whether to defer curl events to a periodic timer");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << std::endl;
    exit(0);
  }

  if (vm.count("multiplexing")) config.multiplexing = true;
  if (vm.count("http-version")) {
    auto value = vm["http-version"].as<std::string>();
    if (value == "1.0")
      config.http_version = http::HttpVersion::k10;
    else if (value == "1.1")
      config.http_version = http::HttpVersion::k11;
    else if (value == "2")
      config.http_version = http::HttpVersion::k2;
    else if (value == "2tls")
      config.http_version = http::HttpVersion::k2Tls;
    else if (value == "2-prior")
      config.http_version = http::HttpVersion::k2PriorKnowledge;
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
      LOG_ERROR() << "Exception: " << e;
    } catch (...) {
      LOG_ERROR() << "Non-std::exception exception";
    }
  }
  LOG_INFO() << "Worker stopped";
}

}  // namespace

void DoWork(const Config& config, const std::vector<std::string>& urls) {
  LOG_INFO() << "Starting thread " << std::this_thread::get_id();

  auto& tp = engine::current_task::GetTaskProcessor();
  http::Client http_client{
      {"", config.io_threads, config.defer_events},
      tp,
      std::vector<utils::NotNull<clients::http::Plugin*>>{}};
  LOG_INFO() << "Client created";

  http_client.SetMultiplexingEnabled(config.multiplexing);
  if (config.max_host_connections > 0)
    http_client.SetMaxHostConnections(config.max_host_connections);

  WorkerContext worker_context{{0},    2000, 0, std::ref(http_client),
                               config, urls};

  std::vector<engine::TaskWithResult<void>> tasks;
  tasks.resize(config.coroutines);
  auto tp1 = std::chrono::system_clock::now();
  LOG_WARNING() << "Creating workers...";
  for (size_t i = 0; i < config.coroutines; ++i) {
    tasks[i] = engine::AsyncNoSpan(tp, &Worker, std::ref(worker_context));
  }
  LOG_WARNING() << "All workers are started " << std::this_thread::get_id();

  for (auto& task : tasks) task.Get();

  auto tp2 = std::chrono::system_clock::now();
  auto rps = config.count * 1000 /
             (std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1)
                  .count() +
              1);

  std::cerr << std::endl;
  LOG_CRITICAL() << "counter = " << worker_context.counter.load()
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
                << " coroutines=" << config.coroutines
                << " timeout=" << config.timeout_ms << "ms";
  LOG_WARNING() << "multiplexing ="
                << (config.multiplexing ? "enabled" : "disabled")
                << " max_host_connections=" << config.max_host_connections;

  const std::vector<std::string> urls = ReadUrls(config);

  engine::RunStandalone(config.worker_threads, [&]() { DoWork(config, urls); });

  LOG_WARNING() << "Exit";
}
