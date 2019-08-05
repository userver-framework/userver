#include <storages/redis/client.hpp>

#include <redis/sentinel.hpp>

namespace storages {
namespace redis {

std::string CreateTmpKey(const std::string& key, std::string prefix) {
  return ::redis::Sentinel::CreateTmpKey(key, std::move(prefix));
}

void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            boost::optional<ScanOptions::Match> arg) {
  if (arg) {
    args_.emplace_back("MATCH");
    args_.emplace_back(std::move(arg)->Get());
  }
}

void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            boost::optional<ScanOptions::Count> arg) {
  if (arg) {
    args_.emplace_back("COUNT");
    args_.emplace_back(std::to_string(arg->Get()));
  }
}

}  // namespace redis
}  // namespace storages
