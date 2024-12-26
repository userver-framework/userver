#pragma once

#include <optional>
#include <string>
#include <vector>

#include <userver/logging/fwd.hpp>
#include <userver/utils/strong_typedef.hpp>

#include <userver/storages/redis/command_options.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

class CmdArgs {
public:
    using CmdArgsArray = std::vector<std::string>;
    using CmdArgsChain = std::vector<CmdArgsArray>;

    CmdArgs() = default;

    template <typename... Args>
    CmdArgs(Args&&... _args) {
        Then(std::forward<Args>(_args)...);
    }

    CmdArgs(const CmdArgs& o) = delete;
    CmdArgs(CmdArgs&& o) = default;

    CmdArgs& operator=(const CmdArgs& o) = delete;
    CmdArgs& operator=(CmdArgs&& o) = default;

    template <typename... Args>
    CmdArgs& Then(Args&&... _args);

    CmdArgs Clone() const {
        CmdArgs r;
        r.args = args;
        return r;
    }

    CmdArgsChain args;
};

void PutArg(CmdArgs::CmdArgsArray& args_, const char* arg);

void PutArg(CmdArgs::CmdArgsArray& args_, const std::string& arg);

void PutArg(CmdArgs::CmdArgsArray& args_, std::string&& arg);

void PutArg(CmdArgs::CmdArgsArray& args_, const std::vector<std::string>& arg);

void PutArg(CmdArgs::CmdArgsArray& args_, const std::vector<std::pair<std::string, std::string>>& arg);

void PutArg(CmdArgs::CmdArgsArray& args_, const std::vector<std::pair<double, std::string>>& arg);

void PutArg(CmdArgs::CmdArgsArray& args_, std::optional<ScanOptionsBase::Match> arg);
void PutArg(CmdArgs::CmdArgsArray& args_, std::optional<ScanOptionsBase::Count> arg);
void PutArg(CmdArgs::CmdArgsArray& args_, GeoaddArg arg);
void PutArg(CmdArgs::CmdArgsArray& args_, std::vector<GeoaddArg> arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const GeoradiusOptions& arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const GeosearchOptions& arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const SetOptions& arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const ZaddOptions& arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const ScanOptions& arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const ScoreOptions& arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const RangeOptions& arg);
void PutArg(CmdArgs::CmdArgsArray& args_, const RangeScoreOptions& arg);

logging::LogHelper& operator<<(logging::LogHelper& os, const CmdArgs& v);

template <typename Arg>
typename std::enable_if<std::is_arithmetic<Arg>::value, void>::type
PutArg(CmdArgs::CmdArgsArray& args_, const Arg& arg) {
    args_.emplace_back(std::to_string(arg));
}

template <typename... Args>
CmdArgs& CmdArgs::Then(Args&&... _args) {
    args.emplace_back();
    auto& new_args = args.back();
    new_args.reserve(sizeof...(Args));
    (storages::redis::impl::PutArg(new_args, std::forward<Args>(_args)), ...);
    return *this;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
