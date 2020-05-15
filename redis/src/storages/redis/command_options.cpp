#include <storages/redis/command_options.hpp>

namespace storages {
namespace redis {

void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Match> arg) {
  if (arg) {
    args_.emplace_back("MATCH");
    args_.emplace_back(std::move(arg)->Get());
  }
}

void PutArg(::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Count> arg) {
  if (arg) {
    args_.emplace_back("COUNT");
    args_.emplace_back(std::to_string(arg->Get()));
  }
}

}  // namespace redis
}  // namespace storages
