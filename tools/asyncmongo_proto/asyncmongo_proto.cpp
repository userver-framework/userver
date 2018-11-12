#include <arpa/inet.h>
#include <netinet/in.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <condition_variable>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <mongoc.h>
#include <boost/program_options.hpp>

#include <engine/async.hpp>
#include <engine/standalone.hpp>
#include <logging/log.hpp>
#include "async_stream.hpp"

namespace asyncmongo {
namespace {

struct Config {
  std::string log_level = "debug";
  std::string log_file;
  size_t worker_threads = 1;
  std::string uri;
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
      "worker thread count")("uri,u", po::value(&config.uri), "mongodb URI");

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

struct MongoScope {
  MongoScope() { mongoc_init(); }
  ~MongoScope() { mongoc_cleanup(); }
};

struct UriDeleter {
  void operator()(mongoc_uri_t* p) const { mongoc_uri_destroy(p); }
};
using UriPtr = std::unique_ptr<mongoc_uri_t, UriDeleter>;

struct ClientDeleter {
  void operator()(mongoc_client_t* p) const { mongoc_client_destroy(p); }
};
using ClientPtr = std::unique_ptr<mongoc_client_t, ClientDeleter>;

class OwnedBson {
 public:
  OwnedBson() { bson_init(&bson_); }
  ~OwnedBson() { bson_destroy(&bson_); }

  bson_t& operator*() { return bson_; }
  bson_t* operator->() { return Get(); }

  bson_t* Get() { return &bson_; }

 private:
  bson_t bson_;
};

struct BsonDeleter {
  void operator()(bson_t* p) const { bson_destroy(p); }
};
using BsonPtr = std::unique_ptr<bson_t, BsonDeleter>;

struct BsonMemDeleter {
  void operator()(char* p) const { bson_free(p); }
};
using BsonCStringPtr = std::unique_ptr<char, BsonMemDeleter>;

}  // namespace

void CoroMain(const Config& config) {
  MongoScope mongo_scope;

  bson_error_t error_buf;

  UriPtr uri(mongoc_uri_new_with_error(config.uri.c_str(), &error_buf));
  if (!uri) {
    LOG_ERROR() << "Cannot parse URI: " << error_buf.message;
    return;
  }

  ClientPtr client(mongoc_client_new_from_uri(uri.get()));
  if (!client) return;
  mongoc_client_set_stream_initiator(client.get(), &AsyncStream::Create,
                                     nullptr);

  mongoc_client_set_appname(client.get(), "asyncmongo-proto");

  OwnedBson command;
  bson_append_int32(command.Get(), "ping", 4, 1);

  OwnedBson reply;
  if (!mongoc_client_command_simple(client.get(), "admin", command.Get(),
                                    nullptr, reply.Get(), &error_buf)) {
    LOG_ERROR() << "Ping failed: " << error_buf.message;
    return;
  }

  BsonCStringPtr reply_json(
      bson_as_relaxed_extended_json(reply.Get(), nullptr));
  LOG_INFO() << "Ping reply: " << reply_json.get();
}

}  // namespace asyncmongo

int main(int argc, char** argv) {
  auto config = asyncmongo::ParseConfig(argc, argv);

  auto task_processor_holder =
      engine::impl::TaskProcessorHolder::MakeTaskProcessor(
          config.worker_threads, "task_processor",
          engine::impl::MakeTaskProcessorPools());

  signal(SIGPIPE, SIG_IGN);

  std::mutex mutex;
  std::condition_variable cv;
  std::atomic_bool done{false};
  std::exception_ptr ex;
  engine::Async(*task_processor_holder,
                [&] {
                  try {
                    asyncmongo::CoroMain(config);
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
