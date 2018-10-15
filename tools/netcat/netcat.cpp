#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdlib>
#include <exception>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include <engine/async.hpp>
#include <engine/io/socket.hpp>
#include <engine/standalone.hpp>
#include <logging/log.hpp>

namespace {

struct Config {
  std::string log_level = "error";
  std::string log_file;
  size_t worker_threads = 1;
  size_t buffer_size = 32 * 1024;
  uint16_t port = 3333;
};

Config ParseConfig(int argc, char** argv) {
  namespace po = boost::program_options;

  Config config;
  po::options_description desc("Allowed options");
  desc.add_options()("help,h", "produce help message")(
      "log-level",
      po::value(&config.log_level)->default_value(config.log_level),
      "log level (trace, debug, info, warning, error)")(
      "log-file", po::value(&config.log_file),
      "log filename (sync logging to stderr by default)")(
      "worker-threads,t",
      po::value(&config.worker_threads)->default_value(config.worker_threads),
      "worker thread count")(
      "buffer,b",
      po::value(&config.buffer_size)->default_value(config.buffer_size),
      "buffer size")("port,p",
                     po::value(&config.port)->default_value(config.port),
                     "port to listen to");

  po::variables_map vm;
  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);
  } catch (const std::exception& ex) {
    std::cerr << "Cannot parse command line: " << ex.what() << '\n';
    exit(1);
  }

  if (vm.count("help")) {
    std::cout << desc << '\n';
    exit(0);
  }

  return config;
}

}  // namespace

int main(int argc, char** argv) {
  auto config = ParseConfig(argc, argv);

  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          config.worker_threads, "task_processor",
          engine::impl::MakeTaskProcessorPools());

  signal(SIGPIPE, SIG_IGN);

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_bool done{false};
  std::exception_ptr ex;
  engine::Async(
      *task_processor_holder,
      [&] {
        try {
          struct sockaddr_in6 sa;
          memset(&sa, 0, sizeof(sa));
          sa.sin6_family = AF_INET6;
          sa.sin6_addr = in6addr_any;
          sa.sin6_port = htons(config.port);

          auto listsock = engine::io::Listen(engine::io::Addr(
              engine::io::AddrDomain::kInet6, SOCK_STREAM, 0, &sa));

          auto worksock = listsock.Accept({});
          std::vector<char> buf(config.buffer_size);
          while (auto len = worksock.Recv(buf.data(), buf.size(), {})) {
            std::cout << std::string(buf.data(), len);
            worksock.Send(buf.data(), len, {});
          }
          worksock.Close();
          listsock.Close();
        } catch (const std::exception&) {
          ex = std::current_exception();
        }
        std::lock_guard<std::mutex> lock(mutex);
        done = true;
        cv.notify_all();
      })
      .Detach();

  {
    std::unique_lock<std::mutex> lock(mutex);
    cv.wait(lock, [&done]() { return done.load(); });
  }
  if (ex) std::rethrow_exception(ex);
}
