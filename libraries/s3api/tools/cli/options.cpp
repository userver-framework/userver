#include "options.hpp"

#include <boost/program_options.hpp>
#include <stdexcept>

USERVER_NAMESPACE_BEGIN

namespace s3api::cli {

boost::program_options::options_description DescribeOptions() {
    namespace po = boost::program_options;
    po::options_description description("Options");
    description.add_options()
        ("help,h", "Show usage")
        ("endpoint-url", po::value<std::string>(), "S3 HTTP(S) endpoint without the bucket")
        ("bucket", po::value<std::string>(), "Bucket with virtual-hosted addressing")
        ("key", po::value<std::string>(), "Object key, without URL encoding")
        ("credentials-file", po::value<std::string>(), "JSON file with access_key_id and secret_key")
        ("signature-version", po::value<std::string>()->default_value("v4"), "v2 or v4")
        ("region", po::value<std::string>(), "Signing region, required for v4")
        ("timeout-ms", po::value<int>()->default_value(kDefaultTimeoutMs), "Positive request timeout in milliseconds")
        ("retries", po::value<int>()->default_value(kDefaultRetries), "Positive HTTP driver attempt count")
        ("body", po::value<std::string>(), "Input file for put-object")
        ("if-match", po::value<std::string>(), "Expected ETag for conditional put-object")
        ("if-none-match", po::value<std::string>(), "Use '*' to create an object only if absent");
    return description;
}

namespace {

void RequireNonempty(const std::string& value, const char* message) {
    if (value.empty()) {
        throw std::invalid_argument(message);
    }
}

void ValidateEndpoint(std::string& endpoint) {
    const auto scheme_size = endpoint.starts_with(kHttpsScheme) ? kHttpsScheme.size() : kHttpScheme.size();
    if (!endpoint.starts_with(kHttpsScheme) && !endpoint.starts_with(kHttpScheme)) {
        throw std::invalid_argument("--endpoint-url must start with http:// or https://");
    }
    if (endpoint.ends_with('/')) {
        endpoint.pop_back();
    }
    if (endpoint.size() <= scheme_size || endpoint.find_first_of("/?#@", scheme_size) != std::string::npos) {
        throw std::invalid_argument("--endpoint-url must contain only a scheme and host, optionally a port");
    }
}

void ValidateAuthentication(const Options& options) {
    if (options.signature_version != "v2" && options.signature_version != "v4") {
        throw std::invalid_argument("--signature-version must be v2 or v4");
    }
    if (options.signature_version == "v4" && (!options.region || options.region->empty())) {
        throw std::invalid_argument("Specify --region for signature version v4");
    }
    if (options.signature_version == "v2" && options.region) {
        throw std::invalid_argument("Remove --region when using signature version v2");
    }
    if (options.timeout_ms <= 0 || options.retries <= 0) {
        throw std::invalid_argument("--timeout-ms and --retries must be positive");
    }
}

void ValidateOperation(const Options& options, bool body_specified, bool output_specified) {
    if (options.if_match && options.if_none_match) {
        throw std::invalid_argument("Use only one of --if-match and --if-none-match");
    }
    if (options.command == Command::kGetObject) {
        RequireNonempty(options.output_file, "get-object requires an output file after the command");
        if (body_specified || options.if_match || options.if_none_match) {
            throw std::invalid_argument("--body, --if-match and --if-none-match are supported only for put-object");
        }
    } else {
        RequireNonempty(options.body_file, "put-object requires --body with an input file path");
        if (output_specified) {
            throw std::invalid_argument("put-object does not accept an output file");
        }
    }
}

}  // namespace

Options ParseOptions(const std::vector<std::string>& arguments) {
    namespace po = boost::program_options;
    auto description = DescribeOptions();
    po::options_description positional_options;
    positional_options.add_options()("command", po::value<std::string>())("outfile", po::value<std::string>());
    description.add(positional_options);
    po::positional_options_description positional;
    positional.add("command", 1).add("outfile", 1);
    po::variables_map values;
    po::store(po::command_line_parser(arguments).options(description).positional(positional).run(), values);
    po::notify(values);
    Options options;
    options.help = values.count("help") != 0;
    if (options.help) {
        return options;
    }
    const auto read_string = [&values](const char* name) {
        return values.count(name) ? values[name].as<std::string>() : std::string{};
    };
    const auto command_name = read_string("command");
    options.endpoint = read_string("endpoint-url");
    options.bucket = read_string("bucket");
    options.key = read_string("key");
    options.credentials_file = read_string("credentials-file");
    options.signature_version = read_string("signature-version");
    options.body_file = read_string("body");
    options.output_file = read_string("outfile");
    options.timeout_ms = values["timeout-ms"].as<int>();
    options.retries = values["retries"].as<int>();
    if (values.count("region")) {
        options.region = read_string("region");
    }
    if (values.count("if-match")) {
        options.if_match = read_string("if-match");
        RequireNonempty(*options.if_match, "--if-match requires a nonempty ETag");
    }
    if (values.count("if-none-match")) {
        if (read_string("if-none-match") != "*") {
            throw std::invalid_argument("--if-none-match supports only '*'");
        }
        options.if_none_match = true;
    }
    if (command_name == "get-object") {
        options.command = Command::kGetObject;
    } else if (command_name == "put-object") {
        options
            .command = options.if_match || options.if_none_match ? Command::kPutObjectConditional : Command::kPutObject;
    } else {
        throw std::invalid_argument("Specify get-object or put-object; use --help for usage");
    }
    RequireNonempty(options.endpoint, "Specify --endpoint-url");
    RequireNonempty(options.bucket, "Specify --bucket");
    RequireNonempty(options.key, "Specify --key");
    RequireNonempty(options.credentials_file, "Specify --credentials-file");
    ValidateEndpoint(options.endpoint);
    ValidateAuthentication(options);
    ValidateOperation(options, values.count("body") != 0, values.count("outfile") != 0);
    return options;
}

}  // namespace s3api::cli

USERVER_NAMESPACE_END
