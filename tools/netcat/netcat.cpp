#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include <userver/engine/async.hpp>
#include <userver/engine/io/socket.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace {

struct Config {
  std::string log_level = "error";
  std::string log_file;
  size_t worker_threads = 1;
  size_t buffer_size = 32 * 1024;
  uint16_t port = 3333;
  bool listen = false;
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
                     "port to listen to")(
      "listen,l", po::bool_switch(&config.listen), "listen mode");

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
  engine::RunStandalone(config.worker_threads, [&] {
    engine::io::Sockaddr addr;
    auto* sa = addr.As<struct sockaddr_in6>();
    sa->sin6_family = AF_INET6;
    sa->sin6_port = htons(config.port);
    sa->sin6_addr = in6addr_loopback;

    engine::io::Socket worksock;
    std::vector<char> buf(config.buffer_size);
    if (config.listen) {
      LOG_INFO() << "Listening on " << addr;
      engine::io::Socket listsock{addr.Domain(),
                                  engine::io::SocketType::kStream};
      listsock.Bind(addr);
      listsock.Listen();
      worksock = listsock.Accept({});
      LOG_INFO() << "Connection from " << worksock.Getpeername();
      listsock.Close();
      while (auto len = worksock.RecvSome(buf.data(), buf.size(), {})) {
        std::cout.write(buf.data(), len);
      }
    } else {
      LOG_INFO() << "Connecting to " << addr;
      worksock =
          engine::io::Socket{addr.Domain(), engine::io::SocketType::kStream};
      worksock.Connect(addr, {});
      while (std::cin.read(buf.data(), buf.size())) {
        const size_t to_send = std::cin.gcount();
        if (to_send != worksock.SendAll(buf.data(), to_send, {})) {
          break;
        }
      }
    }
    worksock.Close();
  });
}
