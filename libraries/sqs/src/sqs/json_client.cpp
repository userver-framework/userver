#include <userver/sqs/json_client.hpp>

#include <aws/core/http/HttpTypes.h>
#include <aws/core/utils/Array.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/json/JsonSerializer.h>
#include <aws/sqs/SQSErrors.h>
#include <aws/sqs/model/AddPermissionRequest.h>
#include <aws/sqs/model/ChangeMessageVisibilityBatchRequest.h>
#include <aws/sqs/model/ChangeMessageVisibilityRequest.h>
#include <aws/sqs/model/CreateQueueRequest.h>
#include <aws/sqs/model/DeleteMessageBatchRequest.h>
#include <aws/sqs/model/DeleteMessageRequest.h>
#include <aws/sqs/model/DeleteQueueRequest.h>
#include <aws/sqs/model/GetQueueAttributesRequest.h>
#include <aws/sqs/model/GetQueueUrlRequest.h>
#include <aws/sqs/model/ListDeadLetterSourceQueuesRequest.h>
#include <aws/sqs/model/ListQueueTagsRequest.h>
#include <aws/sqs/model/ListQueuesRequest.h>
#include <aws/sqs/model/MessageSystemAttributeNameForSends.h>
#include <aws/sqs/model/PurgeQueueRequest.h>
#include <aws/sqs/model/ReceiveMessageRequest.h>
#include <aws/sqs/model/RemovePermissionRequest.h>
#include <aws/sqs/model/SendMessageBatchRequest.h>
#include <aws/sqs/model/SendMessageRequest.h>
#include <aws/sqs/model/SetQueueAttributesRequest.h>
#include <aws/sqs/model/TagQueueRequest.h>
#include <aws/sqs/model/UntagQueueRequest.h>

#include <userver/clients/http/client.hpp>
#include <userver/clients/http/request.hpp>
#include <userver/clients/http/response.hpp>
#include <userver/crypto/aws.hpp>
#include <userver/crypto/base64.hpp>
#include <userver/http/common_headers.hpp>
#include <userver/http/predefined_header.hpp>
#include <userver/http/url.hpp>
#include <userver/logging/log.hpp>
#include <userver/logging/log_extra.hpp>
#include <userver/tracing/tags.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/string_literal.hpp>
#include <userver/utils/text.hpp>
#include <userver/utils/uuid4.hpp>
#include <userver/utils/zstring_view.hpp>

#include <algorithm>
#include <exception>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

USERVER_NAMESPACE_BEGIN

