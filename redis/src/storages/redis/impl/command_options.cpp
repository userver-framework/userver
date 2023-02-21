#include <userver/storages/redis/impl/command_options.hpp>

#include <limits>

USERVER_NAMESPACE_BEGIN

namespace redis {

ZaddOptions operator|(ZaddOptions::Exist exist,
                      ZaddOptions::ReturnValue return_value) {
  return {exist, return_value};
}

ZaddOptions operator|(ZaddOptions::ReturnValue return_value,
                      ZaddOptions::Exist exist) {
  return {exist, return_value};
}

void PutArg(CmdArgs::CmdArgsArray& args_, GeoaddArg arg) {
  args_.emplace_back(std::to_string(arg.lon));
  args_.emplace_back(std::to_string(arg.lat));
  args_.emplace_back(std::move(arg.member));
}

void PutArg(CmdArgs::CmdArgsArray& args_, std::vector<GeoaddArg> arg) {
  for (auto& geoadd_arg : arg) {
    args_.emplace_back(std::to_string(geoadd_arg.lon));
    args_.emplace_back(std::to_string(geoadd_arg.lat));
    args_.emplace_back(std::move(geoadd_arg.member));
  }
}

void PutArg(CmdArgs::CmdArgsArray& args_, const GeoradiusOptions& arg) {
  switch (arg.unit) {
    case GeoradiusOptions::Unit::kM:
      args_.emplace_back("m");
      break;
    case GeoradiusOptions::Unit::kKm:
      args_.emplace_back("km");
      break;
    case GeoradiusOptions::Unit::kMi:
      args_.emplace_back("mi");
      break;
    case GeoradiusOptions::Unit::kFt:
      args_.emplace_back("ft");
      break;
  }

  if (arg.withcoord) args_.emplace_back("WITHCOORD");
  if (arg.withdist) args_.emplace_back("WITHDIST");
  if (arg.withhash) args_.emplace_back("WITHHASH");
  if (arg.count) {
    args_.emplace_back("COUNT");
    args_.emplace_back(std::to_string(arg.count));
  }
  if (arg.sort == GeoradiusOptions::Sort::kAsc)
    args_.emplace_back("ASC");
  else if (arg.sort == GeoradiusOptions::Sort::kDesc)
    args_.emplace_back("DESC");
}

void PutArg(CmdArgs::CmdArgsArray& args_, const GeosearchOptions& arg) {
  switch (arg.unit) {
    case GeosearchOptions::Unit::kM:
      args_.emplace_back("m");
      break;
    case GeosearchOptions::Unit::kKm:
      args_.emplace_back("km");
      break;
    case GeosearchOptions::Unit::kMi:
      args_.emplace_back("mi");
      break;
    case GeosearchOptions::Unit::kFt:
      args_.emplace_back("ft");
      break;
  }

  if (arg.withcoord) args_.emplace_back("WITHCOORD");
  if (arg.withdist) args_.emplace_back("WITHDIST");
  if (arg.withhash) args_.emplace_back("WITHHASH");
  if (arg.count) {
    args_.emplace_back("COUNT");
    args_.emplace_back(std::to_string(arg.count));
  }
  if (arg.sort == GeosearchOptions::Sort::kAsc)
    args_.emplace_back("ASC");
  else if (arg.sort == GeosearchOptions::Sort::kDesc)
    args_.emplace_back("DESC");
}

void PutArg(CmdArgs::CmdArgsArray& args_, const SetOptions& arg) {
  if (arg.milliseconds) {
    args_.emplace_back("PX");
    args_.emplace_back(std::to_string(arg.milliseconds));
  } else if (arg.seconds) {
    args_.emplace_back("EX");
    args_.emplace_back(std::to_string(arg.seconds));
  }
  if (arg.exist == SetOptions::Exist::kSetIfNotExist)
    args_.emplace_back("NX");
  else if (arg.exist == SetOptions::Exist::kSetIfExist)
    args_.emplace_back("XX");
}

void PutArg(CmdArgs::CmdArgsArray& args_, const ZaddOptions& arg) {
  if (arg.exist == ZaddOptions::Exist::kAddIfNotExist)
    args_.emplace_back("NX");
  else if (arg.exist == ZaddOptions::Exist::kAddIfExist)
    args_.emplace_back("XX");

  if (arg.return_value == ZaddOptions::ReturnValue::kChangedCount)
    args_.emplace_back("CH");
}

void PutArg(CmdArgs::CmdArgsArray& args_, const ScanOptions& arg) {
  if (arg.cursor) {
    args_.emplace_back(std::to_string(*arg.cursor));
  } else {
    args_.emplace_back("0");
  }
  if (arg.pattern) {
    args_.emplace_back("MATCH");
    args_.emplace_back(*arg.pattern);
  }
  if (arg.count) {
    args_.emplace_back("COUNT");
    args_.emplace_back(std::to_string(*arg.count));
  }
}

void PutArg(CmdArgs::CmdArgsArray& args_, const ScoreOptions& arg) {
  if (arg.withscores) args_.emplace_back("WITHSCORES");
}

void PutArg(CmdArgs::CmdArgsArray& args_, const RangeOptions& arg) {
  if (arg.offset || arg.count) {
    args_.emplace_back("LIMIT");
    args_.emplace_back(arg.offset ? std::to_string(*arg.offset) : "0");
    args_.emplace_back(std::to_string(
        arg.count ? *arg.count : std::numeric_limits<int64_t>::max()));
  }
}

void PutArg(CmdArgs::CmdArgsArray& args_, const RangeScoreOptions& arg) {
  PutArg(args_, arg.score_options);
  PutArg(args_, arg.range_options);
}

}  // namespace redis

USERVER_NAMESPACE_END
