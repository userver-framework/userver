#pragma once

#include <chrono>
#include <string>
#include <vector>

#include <boost/variant.hpp>

namespace storages {
namespace redis {

const int REDIS_ERR_TIMEOUT = 3;
const int REDIS_ERR_NOT_READY = 4;

struct ConnectionInfo {
  std::string host = "localhost";
  int port = 26379;
  std::string password;
};

struct GeoaddArg {
  double lon;
  double lat;
  std::string member;
};

struct GeoradiusOptions {
  enum class Sort { kNone, kAsc, kDesc };

  bool withcoord = false;
  bool withdist = false;
  bool withhash = false;
  size_t count = 0;
  Sort sort = Sort::kNone;
};

struct SetOptions {
  enum class Exist { kSetAlways, kSetIfNotExist, kSetIfExist };

  int seconds = 0;
  int milliseconds = 0;
  Exist exist = Exist::kSetAlways;
};

struct ScoreOptions {
  bool withscores = false;
};

class CmdArgs {
 public:
  using CmdArg =
      boost::variant<double, int, size_t, time_t, int64_t, std::string,
                     std::vector<std::string>, GeoaddArg,
                     std::vector<GeoaddArg>,
                     std::vector<std::pair<std::string, std::string>>,
                     GeoradiusOptions, SetOptions, ScoreOptions>;
  using CmdArgsList = std::vector<CmdArg>;

  using CmdArgsArray = std::vector<std::string>;
  using CmdArgsChain = std::vector<CmdArgsArray>;

  CmdArgs() {}
  CmdArgs(std::initializer_list<CmdArg> _args) { Then(_args); }
  CmdArgs(const CmdArgsList& o) { Then(o); }
  CmdArgs(const CmdArgs& o) = default;
  CmdArgs(CmdArgs&& o) = default;

  CmdArgs& operator=(const CmdArgs& o) = default;
  CmdArgs& operator=(CmdArgs&& o) = default;

  CmdArgs& Then(const CmdArgsList& _args);

  std::string ToString() const;

  std::shared_ptr<CmdArgsChain> args;
};

struct Stat {
  double tps = 0.0;
  double queue = 0.0;
  double inprogress = 0.0;
  double timeouts = 0.0;
};

struct CommandControl {
  std::chrono::milliseconds timeout_single{0};
  std::chrono::milliseconds timeout_all{0};
  size_t max_retries = 0;

  CommandControl() = default;
  CommandControl(std::chrono::milliseconds timeout_single,
                 std::chrono::milliseconds timeout_all, size_t max_retries)
      : timeout_single(timeout_single),
        timeout_all(timeout_all),
        max_retries(max_retries) {}
};

extern const CommandControl command_control_init;

}  // namespace redis
}  // namespace storages
