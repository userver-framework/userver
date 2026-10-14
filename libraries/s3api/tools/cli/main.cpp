#include "cli.hpp"

#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <userver/clients/http/error.hpp>
#include <userver/clients/http/standalone_client.hpp>
#include <userver/engine/run_standalone.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/formats/json/serialize.hpp>
#include <userver/formats/json/value_builder.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/write.hpp>
#include <userver/logging/format.hpp>
#include <userver/logging/level.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/logger.hpp>

namespace {

namespace cli = USERVER_NAMESPACE::s3api::cli;

void PrintError(const std::string& type, const std::string& message, std::optional<int> status = std::nullopt) {
    USERVER_NAMESPACE::formats::json::ValueBuilder result;
    result["Error"]["Type"] = type;
    result["Error"]["Message"] = message;
    if (status) {
        result["Error"]["HTTPStatusCode"] = *status;
    }
    std::cerr << USERVER_NAMESPACE::formats::json::ToString(result.ExtractValue()) << '\n';
}

void PrintSuccess(const cli::Result& result) {
    USERVER_NAMESPACE::formats::json::ValueBuilder json;
    json["Success"] = true;
    if (result.etag) {
        json["ETag"] = *result.etag;
    }
    std::cout << USERVER_NAMESPACE::formats::json::ToString(json.ExtractValue()) << '\n';
}

int Run(const cli::Options& options, const cli::Credentials& credentials, std::string body) {
    try {
        cli::Result result;
        USERVER_NAMESPACE::engine::RunStandalone([&] {
            auto http_client = USERVER_NAMESPACE::clients::http::CreateStandaloneHttpClient(
                {"s3-cli", 1},
                {},
                USERVER_NAMESPACE::engine::current_task::GetTaskProcessor()
            );
            const auto client = cli::MakeClient(*http_client, options, credentials);
            result = cli::Execute(*client, options, std::move(body));
        });
        if (options.command == cli::Command::kGetObject) {
            USERVER_NAMESPACE::fs::blocking::RewriteFileContents(options.output_file, result.body);
        }
        PrintSuccess(result);
        return cli::kSuccessExitCode;
    } catch (const USERVER_NAMESPACE::clients::http::HttpException& error) {
        PrintError("Http", "S3 request failed", error.code());
    } catch (const USERVER_NAMESPACE::clients::http::TimeoutException&) {
        PrintError("Timeout", "S3 request timed out; increase --timeout-ms or check connectivity");
    } catch (const USERVER_NAMESPACE::clients::http::BaseException&) {
        PrintError("Transport", "S3 transport failed; check the endpoint, DNS and TLS configuration");
    } catch (const std::exception&) {
        PrintError("Operation", "Operation failed; check the response ETag and output file permissions");
    }
    return cli::kOperationErrorExitCode;
}

}  // namespace

int main(int argc, char* argv[]) {
    const USERVER_NAMESPACE::logging::DefaultLoggerGuard logger{USERVER_NAMESPACE::logging::MakeStderrLogger(
        "s3-cli",
        USERVER_NAMESPACE::logging::Format::kTskv,
        USERVER_NAMESPACE::logging::Level::kNone
    )};
    try {
        const auto options = cli::ParseOptions(std::vector<std::string>{argv + 1, argv + argc});
        if (options.help) {
            std::cout
                << "Usage: userver-s3-cli [options] get-object --bucket B --key K outfile\n"
                   "       userver-s3-cli [options] put-object --bucket B --key K --body infile\n"
                << cli::DescribeOptions() << '\n';
            return cli::kSuccessExitCode;
        }
        const auto credentials = cli::ReadCredentials(options.credentials_file);
        std::string body;
        if (!options.body_file.empty()) {
            try {
                body = USERVER_NAMESPACE::fs::blocking::ReadFileContents(options.body_file);
            } catch (const std::exception&) {
                throw std::invalid_argument("Cannot read --body; specify a readable input file");
            }
        }
        return Run(options, credentials, std::move(body));
    } catch (const std::exception& error) {
        PrintError("Configuration", error.what());
        return cli::kConfigurationErrorExitCode;
    }
}