namespace sqs {

// NOLINTNEXTLINE(google-build-using-namespace)
using namespace Aws::SQS::Model;

namespace {

constexpr auto kContentTypeValue = "application/x-amz-json-1.0";
constexpr auto kAmzSdkRequestValue = "attempt=1";
constexpr auto kXAmzAPIVersionValue = "2012-11-05";
constexpr auto kAuthorizationOverrideDate = "20150830T123600Z";
constexpr auto kServiceName = "sqs";

constexpr http::headers::PredefinedHeader kAmzSdkRequestHeader{"amz-sdk-request"};
constexpr http::headers::PredefinedHeader kXAmzAPIVersionHeader{"x-amz-api-version"};
constexpr http::headers::PredefinedHeader kXYaCloudSubjectTokenHeader{"X-YaCloud-SubjectToken"};
constexpr http::headers::PredefinedHeader kAmzSdkInvocationIdHeader{"amz-sdk-invocation-id"};
constexpr http::headers::PredefinedHeader kXAmzDateHeader{"x-amz-date"};
constexpr http::headers::PredefinedHeader kXAmzSecurityTokenHeader{"x-amz-security-token"};
constexpr http::headers::PredefinedHeader kXAmzTargetHeader{"x-amz-target"};
constexpr http::headers::PredefinedHeader kXAmznQueryModeHeader{"x-amzn-query-mode"};

constexpr utils::StringLiteral kTracingTypeRequest = "request";
constexpr utils::StringLiteral kTracingBody = "body";
constexpr utils::StringLiteral kTracingUri = "uri";
constexpr utils::StringLiteral kTracingRequestBodyLength = "request_body_length";
constexpr utils::StringLiteral kHttpMethodPost = "POST";
constexpr std::string_view kMaskedHeaderValue = "***";

bool IsSensitiveHeader(std::string_view header_name) {
    const auto lower = utils::text::ToLower(header_name);
    return lower == "authorization" || lower == "x-yacloud-subjecttoken" || lower == "x-amz-security-token";
}

std::string FormatRequestHeaders(const clients::http::Headers& headers) {
    std::vector<std::pair<std::string_view, std::string_view>> sorted_headers;
    sorted_headers.reserve(headers.size());
    for (const auto& [name, value] : headers) {
        sorted_headers.emplace_back(name, value);
    }
    std::ranges::sort(sorted_headers, [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });

    std::string result;
    for (const auto& [name, value] : sorted_headers) {
        result.append(name);
        result.append(": ");
        result.append(IsSensitiveHeader(name) ? kMaskedHeaderValue : value);
        result.push_back('\n');
    }
    return result;
}

void LogRequest(
    const char* operation_name,
    std::string_view url,
    const clients::http::Headers& headers,
    std::string_view body
) {
    LOG_INFO() << [&](auto& logger) {
        logging::LogExtra log_extra{
            {tracing::kHttpMetaType, operation_name},
            {tracing::kType, kTracingTypeRequest},
            {kTracingRequestBodyLength, static_cast<unsigned long long>(body.size())},
            {kTracingBody, body},
            {kTracingUri, url},
            {tracing::kHttpMethod, kHttpMethodPost},
        };
        log_extra.Extend("request_headers", FormatRequestHeaders(headers));
        logger.Format("start SQS {} {}", kHttpMethodPost, operation_name) << log_extra;
    };
}

// SigV4 signs the Host header including the port when the URL specifies one.
// ExtractHostnameView returns host only, so the port is taken from the URL
// immediately after that hostname substring.
std::string MakeHostHeaderValue(std::string_view url) {
    const auto host = http::ExtractHostnameView(url);
    const auto after_host = static_cast<std::size_t>(host.data() + host.size() - url.data());
    if (after_host >= url.size() || url[after_host] != ':') {
        return std::string{host};
    }

    auto port = url.substr(after_host);
    const auto port_end = port.find_first_of("/?#");
    if (port_end != std::string_view::npos) {
        port = port.substr(0, port_end);
    }

    std::string result;
    result.reserve(host.size() + port.size());
    result.append(host);
    result.append(port);
    return result;
}

std::string CanonicalUri(std::string_view url) {
    auto path = http::ExtractPathView(url);
    if (path.empty()) {
        path = "/";
    }
    return http::EncodeS3Key(path);
}

std::string EncodeBinary(const Aws::Utils::ByteBuffer& buffer) {
    return crypto::base64::Base64Encode(std::string_view{
        reinterpret_cast<const char*>(buffer.GetUnderlyingData()),
        buffer.GetLength()
    });
}

Aws::Utils::ByteBuffer DecodeBinary(const Aws::String& value) {
    const auto decoded = crypto::base64::Base64Decode(std::string_view{value.c_str(), value.size()});
    return Aws::Utils::ByteBuffer{reinterpret_cast<const unsigned char*>(decoded.data()), decoded.size()};
}

template <typename T>
Aws::Utils::Json::JsonValue BuildAttributeValueJson(const T& attr_value) {
    Aws::Utils::Json::JsonValue attr_value_json;
    attr_value_json.WithString("DataType", attr_value.GetDataType());

    if (attr_value.StringValueHasBeenSet()) {
        attr_value_json.WithString("StringValue", attr_value.GetStringValue());
    }

    if (attr_value.StringListValuesHasBeenSet()) {
        Aws::Vector<Aws::Utils::Json::JsonValue> string_list_values_vector;
        string_list_values_vector.reserve(attr_value.GetStringListValues().size());
        for (const auto& string_value : attr_value.GetStringListValues()) {
            Aws::Utils::Json::JsonValue string_value_json;
            string_value_json.AsString(string_value);
            string_list_values_vector.push_back(string_value_json);
        }
        Aws::Utils::Array<Aws::Utils::Json::JsonValue> string_list_values_array(string_list_values_vector.size());
        for (size_t i = 0; i < string_list_values_vector.size(); ++i) {
            string_list_values_array[i] = string_list_values_vector[i];
        }
        attr_value_json.WithArray("StringListValues", string_list_values_array);
    }

    if (attr_value.BinaryValueHasBeenSet()) {
        const auto& binary_value = attr_value.GetBinaryValue();
        attr_value_json.WithString("BinaryValue", Aws::String{EncodeBinary(binary_value)});
    }

    if (attr_value.BinaryListValuesHasBeenSet()) {
        Aws::Vector<Aws::Utils::Json::JsonValue> binary_list_values_vector;
        binary_list_values_vector.reserve(attr_value.GetBinaryListValues().size());
        for (const auto& binary_value : attr_value.GetBinaryListValues()) {
            const auto encoded = EncodeBinary(binary_value);
            Aws::Utils::Json::JsonValue binary_value_json;
            binary_value_json.AsString(Aws::String{encoded});
            binary_list_values_vector.push_back(binary_value_json);
        }
        Aws::Utils::Array<Aws::Utils::Json::JsonValue> binary_list_values_array(binary_list_values_vector.size());
        for (size_t i = 0; i < binary_list_values_vector.size(); ++i) {
            binary_list_values_array[i] = binary_list_values_vector[i];
        }
        attr_value_json.WithArray("BinaryListValues", binary_list_values_array);
    }

    return attr_value_json;
}

Aws::Utils::Json::JsonValue BuildMessageAttributesJson(
    const Aws::Map<Aws::String, MessageAttributeValue>& message_attributes
) {
    Aws::Utils::Json::JsonValue message_attributes_json;
    for (const auto& attr_pair : message_attributes) {
        message_attributes_json.WithObject(attr_pair.first, BuildAttributeValueJson(attr_pair.second));
    }
    return message_attributes_json;
}

Aws::Utils::Json::JsonValue BuildMessageSystemAttributesJson(
    const Aws::Map<MessageSystemAttributeNameForSends, MessageSystemAttributeValue>& message_system_attributes
) {
    Aws::Utils::Json::JsonValue message_system_attributes_json;
    for (const auto& attr_pair : message_system_attributes) {
        Aws::String attr_name =
            MessageSystemAttributeNameForSendsMapper::GetNameForMessageSystemAttributeNameForSends(attr_pair.first);
        message_system_attributes_json.WithObject(attr_name, BuildAttributeValueJson(attr_pair.second));
    }
    return message_system_attributes_json;
}

Aws::Utils::Json::JsonValue BuildQueueAttributesJson(const Aws::Map<QueueAttributeName, Aws::String>& attributes) {
    Aws::Utils::Json::JsonValue attributes_json;
    for (const auto& attr_pair : attributes) {
        attributes_json
            .WithString(QueueAttributeNameMapper::GetNameForQueueAttributeName(attr_pair.first), attr_pair.second);
    }
    return attributes_json;
}

Aws::Utils::Json::JsonValue BuildTagsJson(const Aws::Map<Aws::String, Aws::String>& tags) {
    Aws::Utils::Json::JsonValue tags_json;
    for (const auto& tag_pair : tags) {
        tags_json.WithString(tag_pair.first, tag_pair.second);
    }
    return tags_json;
}

template <typename Container>
Aws::Utils::Array<Aws::Utils::Json::JsonValue> BuildStringArrayJson(const Container& strings) {
    Aws::Utils::Array<Aws::Utils::Json::JsonValue> array(strings.size());
    for (size_t i = 0; i < strings.size(); ++i) {
        array[i].AsString(strings[i]);
    }
    return array;
}

Aws::String JsonViewToString(const Aws::Utils::Json::JsonView& value_view) {
    if (value_view.IsString()) {
        return value_view.AsString();
    }
    if (value_view.IsIntegerType()) {
        return Aws::Utils::StringUtils::to_string(value_view.AsInteger());
    }
    if (value_view.IsFloatingPointType()) {
        return Aws::Utils::StringUtils::to_string(value_view.AsDouble());
    }
    if (value_view.IsBool()) {
        return value_view.AsBool() ? "true" : "false";
    }
    return {};
}

bool JsonViewIsScalar(const Aws::Utils::Json::JsonView& value_view) {
    return value_view.IsString() || value_view.IsIntegerType() || value_view.IsFloatingPointType() ||
           value_view.IsBool();
}

Aws::String ExtractErrorMessage(const Aws::Utils::Json::JsonView& error_view) {
    Aws::Vector<Aws::String> parts;

    if (error_view.KeyExists("Code")) {
        parts.push_back(error_view.GetString("Code"));
    }
    if (error_view.KeyExists("__type")) {
        parts.push_back(error_view.GetString("__type"));
    }
    if (error_view.KeyExists("Message")) {
        parts.push_back(error_view.GetString("Message"));
    }
    if (error_view.KeyExists("message")) {
        parts.push_back(error_view.GetString("message"));
    }

    if (parts.empty()) {
        return {};
    }

    Aws::String result = parts.front();
    for (size_t i = 1; i < parts.size(); ++i) {
        result += ": ";
        result += parts[i];
    }
    return result;
}

Aws::String ExtractErrorMessageFromBody(utils::zstring_view body_string) {
    if (body_string.empty()) {
        return {};
    }

    Aws::Utils::Json::JsonValue response_json(Aws::String{body_string.c_str(), body_string.size()});
    if (!response_json.WasParseSuccessful()) {
        return Aws::String{body_string.c_str(), body_string.size()};
    }

    auto response_view = response_json.View();
    if (response_view.KeyExists("Error") && response_view.GetObject("Error").IsObject()) {
        const auto error_message = ExtractErrorMessage(response_view.GetObject("Error"));
        if (!error_message.empty()) {
            return error_message;
        }
    }

    return ExtractErrorMessage(response_view);
}

Aws::Map<QueueAttributeName, Aws::String> ParseQueueAttributesJson(const Aws::Utils::Json::JsonView& attributes_view) {
    Aws::Map<QueueAttributeName, Aws::String> attributes;
    for (const auto& [attribute_name, attribute_value] : attributes_view.GetAllObjects()) {
        if (JsonViewIsScalar(attribute_value)) {
            attributes[QueueAttributeNameMapper::GetQueueAttributeNameForName(attribute_name
            )] = JsonViewToString(attribute_value);
        }
    }
    return attributes;
}

Aws::Map<Aws::String, Aws::String> ParseTagsJson(const Aws::Utils::Json::JsonView& tags_view) {
    Aws::Map<Aws::String, Aws::String> tags;
    for (const auto& [tag_key, tag_value] : tags_view.GetAllObjects()) {
        if (tag_value.IsString()) {
            tags[tag_key] = tag_value.AsString();
        }
    }
    return tags;
}

Message ParseMessageJson(const Aws::Utils::Json::JsonView& message_view) {
    Message message;
    message.WithBody(message_view.GetString("Body"))
        .WithMessageId(message_view.GetString("MessageId"))
        .WithReceiptHandle(message_view.GetString("ReceiptHandle"))
        .WithMD5OfBody(message_view.GetString("MD5OfBody"));

    if (message_view.KeyExists("MD5OfMessageAttributes")) {
        message.WithMD5OfMessageAttributes(message_view.GetString("MD5OfMessageAttributes"));
    }

    if (message_view.KeyExists("Attributes")) {
        const auto& message_attributes = message_view.GetObject("Attributes");
        Aws::Map<MessageSystemAttributeName, Aws::String> message_system_attributes_map;
        for (const auto& [attribute_name, attribute_value] : message_attributes.GetAllObjects()) {
            auto message_attribute =
                MessageSystemAttributeNameMapper::GetMessageSystemAttributeNameForName(attribute_name);
            if (JsonViewIsScalar(attribute_value)) {
                message_system_attributes_map[message_attribute] = JsonViewToString(attribute_value);
            }
        }
        message.WithAttributes(message_system_attributes_map);
    }

    if (message_view.KeyExists("MessageAttributes")) {
        const auto& message_attributes = message_view.GetObject("MessageAttributes");
        Aws::Map<Aws::String, MessageAttributeValue> message_attributes_map;
        for (const auto& [attribute_name, attribute_value] : message_attributes.GetAllObjects()) {
            if (attribute_value.IsObject()) {
                const auto attr_object = attribute_value.AsObject();
                MessageAttributeValue attr;
                if (attr_object.KeyExists("DataType")) {
                    attr.WithDataType(attr_object.GetString("DataType"));
                }
                if (attr_object.KeyExists("StringValue")) {
                    attr.WithStringValue(attr_object.GetString("StringValue"));
                }
                if (attr_object.KeyExists("BinaryValue")) {
                    attr.WithBinaryValue(DecodeBinary(attr_object.GetString("BinaryValue")));
                }
                if (attr_object.KeyExists("StringListValues")) {
                    const auto string_list_values = attr_object.GetArray("StringListValues");
                    for (size_t i = 0; i < string_list_values.GetLength(); ++i) {
                        attr.AddStringListValues(string_list_values[i].AsString());
                    }
                }
                if (attr_object.KeyExists("BinaryListValues")) {
                    const auto binary_list_values = attr_object.GetArray("BinaryListValues");
                    for (size_t i = 0; i < binary_list_values.GetLength(); ++i) {
                        attr.AddBinaryListValues(DecodeBinary(binary_list_values[i].AsString()));
                    }
                }
                message_attributes_map[attribute_name] = attr;
            }
        }
        message.WithMessageAttributes(message_attributes_map);
    }

    return message;
}

template <typename Outcome>
Outcome MakeHttpErrorOutcome(int status_code, utils::zstring_view body, Aws::String message = {}) {
    Aws::SQS::SQSError error;
    error.SetResponseCode(static_cast<Aws::Http::HttpResponseCode>(status_code));
    if (message.empty()) {
        message = ExtractErrorMessageFromBody(body);
    }
    if (!message.empty()) {
        error.SetMessage(message);
    }
    return Outcome(error);
}

template <typename Outcome>
Outcome MakeJsonParseErrorOutcome(const impl::JsonResponse& response) {
    return MakeHttpErrorOutcome<Outcome>(response.status_code, response.body, response.json.GetErrorMessage());
}

template <typename Outcome>
Outcome MakeFailedOutcome(const impl::JsonResponse& response) {
    if (!response.transport_error.empty()) {
        return MakeHttpErrorOutcome<Outcome>(
            response.status_code,
            response.body,
            Aws::String{response.transport_error.c_str(), response.transport_error.size()}
        );
    }
    if (response.status_code != 200) {
        return MakeHttpErrorOutcome<Outcome>(response.status_code, response.body);
    }
    return MakeJsonParseErrorOutcome<Outcome>(response);
}

}  // namespace

class JsonClient::Impl {
public:
    Impl(
        clients::http::Client& http_client,
        Credentials credentials,
        ClientSettings settings,
        ExtraCredentials extra_credentials
    )
        : http_client(http_client),
          settings(std::move(settings)),
          extra_credentials(std::move(extra_credentials)),
          access_key(std::move(credentials.access_key_id)),
          secret_key(std::move(credentials.secret_key)),
          session_token(std::move(credentials.session_token)),
          has_signing_credentials(!access_key.empty() || !secret_key.empty())
    {
        UINVARIANT(!this->settings.endpoint.empty(), "sqs::ClientSettings.endpoint is empty; set the SQS JSON API URL");
        UINVARIANT(
            this->settings.timeout > std::chrono::milliseconds{0},
            "sqs::ClientSettings.timeout must be positive"
        );
        UINVARIANT(
            !this->settings.region.empty(),
            "sqs::ClientSettings.region is empty; set the AWS signing region "
            "(for example, ru-central1 or us-east-1)"
        );
    }

