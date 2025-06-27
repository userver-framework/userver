#include <cstdlib>
#include <fstream>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

#include <userver/clients/http/client.hpp>
#include <userver/engine/async.hpp>
#include <userver/engine/get_all.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/http/http_version.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace {

namespace http = clients::http;

struct Config {
    std::string log_level = "error";
    std::string logfile = "";
    std::size_t count = 1000;
    std::size_t coroutines = 1;
    std::size_t worker_threads = 1;
    std::size_t io_threads = 1;
    long timeout_ms = 1000;
    bool multiplexing = false;
    std::size_t max_host_connections = 0;
    USERVER_NAMESPACE::http::HttpVersion http_version = USERVER_NAMESPACE::http::HttpVersion::k11;
    std::string url_file;
};

struct WorkerContext {
    std::atomic<std::uint64_t> counter{0};
    const std::uint64_t print_each_counter;
    std::uint64_t response_len;

    http::Client& http_client;
    const Config& config;
    const std::vector<std::string>& urls;
};

Config ParseConfig(int argc, const char* const argv[]) {
    namespace po = boost::program_options;

    Config c;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
      ("help,h", "produce help message")
      ("log-level", po::value(&c.log_level)->default_value(c.log_level), "One of: trace, debug, info, warning, error")
      ("log-file", po::value(&c.logfile)->default_value(c.logfile), "log filename (empty for synchronous stderr)")
      ("url-file,f", po::value(&c.url_file)->default_value(c.url_file), "input file")
      ("count,c", po::value(&c.count)->default_value(c.count), "request count")
      ("coroutines", po::value(&c.coroutines)->default_value(c.coroutines), "client coroutine count")
      ("worker-threads", po::value(&c.worker_threads)->default_value(c.worker_threads), "worker thread count")
      ("io-threads", po::value(&c.io_threads)->default_value(c.io_threads), "IO thread count")
      ("timeout,t", po::value(&c.timeout_ms)->default_value(c.timeout_ms), "request timeout in ms")
      ("multiplexing", "enable HTTP/2 multiplexing")
      ("http-version,V", po::value<std::string>(), "http version, possible values: 1.0, 1.1, 2, 2-prior")
      ("max-host-connections", po::value(&c.max_host_connections)->default_value(c.max_host_connections)
          , "maximum HTTP connection number to a single host")
    ;
    // clang-format on

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        std::exit(0);  // NOLINT(concurrency-mt-unsafe)
    }

    c.multiplexing = vm.count("multiplexing");
    if (vm.count("http-version")) {
        c.http_version = USERVER_NAMESPACE::http::HttpVersionFromString(vm["http-version"].as<std::string>());
    }

    if (c.url_file.empty()) {
        std::cerr << "url-file is undefined" << std::endl;
        std::cout << desc << std::endl;
        std::exit(1);  // NOLINT(concurrency-mt-unsafe)
    }

    return c;
}

std::vector<std::string> ReadUrls(const Config& config) {
    std::vector<std::string> urls;

    std::ifstream infile(config.url_file);
    if (!infile.is_open()) {
        throw std::runtime_error("Failed to open url-file\n");
    }

    std::string line;
    while (std::getline(infile, line)) urls.emplace_back(std::move(line));

    if (urls.empty()) {
        throw std::runtime_error("No URL in URL file!");
    }

    return urls;
}

http::Request CreateRequest(http::Client& http_client, const Config& config, const std::string& url) {
    return http_client.CreateRequest()
        .get(url)
        .timeout(config.timeout_ms)
        .retry()
        .verify(false)
        .http_version(config.http_version);
}

void Worker(WorkerContext& context) {
    LOG_DEBUG() << "Worker started";
    for (;;) {
        const uint32_t idx = context.counter++;
        if (idx >= context.config.count) break;
        if (idx % context.print_each_counter == 0) std::cerr << ".";

        const std::string& url = context.urls[idx % context.urls.size()];

        try {
            const auto ts1 = std::chrono::system_clock::now();
            auto request = CreateRequest(context.http_client, context.config, url);
            const auto ts2 = std::chrono::system_clock::now();
            LOG_DEBUG() << "CreateRequest";

            auto response = request.perform();
            context.response_len += response->body().size();
            LOG_DEBUG() << "Got response body_size=" << response->body().size();
            const auto ts3 = std::chrono::system_clock::now();
            LOG_INFO() << "timings create=" << std::chrono::duration_cast<std::chrono::microseconds>(ts2 - ts1).count()
                       << "us "
                       << "response=" << std::chrono::duration_cast<std::chrono::microseconds>(ts3 - ts2).count()
                       << "us";
        } catch (const std::exception& e) {
            LOG_ERROR() << "Exception: " << e;
        } catch (...) {
            LOG_ERROR() << "Non-std::exception exception";
        }
    }
    LOG_INFO() << "Worker stopped";
}

void DoWork(const Config& config, const std::vector<std::string>& urls) {
    LOG_INFO() << "Starting thread " << std::this_thread::get_id();

    auto& tp = engine::current_task::GetTaskProcessor();
    http::Client http_client{{"", config.io_threads}, tp, std::vector<utils::NotNull<clients::http::Plugin*>>{}};
    LOG_INFO() << "Client created";

    http_client.SetMultiplexingEnabled(config.multiplexing);
    if (config.max_host_connections > 0) http_client.SetMaxHostConnections(config.max_host_connections);

    WorkerContext worker_context{{0}, 2000, 0, http_client, config, urls};

    std::vector<engine::TaskWithResult<void>> tasks;
    tasks.resize(config.coroutines);
    const auto tp1 = std::chrono::system_clock::now();
    LOG_WARNING() << "Creating workers...";
    for (std::size_t i = 0; i < config.coroutines; ++i) {
        tasks[i] = engine::AsyncNoSpan(tp, &Worker, std::ref(worker_context));
    }
    LOG_WARNING() << "All workers are started " << std::this_thread::get_id();

    engine::GetAll(tasks);

    const auto tp2 = std::chrono::system_clock::now();
    auto rps = config.count * 1000 / (std::chrono::duration_cast<std::chrono::milliseconds>(tp2 - tp1).count() + 1);

    std::cerr << std::endl;
    LOG_CRITICAL() << "counter = " << worker_context.counter.load()
                   << " sum response body size = " << worker_context.response_len << " average RPS = " << rps;
}

}  // namespace

int main(int argc, const char* const argv[]) {
    const Config config = ParseConfig(argc, argv);

    const auto level = logging::LevelFromString(config.log_level);
    const logging::DefaultLoggerGuard guard{
        config.logfile.empty() ? logging::MakeStderrLogger("default", logging::Format::kTskv, level)
                               : logging::MakeFileLogger("default", config.logfile, logging::Format::kTskv, level)};

    LOG_WARNING() << "Starting using requests=" << config.count << " coroutines=" << config.coroutines
                  << " timeout=" << config.timeout_ms << "ms";
    LOG_WARNING() << "multiplexing =" << (config.multiplexing ? "enabled" : "disabled")
                  << " max_host_connections=" << config.max_host_connections;

    const std::vector<std::string> urls = ReadUrls(config);
    engine::RunStandalone(config.worker_threads, [&]() { DoWork(config, urls); });

    LOG_WARNING() << "Finished";
}
