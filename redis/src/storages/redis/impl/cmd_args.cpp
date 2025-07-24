#include <storages/redis/impl/cmd_args.hpp>

#include <sstream>

#include <userver/logging/log_helper.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace storages::redis::impl {

void CmdWithArgs::PutArg(const char* arg) {
    UASSERT(arg);
    UASSERT(arg[0] != '\0');
    args_.emplace_back(arg);
}

void CmdWithArgs::PutArg(std::string_view arg) { args_.emplace_back(arg); }

void CmdWithArgs::PutArg(const std::string& arg) { args_.emplace_back(arg); }

void CmdWithArgs::PutArg(std::string&& arg) { args_.emplace_back(std::move(arg)); }

void CmdWithArgs::PutArg(const std::vector<std::string>& arg) {
    for (const auto& str : arg) {
        args_.emplace_back(str);
    }
}

void CmdWithArgs::PutArg(const std::vector<std::pair<std::string, std::string>>& arg) {
    for (const auto& pair : arg) {
        args_.emplace_back(pair.first);
        args_.emplace_back(pair.second);
    }
}

void CmdWithArgs::PutArg(const std::vector<std::pair<double, std::string>>& arg) {
    for (const auto& pair : arg) {
        args_.emplace_back(std::to_string(pair.first));
        args_.emplace_back(pair.second);
    }
}

void CmdWithArgs::PutArg(std::optional<ScanOptionsGeneric::Match> arg) {
    if (arg) {
        args_.emplace_back("MATCH");
        args_.emplace_back(std::move(arg)->Get());
    }
}

void CmdWithArgs::PutArg(std::optional<ScanOptionsGeneric::Count> arg) {
    if (arg) {
        args_.emplace_back("COUNT");
        args_.emplace_back(std::to_string(arg->Get()));
    }
}

void CmdWithArgs::PutArg(GeoaddArg arg) {
    args_.emplace_back(std::to_string(arg.lon));
    args_.emplace_back(std::to_string(arg.lat));
    args_.emplace_back(std::move(arg.member));
}

void CmdWithArgs::PutArg(std::vector<GeoaddArg> arg) {
    for (auto& geoadd_arg : arg) {
        args_.emplace_back(std::to_string(geoadd_arg.lon));
        args_.emplace_back(std::to_string(geoadd_arg.lat));
        args_.emplace_back(std::move(geoadd_arg.member));
    }
}

void CmdWithArgs::PutArg(const GeoradiusOptions& arg) {
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

void CmdWithArgs::PutArg(const GeosearchOptions& arg) {
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

void CmdWithArgs::PutArg(const SetOptions& arg) {
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

void CmdWithArgs::PutArg(const ZaddOptions& arg) {
    if (arg.exist == ZaddOptions::Exist::kAddIfNotExist)
        args_.emplace_back("NX");
    else if (arg.exist == ZaddOptions::Exist::kAddIfExist)
        args_.emplace_back("XX");

    if (arg.compare == ZaddOptions::Compare::kGreaterThan)
        args_.emplace_back("GT");
    else if (arg.compare == ZaddOptions::Compare::kLessThan)
        args_.emplace_back("LT");

    if (arg.return_value == ZaddOptions::ReturnValue::kChangedCount) args_.emplace_back("CH");
}

void CmdWithArgs::PutArg(const ScoreOptions& arg) {
    if (arg.withscores) args_.emplace_back("WITHSCORES");
}

void CmdWithArgs::PutArg(const RangeOptions& arg) {
    if (arg.offset || arg.count) {
        args_.emplace_back("LIMIT");
        args_.emplace_back(arg.offset ? std::to_string(*arg.offset) : "0");
        args_.emplace_back(std::to_string(arg.count ? *arg.count : std::numeric_limits<int64_t>::max()));
    }
}

void CmdWithArgs::PutArg(const RangeScoreOptions& arg) {
    PutArg(arg.score_options);
    PutArg(arg.range_options);
}

logging::LogHelper& operator<<(logging::LogHelper& os, const CmdWithArgs& v) {
    constexpr std::size_t kArgSizeLimit = 1024;

    bool first_arg = true;
    for (const auto& arg : v.args_) {
        if (first_arg)
            first_arg = false;
        else {
            os << " ";
            if (os.IsLimitReached()) {
                os << "...";
                break;
            }
        }

        if (utils::text::IsUtf8(arg)) {
            if (arg.size() <= kArgSizeLimit) {
                os << arg;
            } else {
                std::string_view view{arg};
                view = view.substr(0, kArgSizeLimit);
                utils::text::utf8::TrimViewTruncatedEnding(view);
                os << view << "<...>";
            }
        } else {
            os << "<bin:" << arg.size() << ">";
        }
    }

    return os;
}

logging::LogHelper& operator<<(logging::LogHelper& os, const CmdArgs& v) {
    if (v.commands_.size() > 1) os << "[";
    bool first = true;
    for (const auto& arg_array : v.commands_) {
        if (first)
            first = false;
        else
            os << ", ";

        if (os.IsLimitReached()) {
            os << "...";
            break;
        }

        os << "\"" << arg_array << "\"";
    }
    if (v.commands_.size() > 1) os << "]";
    return os;
}

}  // namespace storages::redis::impl

USERVER_NAMESPACE_END
