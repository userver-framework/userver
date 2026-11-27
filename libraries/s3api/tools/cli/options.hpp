#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/program_options/options_description.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::cli {

inline constexpr int kDefaultTimeoutMs = 3000;
inline constexpr int kDefaultRetries = 1;
inline constexpr int kSuccessExitCode = 0;
inline constexpr int kOperationErrorExitCode = 1;
inline constexpr int kConfigurationErrorExitCode = 2;
inline constexpr std::string_view kHttpScheme = "http://";
inline constexpr std::string_view kHttpsScheme = "https://";

enum class Command {
    kGetObject,
    kPutObject,
    kPutObjectConditional,
};

struct Options {
    bool help = false;
    Command command = Command::kGetObject;
    std::string endpoint;
    std::string bucket;
    std::string key;
    std::string credentials_file;
    std::string signature_version = "v4";
    std::optional<std::string> region;
    std::string body_file;
    std::string output_file;
    std::optional<std::string> if_match;
    bool if_none_match = false;
    int timeout_ms = kDefaultTimeoutMs;
    int retries = kDefaultRetries;
};

boost::program_options::options_description DescribeOptions();
Options ParseOptions(const std::vector<std::string>& arguments);

}  // namespace s3api::cli

USERVER_NAMESPACE_END