    impl::JsonResponse ExecuteJsonOperation(
        const char* operation_name,
        const Aws::Http::HeaderValueCollection& custom_headers,
        const Aws::Utils::Json::JsonValue& json_body
    ) const;

    clients::http::Client& http_client;
    ClientSettings settings;
    ExtraCredentials extra_credentials;
    std::string access_key;
    std::string secret_key;
    std::string session_token;
    bool has_signing_credentials;
};

JsonClient::JsonClient(
    clients::http::Client& http_client,
    Credentials credentials,
    ClientSettings settings,
    ExtraCredentials extra_credentials
)
    : impl_(std::make_unique<
            Impl>(http_client, std::move(credentials), std::move(settings), std::move(extra_credentials)))
{}

JsonClient::JsonClient(JsonClient&&) noexcept = default;

JsonClient& JsonClient::operator=(JsonClient&&) noexcept = default;

JsonClient::~JsonClient() = default;

impl::JsonResponse JsonClient::ExecuteJsonOperation(
    const char* operation_name,
    const Aws::String& /*endpoint*/,
    const Aws::Http::HeaderValueCollection& custom_headers,
    const Aws::Utils::Json::JsonValue& json_body
) const {
    return impl_->ExecuteJsonOperation(operation_name, custom_headers, json_body);
}

impl::JsonResponse JsonClient::Impl::ExecuteJsonOperation(
    const char* operation_name,
    const Aws::Http::HeaderValueCollection& custom_headers,
    const Aws::Utils::Json::JsonValue& json_body
) const {
    impl::JsonResponse result;
    const auto compact_json = json_body.View().WriteCompact();
    const std::string body{compact_json.c_str(), compact_json.size()};

    clients::http::Headers headers;
    for (const auto& header : custom_headers) {
        headers.insert_or_assign(std::string{header.first}, std::string{header.second});
    }
    headers.insert_or_assign(http::headers::kContentType, kContentTypeValue);
    headers.insert_or_assign(kAmzSdkRequestHeader, kAmzSdkRequestValue);
    headers.insert_or_assign(kXAmzAPIVersionHeader, kXAmzAPIVersionValue);
    headers.insert_or_assign(kXAmznQueryModeHeader, "true");
    headers.insert_or_assign(kXAmzTargetHeader, std::string{"AmazonSQS."} + operation_name);
    headers.insert_or_assign(http::headers::kHost, MakeHostHeaderValue(settings.endpoint));
    headers.insert_or_assign(kAmzSdkInvocationIdHeader, utils::generators::GenerateUuid());
    if (!extra_credentials.cloud_iam_token.empty()) {
        headers.insert_or_assign(kXYaCloudSubjectTokenHeader, std::string{extra_credentials.cloud_iam_token});
    }
    if (!session_token.empty()) {
        headers.insert_or_assign(kXAmzSecurityTokenHeader, session_token);
    }

    if (!extra_credentials.authorization_override.empty()) {
        // authorization_override is a prebuilt Authorization header; this date is only
        // required to keep the SigV4-shaped header set complete.
        headers.insert_or_assign(http::headers::kAuthorization, std::string{extra_credentials.authorization_override});
        headers.insert_or_assign(kXAmzDateHeader, kAuthorizationOverrideDate);
    } else if (has_signing_credentials) {
        const auto canonical_uri = CanonicalUri(settings.endpoint);
        crypto::aws::SignRequestV4(
            headers,
            {
                .http_method = kHttpMethodPost,
                .canonical_uri = canonical_uri,
                .canonical_query = http::ExtractQueryView(settings.endpoint),
                .payload = body,
                .access_key = access_key,
                .secret_key = secret_key,
                .region = settings.region,
                .service = kServiceName,
            }
        );
    }

    LogRequest(operation_name, settings.endpoint, headers, body);

    try {
        const auto response =
            http_client.CreateNotSignedRequest()
                .post(settings.endpoint, body)
                .headers(headers)
                .timeout(settings.timeout)
                .retry(1)
                .verify(settings.verify_ssl)
                .perform();

        result.status_code = static_cast<int>(response->status_code());
        result.body = response->body();
        if (result.status_code != 200) {
            return result;
        }

        result.json = Aws::Utils::Json::JsonValue(Aws::String{result.body.c_str(), result.body.size()});
        if (!result.json.WasParseSuccessful()) {
            LOG_ERROR()
                << "Failed to parse JSON: " << result.json.GetErrorMessage() << ". Response body was: " << result.body;
            return result;
        }

        result.ok = true;
        return result;
    } catch (const std::exception& ex) {
        result.transport_error = ex.what();
        LOG_ERROR() << operation_name << " HTTP request failed: " << result.transport_error;
        return result;
    }
}

AddPermissionOutcome JsonClient::AddPermission(const AddPermissionRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl())
        .WithString("Label", request.GetLabel())
        .WithArray("AWSAccountIds", BuildStringArrayJson(request.GetAWSAccountIds()))
        .WithArray("Actions", BuildStringArrayJson(request.GetActions()));

