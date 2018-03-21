#include "base.hpp"

#include <sstream>

namespace storages {
namespace redis {

CmdArgs& CmdArgs::Then(const CmdArgsList& _args) {
  if (!args) args = std::make_shared<CmdArgsChain>();
  args->emplace_back();
  auto& new_args = args->back();
  new_args.reserve(_args.size());

  for (const auto& arg : _args) {
    switch (arg.which()) {
      case 0:
        new_args.emplace_back(std::to_string(boost::get<double>(arg)));
        break;
      case 1:
        new_args.emplace_back(std::to_string(boost::get<int>(arg)));
        break;
      case 2:
        new_args.emplace_back(std::to_string(boost::get<size_t>(arg)));
        break;
      case 3:
        new_args.emplace_back(std::to_string(boost::get<time_t>(arg)));
        break;
      case 4:
        new_args.emplace_back(std::to_string(boost::get<int64_t>(arg)));
        break;
      case 5:
        new_args.emplace_back(std::move(boost::get<std::string>(arg)));
        break;
      case 6:
        for (auto& str : boost::get<std::vector<std::string>>(arg))
          new_args.emplace_back(std::move(str));
        break;
      case 7: {
        GeoaddArg geoadd_arg = boost::get<GeoaddArg>(arg);
        new_args.emplace_back(std::to_string(geoadd_arg.lon));
        new_args.emplace_back(std::to_string(geoadd_arg.lat));
        new_args.emplace_back(std::move(geoadd_arg.member));
        break;
      }
      case 8:
        for (auto& geoadd_arg : boost::get<std::vector<GeoaddArg>>(arg)) {
          new_args.emplace_back(std::to_string(geoadd_arg.lon));
          new_args.emplace_back(std::to_string(geoadd_arg.lat));
          new_args.emplace_back(std::move(geoadd_arg.member));
        }
        break;
      case 9:
        for (auto& pair :
             boost::get<std::vector<std::pair<std::string, std::string>>>(
                 arg)) {
          new_args.emplace_back(std::move(pair.first));
          new_args.emplace_back(std::move(pair.second));
        }
        break;
      case 10: {
        GeoradiusOptions options = boost::get<GeoradiusOptions>(arg);
        if (options.withcoord) new_args.emplace_back("WITHCOORD");
        if (options.withdist) new_args.emplace_back("WITHDIST");
        if (options.withhash) new_args.emplace_back("WITHHASH");
        if (options.count) {
          new_args.emplace_back("COUNT");
          new_args.emplace_back(std::to_string(options.count));
        }
        if (options.sort == GeoradiusOptions::Sort::kAsc)
          new_args.emplace_back("ASC");
        else if (options.sort == GeoradiusOptions::Sort::kDesc)
          new_args.emplace_back("DESC");
        break;
      }
      case 11: {
        SetOptions options = boost::get<SetOptions>(arg);
        if (options.seconds) {
          new_args.emplace_back("EX");
          new_args.emplace_back(std::to_string(options.seconds));
        }
        if (options.milliseconds) {
          new_args.emplace_back("PX");
          new_args.emplace_back(std::to_string(options.milliseconds));
        }
        if (options.exist == SetOptions::Exist::kSetIfNotExist)
          new_args.emplace_back("NX");
        else if (options.exist == SetOptions::Exist::kSetIfExist)
          new_args.emplace_back("XX");
        break;
      }
      case 12: {
        ScoreOptions options = boost::get<ScoreOptions>(arg);
        if (options.withscores) new_args.emplace_back("WITHSCORES");
        break;
      }
    }
  }

  return *this;
}

std::string CmdArgs::ToString() const {
  if (!args || args->empty()) return std::string();
  std::ostringstream os("");
  if (args->size() > 1) os << "[";
  bool first = true;
  for (const auto& arg_array : *args) {
    if (first)
      first = false;
    else
      os << ", ";
    os << "\"";
    bool first_arg = true;
    for (const auto& arg : arg_array) {
      if (first_arg)
        first_arg = false;
      else
        os << " ";
      os << arg;
    }
    os << "\"";
  }
  if (args->size() > 1) os << "]";
  return os.str();
}

const CommandControl command_control_init = {
    /*.timeout_single = */ std::chrono::milliseconds(500),
    /*.timeout_all = */ std::chrono::milliseconds(2000),
    /*.max_retries = */ 4};

}  // namespace redis
}  // namespace storages
