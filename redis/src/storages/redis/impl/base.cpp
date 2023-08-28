#include <userver/storages/redis/impl/base.hpp>

#include <sstream>

#include <userver/logging/log_helper.hpp>
#include <userver/utils/text.hpp>

USERVER_NAMESPACE_BEGIN

namespace redis {

void PutArg(CmdArgs::CmdArgsArray& args_, const char* arg) {
  args_.emplace_back(arg);
}

void PutArg(CmdArgs::CmdArgsArray& args_, const std::string& arg) {
  args_.emplace_back(arg);
}

void PutArg(CmdArgs::CmdArgsArray& args_, std::string&& arg) {
  args_.emplace_back(std::move(arg));
}

void PutArg(CmdArgs::CmdArgsArray& args_, const std::vector<std::string>& arg) {
  for (const auto& str : arg) args_.emplace_back(str);
}

void PutArg(CmdArgs::CmdArgsArray& args_,
            const std::vector<std::pair<std::string, std::string>>& arg) {
  for (const auto& pair : arg) {
    args_.emplace_back(pair.first);
    args_.emplace_back(pair.second);
  }
}

void PutArg(CmdArgs::CmdArgsArray& args_,
            const std::vector<std::pair<double, std::string>>& arg) {
  for (const auto& pair : arg) {
    args_.emplace_back(std::to_string(pair.first));
    args_.emplace_back(pair.second);
  }
}

logging::LogHelper& operator<<(logging::LogHelper& os, const CmdArgs& v) {
  constexpr std::size_t kArgSizeLimit = 1024;

  if (v.args.size() > 1) os << "[";
  bool first = true;
  for (const auto& arg_array : v.args) {
    if (first)
      first = false;
    else
      os << ", ";

    if (os.IsLimitReached()) {
      os << "...";
      break;
    }

    os << "\"";
    bool first_arg = true;
    for (const auto& arg : arg_array) {
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
    os << "\"";
  }
  if (v.args.size() > 1) os << "]";
  return os;
}

}  // namespace redis

USERVER_NAMESPACE_END