    const auto response = ExecuteJsonOperation(
        "AddPermission",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<AddPermissionOutcome>(response);
    }

    return AddPermissionOutcome(Aws::NoResult());
}

ChangeMessageVisibilityOutcome JsonClient::ChangeMessageVisibility(const ChangeMessageVisibilityRequest& request
) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl())
        .WithString("ReceiptHandle", request.GetReceiptHandle())
        .WithInteger("VisibilityTimeout", request.GetVisibilityTimeout());

    const auto response = ExecuteJsonOperation(
        "ChangeMessageVisibility",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<ChangeMessageVisibilityOutcome>(response);
    }

    return ChangeMessageVisibilityOutcome(Aws::NoResult());
}

ChangeMessageVisibilityBatchOutcome JsonClient::ChangeMessageVisibilityBatch(
    const ChangeMessageVisibilityBatchRequest& request
) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    Aws::Utils::Array<Aws::Utils::Json::JsonValue> entries_array(request.GetEntries().size());
    for (size_t i = 0; i < request.GetEntries().size(); ++i) {
        const auto& entry = request.GetEntries()[i];
        entries_array[i] =
            Aws::Utils::Json::JsonValue()
                .WithString("Id", entry.GetId())
                .WithString("ReceiptHandle", entry.GetReceiptHandle())
                .WithInteger("VisibilityTimeout", entry.GetVisibilityTimeout());
    }
    json_request.WithArray("Entries", entries_array);

    const auto response = ExecuteJsonOperation(
        "ChangeMessageVisibilityBatch",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<ChangeMessageVisibilityBatchOutcome>(response);
    }

    ChangeMessageVisibilityBatchResult result;
    if (response.json.View().KeyExists("Successful")) {
        const auto& successful = response.json.View().GetArray("Successful");
        for (size_t i = 0; i < successful.GetLength(); ++i) {
            result.AddSuccessful(ChangeMessageVisibilityBatchResultEntry().WithId(successful[i].GetString("Id")));
        }
    }
    if (response.json.View().KeyExists("Failed")) {
        const auto& failed = response.json.View().GetArray("Failed");
        for (size_t i = 0; i < failed.GetLength(); ++i) {
            result
                .AddFailed(BatchResultErrorEntry()
                               .WithId(failed[i].GetString("Id"))
                               .WithSenderFault(failed[i].GetBool("SenderFault"))
                               .WithCode(failed[i].GetString("Code"))
                               .WithMessage(failed[i].GetString("Message")));
        }
    }

    return ChangeMessageVisibilityBatchOutcome(result);
}

