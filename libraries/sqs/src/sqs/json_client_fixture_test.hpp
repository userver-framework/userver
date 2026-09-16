#pragma once

/// @file sqs/json_client_fixture_test.hpp
/// @brief gtest fixture and helpers for SQS JSON client tests.

#include <chrono>
#include <memory>
#include <string>
#include <string_view>

#include <userver/clients/http/client.hpp>
#include <userver/engine/sleep.hpp>
#include <userver/sqs/json_client.hpp>
#include <userver/utest/utest.hpp>

#include <aws/sqs/SQSErrors.h>

USERVER_NAMESPACE_BEGIN

/// @ingroup userver_utest
///
/// @brief gtest fixture for @ref sqs::JsonClient against the SQS JSON recipe.
///
/// Reads `SQS_JSON_PORT` and creates an HTTP client. Drops all test queues
/// before and after each test, like @ref storages::mongo::utest::MongoTest
/// drops the database.
class SqsJsonClient : public ::testing::Test {
public:
    /// @brief Reads `SQS_JSON_PORT`, creates the HTTP client, and deletes leftover queues.
    void SetUp() override;

    /// @brief Deletes all queues left by the test.
    void TearDown() override;

protected:
    /// @brief Builds a @ref sqs::JsonClient for the recipe endpoint.
    std::unique_ptr<sqs::JsonClient> MakeClient() const;

    /// @brief Creates a queue with @a queue_name.
    Aws::String CreateQueue(sqs::JsonClient& client, std::string_view queue_name);

private:
    void DeleteAllQueues();

    std::string endpoint_;
    std::shared_ptr<clients::http::Client> http_client_;
};

/// @brief Formats an AWS SQS error as `"exception: message"`.
std::string GetErrorMessage(const Aws::Client::AWSError<Aws::SQS::SQSErrors>& error);

/// @brief True if the error means the queue is not ready yet.
bool IsQueueNotReady(const Aws::Client::AWSError<Aws::SQS::SQSErrors>& error);

/// @brief Retries @a operation while the queue is reported as nonexistent.
template <typename Operation>
auto RunWhenQueueIsReady(Operation operation) {
    constexpr int kQueueReadyAttempts = 20;
    constexpr std::chrono::milliseconds kQueueReadyRetryDelay{100};

    auto outcome = operation();
    for (int attempt = 1; attempt < kQueueReadyAttempts && !outcome.IsSuccess() && IsQueueNotReady(outcome.GetError());
         ++attempt)
    {
        engine::SleepFor(kQueueReadyRetryDelay);
        outcome = operation();
    }
    return outcome;
}

USERVER_NAMESPACE_END
