#include <sqs/json_client_fixture_test.hpp>

#include <userver/utest/http_client.hpp>

#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/DeleteMessageBatchRequest.h>
#include <aws/sqs/model/DeleteMessageBatchRequestEntry.h>
#include <aws/sqs/model/GetQueueUrlRequest.h>
#include <aws/sqs/model/MessageAttributeValue.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/SendMessageBatchRequest.h>
#include <aws/sqs/model/SendMessageBatchRequestEntry.h>
#include <aws/sqs/model/SendMessageRequest.h>

#include <chrono>
#include <stdexcept>
#include <utility>

USERVER_NAMESPACE_BEGIN

namespace {

namespace sqs_model = Aws::SQS::Model;

void SampleSqsJsonClient(clients::http::Client& http_client) {
    /// [Sample SQS JsonClient]
    sqs::ClientSettings settings;
    settings.endpoint = "https://sqs.example.com";
    settings.region = "us-east-1";
    settings.timeout = std::chrono::seconds{30};

    const sqs::Credentials credentials{
        .access_key_id = "access-key",
        .secret_key = "secret-key",
    };

    sqs::JsonClient client{http_client, credentials, settings};
    /// [Sample SQS JsonClient]
}

void SampleSqsExtraCredentials(clients::http::Client& http_client) {
    sqs::ClientSettings settings;
    settings.endpoint = "https://sqs.example.com";
    settings.region = "us-east-1";
    const sqs::Credentials credentials{
        .access_key_id = "access-key",
        .secret_key = "secret-key",
    };

    /// [Sample SQS ExtraCredentials]
    sqs::ExtraCredentials extra{
        .cloud_iam_token = "iam-token",
        .authorization_override = {},
    };

    sqs::JsonClient client{http_client, credentials, settings, extra};
    /// [Sample SQS ExtraCredentials]
}

void SampleSqsSendAndReceive(sqs::JsonClient& client) {
    /// [Sample SQS send and receive]
    namespace model = Aws::SQS::Model;

    model::CreateQueueRequest create_request;
    create_request.SetQueueName("orders");

    const auto create_outcome = client.CreateQueue(create_request);
    if (!create_outcome.IsSuccess()) {
        throw std::runtime_error{create_outcome.GetError().GetMessage()};
    }

    const auto queue_url = create_outcome.GetResult().GetQueueUrl();

    model::SendMessageRequest send_request;
    send_request.SetQueueUrl(queue_url);
    send_request.SetMessageBody(R"({"order_id":"42"})");

    const auto send_outcome = client.SendMessage(send_request);
    if (!send_outcome.IsSuccess()) {
        throw std::runtime_error{send_outcome.GetError().GetMessage()};
    }

    model::ReceiveMessageRequest receive_request;
    receive_request.SetQueueUrl(queue_url);
    receive_request.SetMaxNumberOfMessages(1);
    receive_request.SetWaitTimeSeconds(5);

    const auto receive_outcome = client.ReceiveMessage(receive_request);
    if (!receive_outcome.IsSuccess()) {
        throw std::runtime_error{receive_outcome.GetError().GetMessage()};
    }
    /// [Sample SQS send and receive]
}

}  // namespace

UTEST_F(SqsJsonClient, DocumentationSnippets) {
    auto http_client = utest::CreateHttpClient();
    SampleSqsJsonClient(*http_client);
    SampleSqsExtraCredentials(*http_client);

    auto client = MakeClient();
    UEXPECT_NO_THROW(SampleSqsSendAndReceive(*client));
}

UTEST_F(SqsJsonClient, RegionIsRequired) {
    auto http_client = utest::CreateHttpClient();
    sqs::ClientSettings settings;
    settings.endpoint = "https://sqs.example.com";
    EXPECT_UINVARIANT_FAILURE_MSG((sqs::JsonClient{*http_client, {}, settings}), "sqs::ClientSettings.region is empty");
}

UTEST_F(SqsJsonClient, CreateQueueAndGetQueueUrl) {
    auto client = MakeClient();
    const auto queue_url = CreateQueue(*client, "json-client-create");
    ASSERT_FALSE(queue_url.empty());

    sqs_model::GetQueueUrlRequest request;
    request.SetQueueName(queue_url.substr(queue_url.rfind('/') + 1));

    const auto outcome = client->GetQueueUrl(request);
    ASSERT_TRUE(outcome.IsSuccess()) << GetErrorMessage(outcome.GetError());
    EXPECT_EQ(outcome.GetResult().GetQueueUrl(), queue_url);
}

UTEST_F(SqsJsonClient, MoveClient) {
    auto client = MakeClient();
    sqs::JsonClient moved{std::move(*client)};
    const auto queue_url = CreateQueue(moved, "json-client-move");
    ASSERT_FALSE(queue_url.empty());

    auto other = MakeClient();
    *other = std::move(moved);

    sqs_model::GetQueueUrlRequest request;
    request.SetQueueName(queue_url.substr(queue_url.rfind('/') + 1));
    const auto outcome = other->GetQueueUrl(request);
    ASSERT_TRUE(outcome.IsSuccess()) << GetErrorMessage(outcome.GetError());
    EXPECT_EQ(outcome.GetResult().GetQueueUrl(), queue_url);
}