CreateQueueOutcome JsonClient::CreateQueue(const CreateQueueRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueName", request.GetQueueName());

    if (request.AttributesHasBeenSet()) {
        json_request.WithObject("Attributes", BuildQueueAttributesJson(request.GetAttributes()));
    }
    if (request.TagsHasBeenSet()) {
        json_request.WithObject("Tags", BuildTagsJson(request.GetTags()));
    }

    const auto response = ExecuteJsonOperation(
        "CreateQueue",
        Aws::String{impl_->settings.endpoint},
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<CreateQueueOutcome>(response);
    }

    CreateQueueResult result;
    if (response.json.View().KeyExists("QueueUrl")) {
        result.SetQueueUrl(response.json.View().GetString("QueueUrl"));
    }

    return CreateQueueOutcome(result);
}

DeleteMessageOutcome JsonClient::DeleteMessage(const DeleteMessageRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl()).WithString("ReceiptHandle", request.GetReceiptHandle());

    const auto response = ExecuteJsonOperation(
        "DeleteMessage",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<DeleteMessageOutcome>(response);
    }

    return DeleteMessageOutcome(Aws::NoResult());
}

DeleteMessageBatchOutcome JsonClient::DeleteMessageBatch(const DeleteMessageBatchRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    Aws::Utils::Array<Aws::Utils::Json::JsonValue> entries_array(request.GetEntries().size());
    for (size_t i = 0; i < request.GetEntries().size(); ++i) {
        const auto& entry = request.GetEntries()[i];
        entries_array[i] =
            Aws::Utils::Json::JsonValue()
                .WithString("Id", entry.GetId())
                .WithString("ReceiptHandle", entry.GetReceiptHandle());
    }
    json_request.WithArray("Entries", entries_array);

    const auto response = ExecuteJsonOperation(
        "DeleteMessageBatch",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<DeleteMessageBatchOutcome>(response);
    }

    DeleteMessageBatchResult result;
    if (response.json.View().KeyExists("Successful")) {
        const auto& successful = response.json.View().GetArray("Successful");
        for (size_t i = 0; i < successful.GetLength(); ++i) {
            result.AddSuccessful(DeleteMessageBatchResultEntry().WithId(successful[i].GetString("Id")));
        }
    }
    if (response.json.View().KeyExists("Failed")) {
        const auto& failed = response.json.View().GetArray("Failed");
        for (size_t i = 0; i < failed.GetLength(); ++i) {
            result
                .AddFailed(BatchResultErrorEntry()
                               .WithId(failed[i].GetString("Id"))
                               .WithSenderFault(failed[i].GetBool("SenderFault"))
                               .WithCode(failed[i].GetString("Code"))
                               .WithMessage(failed[i].GetString("Message")));
        }
    }

    return DeleteMessageBatchOutcome(result);
}

