#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <boost/program_options.hpp>

#include <userver/engine/io/socket.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/logging/log.hpp>

#include <userver/utest/using_namespace_userver.hpp>

namespace {

struct Config {
    std::string log_level = "error";
    std::string log_file;
    std::size_t worker_threads = 1;
    std::size_t buffer_size = 32 * 1024;
    std::uint16_t port = 3333;
    bool listen = false;
};

Config ParseConfig(int argc, char** argv) {
    namespace po = boost::program_options;

    Config c;
    po::options_description desc("Allowed options");

    // clang-format off
    desc.add_options()
      ("help,h", "produce help message")
      ("log-level", po::value(&c.log_level)->default_value(c.log_level), "log level (trace, debug, info, warning, error)")
      ("log-file", po::value(&c.log_file), "log filename (sync logging to stderr by default)")
      ("worker-threads,t", po::value(&c.worker_threads)->default_value(c.worker_threads), "worker thread count")
      ("buffer,b", po::value(&c.buffer_size)->default_value(c.buffer_size), "buffer size")
      ("port,p", po::value(&c.port)->default_value(c.port), "port to listen to")
      ("listen,l", po::bool_switch(&c.listen), "listen mode")
    ;
    // clang-format on

    po::variables_map vm;
    try {
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    } catch (const std::exception& ex) {
        std::cerr << "Cannot parse command line: " << ex.what() << '\n';
        std::exit(1);  // NOLINT(concurrency-mt-unsafe)
    }

    if (vm.count("help")) {
        std::cout << desc << '\n';
        std::exit(0);  // NOLINT(concurrency-mt-unsafe)
    }

    return c;
}

}  // namespace

int main(int argc, char** argv) {
    auto config = ParseConfig(argc, argv);
    engine::RunStandalone(config.worker_threads, [&] {
        auto addr = engine::io::Sockaddr::MakeLoopbackAddress();
        addr.SetPort(config.port);

        engine::io::Socket worksock;
        std::vector<char> buf(config.buffer_size);
        if (config.listen) {
            LOG_INFO() << "Listening on " << addr;
            engine::io::Socket listsock{addr.Domain(), engine::io::SocketType::kStream};
            listsock.Bind(addr);
            listsock.Listen();
            worksock = listsock.Accept({});
            LOG_INFO() << "Connection from " << worksock.Getpeername();
            listsock.Close();
            while (auto len = worksock.RecvSome(buf.data(), buf.size(), {})) {
                std::cout.write(buf.data(), len);
                std::cout << std::endl;
            }
        } else {
            LOG_INFO() << "Connecting to " << addr;
            worksock = engine::io::Socket{addr.Domain(), engine::io::SocketType::kStream};
            worksock.Connect(addr, {});
            while (std::cin.getline(buf.data(), buf.size(), '\n')) {
                const std::size_t to_send = std::cin.gcount();
                if (to_send != worksock.SendAll(buf.data(), to_send, {})) {
                    break;
                }
            }
        }
        worksock.Close();
    });
}
