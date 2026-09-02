#pragma once

/// @file userver/sqs/json_client.hpp
/// @brief @copybrief sqs::JsonClient

#include <chrono>
#include <memory>
#include <string>

#include <aws/core/http/HttpTypes.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/sqs/SQSServiceClientModel.h>

USERVER_NAMESPACE_BEGIN

namespace clients::http {
class Client;
}

/// @brief Amazon SQS-compatible JSON client.
///
/// For more information see @ref scripts/docs/en/userver/libraries/sqs.md .
namespace sqs {

/// Extra authentication headers that are not part of AWS SigV4 keys.
struct ExtraCredentials {
    /// Value of `X-YaCloud-SubjectToken` for IAM-token authenticated requests.
    Aws::String cloud_iam_token;

    /// Prebuilt `Authorization` header. When set, SigV4 signing is skipped.
    Aws::String authorization_override;
};

/// Access keys used to sign requests with AWS SigV4.
struct Credentials {
    std::string access_key_id;
    std::string secret_key;
    std::string session_token;
};

/// Connection and signing settings for @ref JsonClient.
struct ClientSettings {
    /// JSON API URL. Required.
    std::string endpoint;

    /// Signing region. Required.
    std::string region;

    /// HTTP timeout for a whole request, including long polling on ReceiveMessage.
    std::chrono::milliseconds timeout{std::chrono::seconds{30}};

    /// Whether the userver HTTP client verifies TLS certificates.
    bool verify_ssl{true};
};

namespace impl {

struct JsonResponse {
    bool ok{false};
    int status_code{0};
    std::string body;
    Aws::Utils::Json::JsonValue json;
    std::string transport_error;
};

}  // namespace impl

/// @ingroup userver_clients
///
/// @brief SQS JSON protocol client.
///
/// Talks to an Amazon SQS-compatible endpoint over the AWS JSON protocol.
/// Request and result types are AWS SDK SQS models (`Aws::SQS::Model::*`).
/// HTTP is performed by @ref clients::http::Client; requests are signed with
/// SigV4 via userver crypto. `Aws::InitAPI` is not required.
///
/// Call methods from a userver coroutine: they suspend the current
/// @ref engine::Task on HTTP I/O. `clients::http::Client` must outlive this
/// client.
///
/// ## Example usage:
/// @snippet json_client_test.cpp  Sample SQS JsonClient
///
/// @see @ref scripts/docs/en/userver/libraries/sqs.md
class JsonClient {
public:
    /// @brief Constructs a client.
    ///
    /// @param http_client HTTP client used for POST requests. Must outlive `*this`.
    /// @param credentials Access keys for SigV4. Signing is skipped when both
    ///        `access_key_id` and `secret_key` are empty, or when
    ///        @ref ExtraCredentials::authorization_override is set.
    /// @param settings Endpoint, region, timeout and TLS verification.
    /// @param extra_credentials Optional IAM token or prebuilt Authorization.
    JsonClient(
        clients::http::Client& http_client,
        Credentials credentials,
        ClientSettings settings,
        ExtraCredentials extra_credentials = {}
    );

    JsonClient(const JsonClient&) = delete;
    JsonClient& operator=(const JsonClient&) = delete;
    JsonClient(JsonClient&&) noexcept;
    JsonClient& operator=(JsonClient&&) noexcept;

    /// @brief Destroys the client. Does not destroy @a http_client.
    ~JsonClient();

    /// @brief Adds a permission to a queue policy.
    Aws::SQS::Model::AddPermissionOutcome AddPermission(const Aws::SQS::Model::AddPermissionRequest& request) const;

    /// @brief Changes the visibility timeout of a received message.
    Aws::SQS::Model::ChangeMessageVisibilityOutcome ChangeMessageVisibility(
        const Aws::SQS::Model::ChangeMessageVisibilityRequest& request
    ) const;

    /// @brief Changes visibility timeout for a batch of received messages.
    Aws::SQS::Model::ChangeMessageVisibilityBatchOutcome ChangeMessageVisibilityBatch(
        const Aws::SQS::Model::ChangeMessageVisibilityBatchRequest& request
    ) const;

