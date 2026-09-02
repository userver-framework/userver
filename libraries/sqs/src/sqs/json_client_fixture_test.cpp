#include <sqs/json_client_fixture_test.hpp>

#include <userver/utest/http_client.hpp>
#include <userver/utils/zstring_view.hpp>

#include <fmt/format.h>

#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/DeleteQueueRequest.h>
#include <aws/sqs/model/ListQueuesRequest.h>

#include <chrono>
#include <cstdlib>

USERVER_NAMESPACE_BEGIN

namespace {

namespace sqs_model = Aws::SQS::Model;

constexpr utils::zstring_view kSqsJsonPortEnv = "SQS_JSON_PORT";
constexpr utils::zstring_view kSqsUser = "my_user";
constexpr utils::zstring_view kSqsSecret = "unused";
constexpr utils::zstring_view kSqsRegion = "yandex";
constexpr std::chrono::milliseconds kClientTimeout{30000};

}  // namespace

void SqsJsonClient::SetUp() {
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    const auto* port = std::getenv(kSqsJsonPortEnv.c_str());
    ASSERT_NE(port, nullptr)
        << "SQS_JSON_PORT env missing; check that sqs_recipe is started with "
           "--enable-sqs-json-api";

    endpoint_ = fmt::format("http://localhost:{}/Root", port);
    http_client_ = utest::CreateHttpClient();
    DeleteAllQueues();
}

void SqsJsonClient::TearDown() {
    if (!http_client_) {
        return;
    }
    DeleteAllQueues();
}

std::unique_ptr<sqs::JsonClient> SqsJsonClient::MakeClient() const {
    sqs::ClientSettings settings;
    settings.endpoint = endpoint_;
    settings.region = std::string{kSqsRegion};
    settings.timeout = kClientTimeout;
    settings.verify_ssl = false;

    return std::make_unique<sqs::JsonClient>(
        *http_client_,
        sqs::Credentials{
            .access_key_id = std::string{kSqsUser},
            .secret_key = std::string{kSqsSecret},
        },
        settings
    );
}

Aws::String SqsJsonClient::CreateQueue(sqs::JsonClient& client, std::string_view queue_name) {
    sqs_model::CreateQueueRequest request;
    request.SetQueueName(std::string{queue_name}.c_str());

    const auto outcome = client.CreateQueue(request);
    if (!outcome.IsSuccess()) {
        ADD_FAILURE() << "Failed to create queue: " << GetErrorMessage(outcome.GetError());
        return {};
    }

    return outcome.GetResult().GetQueueUrl();
}

void SqsJsonClient::DeleteAllQueues() {
    auto client = MakeClient();
    Aws::String next_token;

    do {
        sqs_model::ListQueuesRequest list_request;
        if (!next_token.empty()) {
            list_request.SetNextToken(next_token);
        }

        const auto list_outcome = client->ListQueues(list_request);
        if (!list_outcome.IsSuccess()) {
            ADD_FAILURE() << "Failed to list queues: " << GetErrorMessage(list_outcome.GetError());
            return;
        }

        const auto& result = list_outcome.GetResult();
        for (const auto& queue_url : result.GetQueueUrls()) {
            sqs_model::DeleteQueueRequest delete_request;
            delete_request.SetQueueUrl(queue_url);

            const auto delete_outcome = RunWhenQueueIsReady([&] { return client->DeleteQueue(delete_request); });
            if (!delete_outcome.IsSuccess()) {
                ADD_FAILURE()
                    << "Failed to delete queue " << queue_url << ": " << GetErrorMessage(delete_outcome.GetError());
            }
        }

        next_token = result.GetNextToken();
    } while (!next_token.empty());
}

std::string GetErrorMessage(const Aws::Client::AWSError<Aws::SQS::SQSErrors>& error) {
    return fmt::format("{}: {}", error.GetExceptionName(), error.GetMessage());
}

bool IsQueueNotReady(const Aws::Client::AWSError<Aws::SQS::SQSErrors>& error) {
    return std::string_view{error.GetMessage()}.find("nonexistent queue") != std::string_view::npos;
}

USERVER_NAMESPACE_END