DeleteQueueOutcome JsonClient::DeleteQueue(const DeleteQueueRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    const auto response =
        ExecuteJsonOperation("DeleteQueue", request.GetQueueUrl(), request.GetAdditionalCustomHeaders(), json_request);
    if (!response.ok) {
        return MakeFailedOutcome<DeleteQueueOutcome>(response);
    }

    return DeleteQueueOutcome(Aws::NoResult());
}

GetQueueAttributesOutcome JsonClient::GetQueueAttributes(const GetQueueAttributesRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    if (!request.GetAttributeNames().empty()) {
        Aws::Vector<Aws::String> attribute_names;
        attribute_names.reserve(request.GetAttributeNames().size());
        for (const auto& attribute_name : request.GetAttributeNames()) {
            attribute_names.push_back(QueueAttributeNameMapper::GetNameForQueueAttributeName(attribute_name));
        }
        json_request.WithArray("AttributeNames", BuildStringArrayJson(attribute_names));
    }

    const auto response = ExecuteJsonOperation(
        "GetQueueAttributes",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<GetQueueAttributesOutcome>(response);
    }

    GetQueueAttributesResult result;
    if (response.json.View().KeyExists("Attributes")) {
        result.SetAttributes(ParseQueueAttributesJson(response.json.View().GetObject("Attributes")));
    }

    return GetQueueAttributesOutcome(result);
}

GetQueueUrlOutcome JsonClient::GetQueueUrl(const GetQueueUrlRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueName", request.GetQueueName());

    if (request.QueueOwnerAWSAccountIdHasBeenSet()) {
        json_request.WithString("QueueOwnerAWSAccountId", request.GetQueueOwnerAWSAccountId());
    }

    const auto response = ExecuteJsonOperation(
        "GetQueueUrl",
        Aws::String{impl_->settings.endpoint},
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<GetQueueUrlOutcome>(response);
    }

    GetQueueUrlResult result;
    if (response.json.View().KeyExists("QueueUrl")) {
        result.SetQueueUrl(response.json.View().GetString("QueueUrl"));
    }

    return GetQueueUrlOutcome(result);
}

ListDeadLetterSourceQueuesOutcome JsonClient::ListDeadLetterSourceQueues(
    const ListDeadLetterSourceQueuesRequest& request
) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    if (request.NextTokenHasBeenSet()) {
        json_request.WithString("NextToken", request.GetNextToken());
    }
    if (request.MaxResultsHasBeenSet()) {
        json_request.WithInteger("MaxResults", request.GetMaxResults());
    }

    const auto response = ExecuteJsonOperation(
        "ListDeadLetterSourceQueues",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<ListDeadLetterSourceQueuesOutcome>(response);
    }

    ListDeadLetterSourceQueuesResult result;
    if (response.json.View().KeyExists("queue_urls")) {
        const auto& queue_urls = response.json.View().GetArray("queue_urls");
        for (size_t i = 0; i < queue_urls.GetLength(); ++i) {
            result.AddQueueUrls(queue_urls[i].AsString());
        }
    } else if (response.json.View().KeyExists("QueueUrls")) {
        const auto& queue_urls = response.json.View().GetArray("QueueUrls");
        for (size_t i = 0; i < queue_urls.GetLength(); ++i) {
            result.AddQueueUrls(queue_urls[i].AsString());
        }
    }
    if (response.json.View().KeyExists("NextToken")) {
        result.SetNextToken(response.json.View().GetString("NextToken"));
    }

    return ListDeadLetterSourceQueuesOutcome(result);
}