    /// @brief Creates a queue and returns its URL.
    Aws::SQS::Model::CreateQueueOutcome CreateQueue(const Aws::SQS::Model::CreateQueueRequest& request) const;

    /// @brief Deletes a message from a queue by receipt handle.
    Aws::SQS::Model::DeleteMessageOutcome DeleteMessage(const Aws::SQS::Model::DeleteMessageRequest& request) const;

    /// @brief Deletes a batch of messages from a queue.
    Aws::SQS::Model::DeleteMessageBatchOutcome DeleteMessageBatch(
        const Aws::SQS::Model::DeleteMessageBatchRequest& request
    ) const;

    /// @brief Deletes a queue.
    Aws::SQS::Model::DeleteQueueOutcome DeleteQueue(const Aws::SQS::Model::DeleteQueueRequest& request) const;

    /// @brief Returns attributes of a queue.
    Aws::SQS::Model::GetQueueAttributesOutcome GetQueueAttributes(
        const Aws::SQS::Model::GetQueueAttributesRequest& request
    ) const;

    /// @brief Resolves a queue name to a queue URL.
    Aws::SQS::Model::GetQueueUrlOutcome GetQueueUrl(const Aws::SQS::Model::GetQueueUrlRequest& request) const;

    /// @brief Lists queues that have the given queue as a dead-letter target.
    Aws::SQS::Model::ListDeadLetterSourceQueuesOutcome ListDeadLetterSourceQueues(
        const Aws::SQS::Model::ListDeadLetterSourceQueuesRequest& request
    ) const;

    /// @brief Lists tags of a queue.
    Aws::SQS::Model::ListQueueTagsOutcome ListQueueTags(const Aws::SQS::Model::ListQueueTagsRequest& request) const;

    /// @brief Lists queues in the account, optionally filtered by prefix.
    Aws::SQS::Model::ListQueuesOutcome ListQueues(const Aws::SQS::Model::ListQueuesRequest& request) const;

    /// @brief Deletes all messages from a queue.
    Aws::SQS::Model::PurgeQueueOutcome PurgeQueue(const Aws::SQS::Model::PurgeQueueRequest& request) const;

    /// @brief Receives messages from a queue. May long-poll up to `WaitTimeSeconds`.
    Aws::SQS::Model::ReceiveMessageOutcome ReceiveMessage(const Aws::SQS::Model::ReceiveMessageRequest& request) const;

    /// @brief Removes a permission from a queue policy.
    Aws::SQS::Model::RemovePermissionOutcome RemovePermission(const Aws::SQS::Model::RemovePermissionRequest& request
    ) const;

    /// @brief Sends a message to a queue.
    Aws::SQS::Model::SendMessageOutcome SendMessage(const Aws::SQS::Model::SendMessageRequest& request) const;

    /// @brief Sends a batch of messages to a queue.
    Aws::SQS::Model::SendMessageBatchOutcome SendMessageBatch(const Aws::SQS::Model::SendMessageBatchRequest& request
    ) const;

    /// @brief Sets attributes of a queue.
    Aws::SQS::Model::SetQueueAttributesOutcome SetQueueAttributes(
        const Aws::SQS::Model::SetQueueAttributesRequest& request
    ) const;

    /// @brief Adds or updates tags on a queue.
    Aws::SQS::Model::TagQueueOutcome TagQueue(const Aws::SQS::Model::TagQueueRequest& request) const;

    /// @brief Removes tags from a queue.
    Aws::SQS::Model::UntagQueueOutcome UntagQueue(const Aws::SQS::Model::UntagQueueRequest& request) const;

private:
    class Impl;

    impl::JsonResponse ExecuteJsonOperation(
        const char* operation_name,
        const Aws::String& endpoint,
        const Aws::Http::HeaderValueCollection& custom_headers,
        const Aws::Utils::Json::JsonValue& json_body
    ) const;

    std::unique_ptr<Impl> impl_;
};

}  // namespace sqs

USERVER_NAMESPACE_END
