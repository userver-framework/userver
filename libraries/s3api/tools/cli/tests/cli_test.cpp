#include "../cli.hpp"

#include <array>
#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <userver/clients/http/error.hpp>
#include <userver/clients/http/standalone_client.hpp>
#include <userver/engine/task/current_task.hpp>
#include <userver/fs/blocking/read.hpp>
#include <userver/fs/blocking/temp_directory.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/header_map.hpp>
#include <userver/utest/http_client.hpp>
#include <userver/utest/http_server_mock.hpp>
#include <userver/utest/utest.hpp>

USERVER_NAMESPACE_BEGIN

namespace s3api::cli {
namespace {

constexpr http::headers::PredefinedHeader kAmzDate{"X-Amz-Date"};
constexpr http::headers::PredefinedHeader kAmzContentSha256{"X-Amz-Content-Sha256"};

std::vector<std::string> BaseArguments() {
    return {
        "--endpoint-url",
        "https://s3.example.com",
        "--bucket",
        "bucket",
        "--key",
        "object",
        "--credentials-file",
        "credentials.json",
        "--region",
        "ru-3"
    };
}

Options OptionsForServer(const utest::HttpServerMock& server) {
    Options options;
    options.endpoint = "http://localhost:" + std::to_string(server.GetPort());
    options.bucket = "bucket";
    options.key = "folder/test object";
    options.region = "ru-3";
    options.timeout_ms = std::chrono::duration_cast<std::chrono::milliseconds>(utest::kMaxTestWaitTime).count();
    return options;
}

TEST(S3Cli, ParsesAwsStyleCommandsAndDefaults) {
    auto arguments = BaseArguments();
    arguments.insert(arguments.end(), {"get-object", "output.bin"});
    const auto get = ParseOptions(arguments);
    EXPECT_EQ(get.command, Command::kGetObject);
    EXPECT_EQ(get.output_file, "output.bin");
    EXPECT_EQ(get.signature_version, "v4");
    EXPECT_EQ(get.timeout_ms, kDefaultTimeoutMs);
    EXPECT_EQ(get.retries, kDefaultRetries);

    arguments = BaseArguments();
    arguments.insert(arguments.end(), {"put-object", "--body", "input.bin", "--if-none-match", "*"});
    const auto put = ParseOptions(arguments);
    EXPECT_EQ(put.body_file, "input.bin");
    EXPECT_EQ(put.command, Command::kPutObjectConditional);
    EXPECT_TRUE(put.if_none_match);
    EXPECT_FALSE(put.if_match);

    EXPECT_TRUE(ParseOptions({"--help"}).help);
    const auto v2 = ParseOptions(
        {"put-object",
         "--endpoint-url",
         "https://s3.example.com/",
         "--bucket",
         "bucket",
         "--key",
         "object",
         "--credentials-file",
         "credentials.json",
         "--signature-version",
         "v2",
         "--body",
         "input.bin",
         "--if-match",
         "\"previous\""}
    );
    EXPECT_EQ(v2.command, Command::kPutObjectConditional);
    EXPECT_EQ(v2.signature_version, "v2");
    EXPECT_EQ(v2.endpoint, "https://s3.example.com");
    EXPECT_EQ(v2.if_match, "\"previous\"");
}

TEST(S3Cli, RejectsUnsupportedOrAmbiguousOperationsBeforeRequests) {
    const std::vector<std::vector<std::string>> invalid_suffixes{
        {"get-object"},
        {"put-object"},
        {"delete-object"},
        {"get-object", "out", "--if-match", "etag"},
        {"get-object", "out", "--body", "in"},
        {"put-object", "--body", "in", "out"},
        {"put-object", "--body", "in", "--if-none-match", "etag"},
        {"put-object", "--body", "in", "--if-none-match", "*", "--if-match", "etag"},
        {"put-object", "--body", "in", "--if-match", ""},
        {"get-object", "out", "--timeout-ms", "0"},
        {"get-object", "out", "--retries", "0"},
        {"get-object", "out", "--signature-version", "v2"},
        {"get-object", "out", "--signature-version", "v3"}
    };
    for (const auto& suffix : invalid_suffixes) {
        auto arguments = BaseArguments();
        arguments.insert(arguments.end(), suffix.begin(), suffix.end());
        EXPECT_THROW(ParseOptions(arguments), std::exception);
    }
    EXPECT_THROW(
        ParseOptions(
            {"get-object",
             "out",
             "--endpoint-url",
             "https://s3.example.com",
             "--bucket",
             "bucket",
             "--key",
             "object",
             "--credentials-file",
             "credentials.json"}
        ),
        std::invalid_argument
    );
}

TEST(S3Cli, InvalidCredentialsNeverExposeTheirContents) {
    const auto credentials = ParseCredentials(R"({"access_key_id":"test-access","secret_key":"test-secret"})");
    EXPECT_EQ(credentials.access_key, "test-access");
    for (const auto& contents :
         {"test-secret",
          R"({"access_key_id":"test-access","secret_key":123})",
          R"({"access_key_id":"test-access","secret_key":""})",
          R"({"secret_key":"test-secret"})"})
    {
        try {
            static_cast<void>(ParseCredentials(contents));
            FAIL() << "Invalid credentials were accepted";
        } catch (const std::invalid_argument& error) {
            EXPECT_EQ(std::string(error.what()).find("test-secret"), std::string::npos);
            EXPECT_EQ(std::string(error.what()).find("test-access"), std::string::npos);
        }
    }
}

TEST(S3Cli, CredentialsReadFailurePreservesOriginalError) {
    const auto directory = fs::blocking::TempDirectory::Create();
    const auto path = directory.GetPath() + "/missing-credentials.json";
    std::string original_error;
    try {
        static_cast<void>(fs::blocking::ReadFileContents(path));
        FAIL() << "Reading a missing file succeeded";
    } catch (const std::runtime_error& error) {
        original_error = error.what();
    }
    ASSERT_FALSE(original_error.empty());
    try {
        static_cast<void>(ReadCredentials(path));
        FAIL() << "Reading missing credentials succeeded";
    } catch (const std::invalid_argument& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find(original_error), std::string::npos);
        EXPECT_NE(message.find("--credentials-file"), std::string::npos);
        EXPECT_NE(message.find("check the path and permissions"), std::string::npos);
    }
}

UTEST(S3Cli, SendsProcessingCompatibleRequestsWithBothAuthenticators) {
    constexpr std::string_view kEtag = "\"object-etag\"";
    const std::string payload{"binary\0payload", 14};
    for (const auto* signature_version : std::array{"v2", "v4"}) {
        std::vector<utest::HttpServerMock::HttpRequest> requests;
        const utest::HttpServerMock server([&](const auto& request) {
            requests.push_back(request);
            return utest::HttpServerMock::HttpResponse{200, {{"ETag", std::string(kEtag)}}, payload};
        });
        auto options = OptionsForServer(server);
        options.signature_version = signature_version;
        const auto credentials = ParseCredentials(R"({"access_key_id":"test-access","secret_key":"test-secret"})");
        auto http_client = clients::http::CreateStandaloneHttpClient(
            {"s3-cli-test", 1},
            {},
            engine::current_task::GetBlockingTaskProcessor()
        );
        const auto client = MakeClient(*http_client, options, credentials);

        options.command = Command::kGetObject;
        const auto result = Execute(*client, options, {});
        EXPECT_EQ(result.body, payload);
        EXPECT_EQ(result.etag, kEtag);

        options.command = Command::kPutObject;
        EXPECT_FALSE(Execute(*client, options, payload).etag);
        options.command = Command::kPutObjectConditional;
        options.if_none_match = true;
        Execute(*client, options, payload);
        options.if_none_match = false;
        options.if_match = kEtag;
        Execute(*client, options, payload);

        ASSERT_EQ(requests.size(), 4);
        EXPECT_EQ(requests[0].method, clients::http::HttpMethod::kGet);
        for (const auto& request : requests) {
            EXPECT_EQ(request.path, "/folder/test%20object");
            EXPECT_EQ(request.headers.at(http::headers::kHost), "bucket.localhost:" + std::to_string(server.GetPort()));
            const auto& authorization = request.headers.at(http::headers::kAuthorization);
            if (options.signature_version == "v2") {
                EXPECT_TRUE(authorization.starts_with("AWS test-access:"));
                EXPECT_TRUE(request.headers.contains(http::headers::kDate));
                EXPECT_FALSE(request.headers.contains(kAmzContentSha256));
            } else {
                EXPECT_TRUE(authorization.starts_with("AWS4-HMAC-SHA256 Credential=test-access/"));
                EXPECT_NE(authorization.find("/ru-3/s3/aws4_request"), std::string::npos);
                EXPECT_TRUE(request.headers.contains(kAmzDate));
                EXPECT_TRUE(request.headers.contains(kAmzContentSha256));
            }
        }
        for (std::size_t index = 1; index < requests.size(); ++index) {
            EXPECT_EQ(requests[index].method, clients::http::HttpMethod::kPut);
            EXPECT_EQ(requests[index].body, payload);
        }
        EXPECT_EQ(requests[1].headers.at(http::headers::kContentType), "text/plain");
        EXPECT_FALSE(requests[1].headers.contains(http::headers::kIfMatch));
        EXPECT_FALSE(requests[1].headers.contains(http::headers::kIfNoneMatch));
        EXPECT_EQ(requests[2].headers.at(http::headers::kContentType), "application/json");
        EXPECT_EQ(requests[2].headers.at(http::headers::kIfNoneMatch), "*");
        EXPECT_EQ(requests[3].headers.at(http::headers::kIfMatch), kEtag);
        EXPECT_FALSE(requests[3].headers.contains(http::headers::kIfNoneMatch));
    }
}

UTEST(S3Cli, PreservesHttpFailuresWithoutExtraRequests) {
    for (const auto status : std::array{404, 409, 412, 403}) {
        std::size_t requests = 0;
        const utest::HttpServerMock server([&](const auto&) {
            ++requests;
            return utest::HttpServerMock::HttpResponse{status, {}, "failure"};
        });
        auto options = OptionsForServer(server);
        options.command = status == 404 ? Command::kGetObject : Command::kPutObjectConditional;
        options.if_match = "\"previous\"";
        const auto credentials = ParseCredentials(R"({"access_key_id":"test-access","secret_key":"test-secret"})");
        auto http_client = utest::CreateHttpClient();
        const auto client = MakeClient(*http_client, options, credentials);
        try {
            Execute(*client, options, "data");
            FAIL() << "HTTP failure was accepted";
        } catch (const clients::http::HttpException& error) {
            EXPECT_EQ(error.code(), status);
        }
        EXPECT_EQ(requests, 1);
    }
}

UTEST(S3Cli, RequiresEtagForGetWithoutSendingHead) {
    std::size_t requests = 0;
    const utest::HttpServerMock server([&](const auto&) {
        ++requests;
        return utest::HttpServerMock::HttpResponse{200, {}, "data"};
    });
    auto options = OptionsForServer(server);
    options.command = Command::kGetObject;
    const auto credentials = ParseCredentials(R"({"access_key_id":"test-access","secret_key":"test-secret"})");
    auto http_client = utest::CreateHttpClient();
    const auto client = MakeClient(*http_client, options, credentials);
    EXPECT_THROW(Execute(*client, options, {}), std::runtime_error);
    EXPECT_EQ(requests, 1);
}

}  // namespace
}  // namespace s3api::cli

USERVER_NAMESPACE_END
