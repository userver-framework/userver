#include <userver/storages/redis/command_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis {

void PutArg(USERVER_NAMESPACE::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Match> arg) {
  if (arg) {
    args_.emplace_back("MATCH");
    args_.emplace_back(std::move(arg)->Get());
  }
}

void PutArg(USERVER_NAMESPACE::redis::CmdArgs::CmdArgsArray& args_,
            std::optional<ScanOptionsBase::Count> arg) {
  if (arg) {
    args_.emplace_back("COUNT");
    args_.emplace_back(std::to_string(arg->Get()));
  }
}

}  // namespace storages::redis

USERVER_NAMESPACE_END