UTEST_F(SqsJsonClient, SendReceiveDeleteMessage) {
    auto client = MakeClient();
    const auto queue_url = CreateQueue(*client, "json-client-message");
    ASSERT_FALSE(queue_url.empty());

    const Aws::String message_body = "hello-from-json-client";

    sqs_model::SendMessageRequest send_request;
    send_request.SetQueueUrl(queue_url);
    send_request.SetMessageBody(message_body);

    const auto send_outcome = RunWhenQueueIsReady([&] { return client->SendMessage(send_request); });
    ASSERT_TRUE(send_outcome.IsSuccess()) << GetErrorMessage(send_outcome.GetError());
    ASSERT_FALSE(send_outcome.GetResult().GetMessageId().empty());

    sqs_model::ReceiveMessageRequest receive_request;
    receive_request.SetQueueUrl(queue_url);
    receive_request.SetMaxNumberOfMessages(1);
    receive_request.SetWaitTimeSeconds(5);

    const auto receive_outcome = client->ReceiveMessage(receive_request);
    ASSERT_TRUE(receive_outcome.IsSuccess()) << GetErrorMessage(receive_outcome.GetError());
    ASSERT_EQ(receive_outcome.GetResult().GetMessages().size(), 1);
    EXPECT_EQ(receive_outcome.GetResult().GetMessages()[0].GetBody(), message_body);

    sqs_model::DeleteMessageBatchRequest delete_request;
    delete_request.SetQueueUrl(queue_url);
    delete_request
        .AddEntries(sqs_model::DeleteMessageBatchRequestEntry{}
                        .WithId("msg-0")
                        .WithReceiptHandle(receive_outcome.GetResult().GetMessages()[0].GetReceiptHandle()));

    const auto delete_outcome = client->DeleteMessageBatch(delete_request);
    ASSERT_TRUE(delete_outcome.IsSuccess()) << GetErrorMessage(delete_outcome.GetError());
    EXPECT_EQ(delete_outcome.GetResult().GetSuccessful().size(), 1);
}

UTEST_F(SqsJsonClient, SendReceiveMessageAttributes) {
    auto client = MakeClient();
    const auto queue_url = CreateQueue(*client, "json-client-attrs");
    ASSERT_FALSE(queue_url.empty());

    sqs_model::SendMessageRequest send_request;
    send_request.SetQueueUrl(queue_url);
    send_request.SetMessageBody("message-with-attributes");
    send_request.AddMessageAttributes(
        "stringAttr",
        sqs_model::MessageAttributeValue{}.WithDataType("String").WithStringValue("string-value")
    );

    const unsigned char binary_value[] = {'b', 'i', 'n'};
    send_request.AddMessageAttributes(
        "binaryAttr",
        sqs_model::MessageAttributeValue{}
            .WithDataType("Binary")
            .WithBinaryValue(Aws::Utils::ByteBuffer{binary_value, sizeof(binary_value)})
    );

    const auto send_outcome = RunWhenQueueIsReady([&] { return client->SendMessage(send_request); });
    ASSERT_TRUE(send_outcome.IsSuccess()) << GetErrorMessage(send_outcome.GetError());

    sqs_model::ReceiveMessageRequest receive_request;
    receive_request.SetQueueUrl(queue_url);
    receive_request.SetMaxNumberOfMessages(1);
    receive_request.SetWaitTimeSeconds(5);
    receive_request.AddMessageAttributeNames("All");

    const auto receive_outcome = client->ReceiveMessage(receive_request);
    ASSERT_TRUE(receive_outcome.IsSuccess()) << GetErrorMessage(receive_outcome.GetError());
    ASSERT_EQ(receive_outcome.GetResult().GetMessages().size(), 1);

    const auto& attributes = receive_outcome.GetResult().GetMessages()[0].GetMessageAttributes();
    EXPECT_EQ(attributes.at("stringAttr").GetStringValue(), "string-value");
    EXPECT_EQ(attributes.at("binaryAttr").GetBinaryValue().GetLength(), sizeof(binary_value));
}

UTEST_F(SqsJsonClient, SendMessageBatch) {
    auto client = MakeClient();
    const auto queue_url = CreateQueue(*client, "json-client-batch");
    ASSERT_FALSE(queue_url.empty());

    sqs_model::SendMessageBatchRequest request;
    request.SetQueueUrl(queue_url);
    request.AddEntries(sqs_model::SendMessageBatchRequestEntry{}.WithId("0").WithMessageBody("batch-body-0"));
    request.AddEntries(sqs_model::SendMessageBatchRequestEntry{}.WithId("1").WithMessageBody("batch-body-1"));

    const auto outcome = RunWhenQueueIsReady([&] { return client->SendMessageBatch(request); });
    ASSERT_TRUE(outcome.IsSuccess()) << GetErrorMessage(outcome.GetError());
    EXPECT_EQ(outcome.GetResult().GetSuccessful().size(), 2);
    EXPECT_EQ(outcome.GetResult().GetFailed().size(), 0);
}

USERVER_NAMESPACE_END