ListQueueTagsOutcome JsonClient::ListQueueTags(const ListQueueTagsRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    const auto response = ExecuteJsonOperation(
        "ListQueueTags",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<ListQueueTagsOutcome>(response);
    }

    ListQueueTagsResult result;
    if (response.json.View().KeyExists("Tags")) {
        result.SetTags(ParseTagsJson(response.json.View().GetObject("Tags")));
    }

    return ListQueueTagsOutcome(result);
}

ListQueuesOutcome JsonClient::ListQueues(const ListQueuesRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;

    if (request.QueueNamePrefixHasBeenSet()) {
        json_request.WithString("QueueNamePrefix", request.GetQueueNamePrefix());
    }
    if (request.NextTokenHasBeenSet()) {
        json_request.WithString("NextToken", request.GetNextToken());
    }
    if (request.MaxResultsHasBeenSet()) {
        json_request.WithInteger("MaxResults", request.GetMaxResults());
    }

    const auto response = ExecuteJsonOperation(
        "ListQueues",
        Aws::String{impl_->settings.endpoint},
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<ListQueuesOutcome>(response);
    }

    ListQueuesResult result;
    if (response.json.View().KeyExists("QueueUrls")) {
        const auto& queue_urls = response.json.View().GetArray("QueueUrls");
        for (size_t i = 0; i < queue_urls.GetLength(); ++i) {
            result.AddQueueUrls(queue_urls[i].AsString());
        }
    }
    if (response.json.View().KeyExists("NextToken")) {
        result.SetNextToken(response.json.View().GetString("NextToken"));
    }

    return ListQueuesOutcome(result);
}

PurgeQueueOutcome JsonClient::PurgeQueue(const PurgeQueueRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    const auto response =
        ExecuteJsonOperation("PurgeQueue", request.GetQueueUrl(), request.GetAdditionalCustomHeaders(), json_request);
    if (!response.ok) {
        return MakeFailedOutcome<PurgeQueueOutcome>(response);
    }

    return PurgeQueueOutcome(Aws::NoResult());
}

ReceiveMessageOutcome JsonClient::ReceiveMessage(const ReceiveMessageRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    if (request.MaxNumberOfMessagesHasBeenSet()) {
        json_request.WithInteger("MaxNumberOfMessages", request.GetMaxNumberOfMessages());
    }
    if (request.VisibilityTimeoutHasBeenSet()) {
        json_request.WithInteger("VisibilityTimeout", request.GetVisibilityTimeout());
    }
    if (request.WaitTimeSecondsHasBeenSet()) {
        json_request.WithInteger("WaitTimeSeconds", request.GetWaitTimeSeconds());
    }
    if (request.ReceiveRequestAttemptIdHasBeenSet()) {
        json_request.WithString("ReceiveRequestAttemptId", request.GetReceiveRequestAttemptId());
    }

    if (!request.GetAttributeNames().empty()) {
        Aws::Vector<Aws::String> attribute_names;
        attribute_names.reserve(request.GetAttributeNames().size());
        for (const auto& attribute_name : request.GetAttributeNames()) {
            attribute_names.push_back(QueueAttributeNameMapper::GetNameForQueueAttributeName(attribute_name));
        }
        json_request.WithArray("AttributeNames", BuildStringArrayJson(attribute_names));
    }

    if (!request.GetMessageAttributeNames().empty()) {
        json_request.WithArray("MessageAttributeNames", BuildStringArrayJson(request.GetMessageAttributeNames()));
    }

    const auto response = ExecuteJsonOperation(
        "ReceiveMessage",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<ReceiveMessageOutcome>(response);
    }

    ReceiveMessageResult result;
    if (response.json.View().KeyExists("Messages")) {
        const auto& messages = response.json.View().GetArray("Messages");
        for (size_t i = 0; i < messages.GetLength(); ++i) {
            result.AddMessages(ParseMessageJson(messages[i]));
        }
    }

    return ReceiveMessageOutcome(result);
}

RemovePermissionOutcome JsonClient::RemovePermission(const RemovePermissionRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl()).WithString("Label", request.GetLabel());

    const auto response = ExecuteJsonOperation(
        "RemovePermission",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<RemovePermissionOutcome>(response);
    }

    return RemovePermissionOutcome(Aws::NoResult());
}

SendMessageOutcome JsonClient::SendMessage(const SendMessageRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl()).WithString("MessageBody", request.GetMessageBody());

    if (request.DelaySecondsHasBeenSet()) {
        json_request.WithInteger("DelaySeconds", request.GetDelaySeconds());
    }
    if (request.MessageGroupIdHasBeenSet()) {
        json_request.WithString("MessageGroupId", request.GetMessageGroupId());
    }
    if (request.MessageDeduplicationIdHasBeenSet()) {
        json_request.WithString("MessageDeduplicationId", request.GetMessageDeduplicationId());
    }
    if (request.MessageAttributesHasBeenSet()) {
        json_request.WithObject("MessageAttributes", BuildMessageAttributesJson(request.GetMessageAttributes()));
    }
    if (request.MessageSystemAttributesHasBeenSet()) {
        json_request.WithObject(
            "MessageSystemAttributes",
            BuildMessageSystemAttributesJson(request.GetMessageSystemAttributes())
        );
    }

    const auto response =
        ExecuteJsonOperation("SendMessage", request.GetQueueUrl(), request.GetAdditionalCustomHeaders(), json_request);
    if (!response.ok) {
        return MakeFailedOutcome<SendMessageOutcome>(response);
    }

    SendMessageResult result;
    if (response.json.View().KeyExists("MessageId")) {
        result.SetMessageId(response.json.View().GetString("MessageId"));
    }
    if (response.json.View().KeyExists("MD5OfMessageBody")) {
        result.SetMD5OfMessageBody(response.json.View().GetString("MD5OfMessageBody"));
    }
    if (response.json.View().KeyExists("MD5OfMessageAttributes")) {
        result.SetMD5OfMessageAttributes(response.json.View().GetString("MD5OfMessageAttributes"));
    }
    if (response.json.View().KeyExists("MD5OfMessageSystemAttributes")) {
        result.SetMD5OfMessageSystemAttributes(response.json.View().GetString("MD5OfMessageSystemAttributes"));
    }
    if (response.json.View().KeyExists("SequenceNumber")) {
        result.SetSequenceNumber(response.json.View().GetString("SequenceNumber"));
    }

    return SendMessageOutcome(result);
}

SendMessageBatchOutcome JsonClient::SendMessageBatch(const SendMessageBatchRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl());

    Aws::Vector<Aws::Utils::Json::JsonValue> entries_vector;
    entries_vector.reserve(request.GetEntries().size());
    for (const auto& entry : request.GetEntries()) {
        Aws::Utils::Json::JsonValue json_entry;
        json_entry.WithString("Id", entry.GetId()).WithString("MessageBody", entry.GetMessageBody());

        if (entry.MessageGroupIdHasBeenSet()) {
            json_entry.WithString("MessageGroupId", entry.GetMessageGroupId());
        }
        if (entry.MessageDeduplicationIdHasBeenSet()) {
            json_entry.WithString("MessageDeduplicationId", entry.GetMessageDeduplicationId());
        }
        if (entry.DelaySecondsHasBeenSet()) {
            json_entry.WithInteger("DelaySeconds", entry.GetDelaySeconds());
        }
        if (entry.MessageAttributesHasBeenSet()) {
            json_entry.WithObject("MessageAttributes", BuildMessageAttributesJson(entry.GetMessageAttributes()));
        }
        if (entry.MessageSystemAttributesHasBeenSet()) {
            json_entry.WithObject(
                "MessageSystemAttributes",
                BuildMessageSystemAttributesJson(entry.GetMessageSystemAttributes())
            );
        }

        entries_vector.push_back(json_entry);
    }

    Aws::Utils::Array<Aws::Utils::Json::JsonValue> entries_array(entries_vector.size());
    for (size_t i = 0; i < entries_vector.size(); ++i) {
        entries_array[i] = entries_vector[i];
    }
    json_request.WithArray("Entries", entries_array);

    const auto response = ExecuteJsonOperation(
        "SendMessageBatch",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<SendMessageBatchOutcome>(response);
    }

    SendMessageBatchResult result;
    if (response.json.View().KeyExists("Successful")) {
        const auto& successful = response.json.View().GetArray("Successful");
        for (size_t i = 0; i < successful.GetLength(); ++i) {
            auto entry =
                SendMessageBatchResultEntry()
                    .WithId(successful[i].GetString("Id"))
                    .WithMessageId(successful[i].GetString("MessageId"))
                    .WithMD5OfMessageBody(successful[i].GetString("MD5OfMessageBody"));
            if (successful[i].KeyExists("MD5OfMessageAttributes")) {
                entry.WithMD5OfMessageAttributes(successful[i].GetString("MD5OfMessageAttributes"));
            }
            if (successful[i].KeyExists("MD5OfMessageSystemAttributes")) {
                entry.WithMD5OfMessageSystemAttributes(successful[i].GetString("MD5OfMessageSystemAttributes"));
            }
            if (successful[i].KeyExists("SequenceNumber")) {
                entry.WithSequenceNumber(successful[i].GetString("SequenceNumber"));
            }
            result.AddSuccessful(entry);
        }
    }
    if (response.json.View().KeyExists("Failed")) {
        const auto& failed = response.json.View().GetArray("Failed");
        for (size_t i = 0; i < failed.GetLength(); ++i) {
            result
                .AddFailed(BatchResultErrorEntry()
                               .WithId(failed[i].GetString("Id"))
                               .WithSenderFault(failed[i].GetBool("SenderFault"))
                               .WithCode(failed[i].GetString("Code"))
                               .WithMessage(failed[i].GetString("Message")));
        }
    }

    return SendMessageBatchOutcome(result);
}

SetQueueAttributesOutcome JsonClient::SetQueueAttributes(const SetQueueAttributesRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl())
        .WithObject("Attributes", BuildQueueAttributesJson(request.GetAttributes()));

    const auto response = ExecuteJsonOperation(
        "SetQueueAttributes",
        request.GetQueueUrl(),
        request.GetAdditionalCustomHeaders(),
        json_request
    );
    if (!response.ok) {
        return MakeFailedOutcome<SetQueueAttributesOutcome>(response);
    }

    return SetQueueAttributesOutcome(Aws::NoResult());
}

TagQueueOutcome JsonClient::TagQueue(const TagQueueRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl()).WithObject("Tags", BuildTagsJson(request.GetTags()));

    const auto response =
        ExecuteJsonOperation("TagQueue", request.GetQueueUrl(), request.GetAdditionalCustomHeaders(), json_request);
    if (!response.ok) {
        return MakeFailedOutcome<TagQueueOutcome>(response);
    }

    return TagQueueOutcome(Aws::NoResult());
}

UntagQueueOutcome JsonClient::UntagQueue(const UntagQueueRequest& request) const {
    Aws::Utils::Json::JsonValue json_request;
    json_request.WithString("QueueUrl", request.GetQueueUrl())
        .WithArray("TagKeys", BuildStringArrayJson(request.GetTagKeys()));

    const auto response =
        ExecuteJsonOperation("UntagQueue", request.GetQueueUrl(), request.GetAdditionalCustomHeaders(), json_request);
    if (!response.ok) {
        return MakeFailedOutcome<UntagQueueOutcome>(response);
    }

    return UntagQueueOutcome(Aws::NoResult());
}

}  // namespace sqs

USERVER_NAMESPACE_END
