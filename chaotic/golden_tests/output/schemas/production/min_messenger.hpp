#pragma once

#include "min_messenger_fwd.hpp"

#include <cstdint>
#include <fmt/core.h>
#include <optional>
#include <string>
#include <userver/chaotic/io/std/vector.hpp>
#include <userver/chaotic/object.hpp>
#include <userver/formats/json/value.hpp>

#include <userver/chaotic/type_bundle_hpp.hpp>

namespace ns {

using V1ChannelId = std::int64_t;

using V1Login = std::string;

// Current user information, including authorization token if authorized (empty otherwise)
struct V1CurrentUser {
    std::optional<std::string> token{};
    ::ns::V1Login login{};
    std::string name{};
};

bool operator==(const V1CurrentUser & lhs,const V1CurrentUser & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1CurrentUser& value);

V1CurrentUser Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

V1CurrentUser Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

V1CurrentUser Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

V1CurrentUser FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

std::string ToJsonString(const V1CurrentUser& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1CurrentUser& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1CurrentUser& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1ChannelMessage {
    ::ns::V1CurrentUser current_user{};
    std::int64_t id{};
    std::string timestamp{};
    std::string message{};
};

bool operator==(const V1ChannelMessage & lhs,const V1ChannelMessage & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelMessage& value);

V1ChannelMessage Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

V1ChannelMessage Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

V1ChannelMessage Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

V1ChannelMessage FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

std::string ToJsonString(const V1ChannelMessage& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessage& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelMessage& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1ChannelMessageByTimestampRequest {
    ::ns::V1ChannelId channel_id{};
    std::string from{};
    std::optional<std::string> to{};
};

bool operator==(const V1ChannelMessageByTimestampRequest & lhs,const V1ChannelMessageByTimestampRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelMessageByTimestampRequest& value);

V1ChannelMessageByTimestampRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

V1ChannelMessageByTimestampRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

V1ChannelMessageByTimestampRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

V1ChannelMessageByTimestampRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

std::string ToJsonString(const V1ChannelMessageByTimestampRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageByTimestampRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelMessageByTimestampRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1ChannelMessageByTimestampResponse {
    std::vector<::ns::V1ChannelMessage> messages{};
};

bool operator==(const V1ChannelMessageByTimestampResponse & lhs,const V1ChannelMessageByTimestampResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelMessageByTimestampResponse& value);

V1ChannelMessageByTimestampResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

V1ChannelMessageByTimestampResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

V1ChannelMessageByTimestampResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

V1ChannelMessageByTimestampResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

std::string ToJsonString(const V1ChannelMessageByTimestampResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageByTimestampResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelMessageByTimestampResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1ChannelMessageNewRequest {
    ::ns::V1CurrentUser current_user{};
    ::ns::V1ChannelId channel_id{};
    std::string message{};
};

bool operator==(const V1ChannelMessageNewRequest & lhs,const V1ChannelMessageNewRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelMessageNewRequest& value);

V1ChannelMessageNewRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

V1ChannelMessageNewRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

V1ChannelMessageNewRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

V1ChannelMessageNewRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

std::string ToJsonString(const V1ChannelMessageNewRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageNewRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelMessageNewRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

using V1MessageId = std::int64_t;

struct V1ChannelMessageNewResponse {
    ::ns::V1MessageId message_id{};
};

bool operator==(const V1ChannelMessageNewResponse & lhs,const V1ChannelMessageNewResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelMessageNewResponse& value);

V1ChannelMessageNewResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

V1ChannelMessageNewResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

V1ChannelMessageNewResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

V1ChannelMessageNewResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

std::string ToJsonString(const V1ChannelMessageNewResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelMessageNewResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1ChannelNotificationListRequest {
    ::ns::V1CurrentUser current_user{};
    ::ns::V1ChannelId channel_id{};
};

bool operator==(const V1ChannelNotificationListRequest & lhs,const V1ChannelNotificationListRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelNotificationListRequest& value);

V1ChannelNotificationListRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

V1ChannelNotificationListRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

V1ChannelNotificationListRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

V1ChannelNotificationListRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

std::string ToJsonString(const V1ChannelNotificationListRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationListRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelNotificationListRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1ChannelNotificationListResponse {
    std::vector<std::int64_t> notifications{};
};

bool operator==(const V1ChannelNotificationListResponse & lhs,const V1ChannelNotificationListResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelNotificationListResponse& value);

V1ChannelNotificationListResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

V1ChannelNotificationListResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

V1ChannelNotificationListResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

V1ChannelNotificationListResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

std::string ToJsonString(const V1ChannelNotificationListResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationListResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelNotificationListResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

// Notify other user that message requires his attention.
struct V1ChannelNotificationNewRequest {
    ::ns::V1CurrentUser current_user{};
    ::ns::V1ChannelId channel_id{};
    ::ns::V1MessageId message_id{};
    std::optional<::ns::V1Login> other_user_login{};
};

bool operator==(const V1ChannelNotificationNewRequest & lhs,const V1ChannelNotificationNewRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelNotificationNewRequest& value);

V1ChannelNotificationNewRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

V1ChannelNotificationNewRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

V1ChannelNotificationNewRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

V1ChannelNotificationNewRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

std::string ToJsonString(const V1ChannelNotificationNewRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationNewRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelNotificationNewRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1ChannelNotificationNewResponse {
};

bool operator==(const V1ChannelNotificationNewResponse & lhs,const V1ChannelNotificationNewResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1ChannelNotificationNewResponse& value);

V1ChannelNotificationNewResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

V1ChannelNotificationNewResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

V1ChannelNotificationNewResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

V1ChannelNotificationNewResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

std::string ToJsonString(const V1ChannelNotificationNewResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1ChannelNotificationNewResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

// Structure to report any error.
//
//The simplest way to report error in userver is via throwing some exception derived from
//server::handlers::CustomHandlerException. For example:
//
//throw server::handlers::ClientError(                            // writes 'client_error' to V1Error.code
//    server::handlers::ExternalBody{"We do not accept key 42"},  // goes to V1Error.message
//    server::handlers::InternalMessage{"User sent a 42 key"}     // goes to server logs
//);
struct V1Error {
    // Any additional info
    struct Details {
        USERVER_NAMESPACE::formats::json::Value extra{};
    };

    std::string code{};
    std::string message{};
    std::optional<::ns::V1Error::Details> details{};
};

bool operator==(const V1Error::Details & lhs,const V1Error::Details & rhs);

bool operator==(const V1Error & lhs,const V1Error & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1Error::Details& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1Error& value);

V1Error::Details Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1Error::Details>);

V1Error Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1Error>);

V1Error::Details Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1Error::Details>);

V1Error Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1Error>);

V1Error::Details Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1Error::Details>);

V1Error Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1Error>);

V1Error FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1Error>);

std::string ToJsonString(const V1Error& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1Error::Details& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1Error& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1Error::Details& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

void WriteToStream(
    const V1Error& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1File {
    ::ns::V1Login login{};
    std::string filename{};
    std::string content{};
};

bool operator==(const V1File & lhs,const V1File & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1File& value);

V1File Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1File>);

V1File Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1File>);

V1File Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1File>);

V1File FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1File>);

std::string ToJsonString(const V1File& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1File& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1File& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1FileByUriRequest {
    ::ns::V1CurrentUser current_user{};
    std::string uri{};
};

bool operator==(const V1FileByUriRequest & lhs,const V1FileByUriRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1FileByUriRequest& value);

V1FileByUriRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

V1FileByUriRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

V1FileByUriRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

V1FileByUriRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

std::string ToJsonString(const V1FileByUriRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1FileByUriRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1FileByUriRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

using V1FileByUriResponse = ::ns::V1File;

using V1FileNewRequest = ::ns::V1File;

struct V1FileNewResponse {
    ::ns::V1CurrentUser current_user{};
    std::string uri{};
};

bool operator==(const V1FileNewResponse & lhs,const V1FileNewResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1FileNewResponse& value);

V1FileNewResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

V1FileNewResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

V1FileNewResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

V1FileNewResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

std::string ToJsonString(const V1FileNewResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1FileNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1FileNewResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

// Adds/removes a specified animation on a message
struct V1LikeTriggerRequest {
    enum class Animation {
        kLike,
        kDislike,
        kHeart,
        kShit,
        kOkay,
        kLol,
        kSmile,
    };

    static constexpr Animation kAnimationValues [] = {
        Animation::kLike,
        Animation::kDislike,
        Animation::kHeart,
        Animation::kShit,
        Animation::kOkay,
        Animation::kLol,
        Animation::kSmile,
    };

    ::ns::V1CurrentUser current_user{};
    std::string idempotency_token{};
    ::ns::V1ChannelId channel_id{};
    ::ns::V1MessageId message_id{};
    ::ns::V1LikeTriggerRequest::Animation animation{};
};

bool operator==(const V1LikeTriggerRequest & lhs,const V1LikeTriggerRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1LikeTriggerRequest::Animation& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1LikeTriggerRequest& value);

V1LikeTriggerRequest::Animation Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

V1LikeTriggerRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

V1LikeTriggerRequest::Animation Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

V1LikeTriggerRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

V1LikeTriggerRequest::Animation Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

V1LikeTriggerRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

V1LikeTriggerRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

std::string ToJsonString(const V1LikeTriggerRequest& value);

V1LikeTriggerRequest::Animation Convert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<V1LikeTriggerRequest::Animation>);

std::optional<V1LikeTriggerRequest::Animation> TryConvert(
    std::string_view value,
    USERVER_NAMESPACE::chaotic::convert::To<V1LikeTriggerRequest::Animation>) noexcept;

V1LikeTriggerRequest::Animation Parse(
    std::string_view value,
    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1LikeTriggerRequest::Animation& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1LikeTriggerRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1LikeTriggerRequest::Animation& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw);

void WriteToStream(
    const V1LikeTriggerRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

std::string ToString(V1LikeTriggerRequest::Animation value);

// Authorization scenario:
//
//User sends `POST /v1/user/authorization` request with this object in body. If the authorization is
//successful (the user previously registered), then the HTTP 200 is returned with
//V1UserAuthorizationResponse object in response body.
struct V1UserAuthorizationRequest {
    ::ns::V1Login login{};
    std::string password{};
};

bool operator==(const V1UserAuthorizationRequest & lhs,const V1UserAuthorizationRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserAuthorizationRequest& value);

V1UserAuthorizationRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

V1UserAuthorizationRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

V1UserAuthorizationRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

V1UserAuthorizationRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

std::string ToJsonString(const V1UserAuthorizationRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserAuthorizationRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserAuthorizationRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1UserAuthorizationResponse {
    ::ns::V1CurrentUser current_user{};
};

bool operator==(const V1UserAuthorizationResponse & lhs,const V1UserAuthorizationResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserAuthorizationResponse& value);

V1UserAuthorizationResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

V1UserAuthorizationResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

V1UserAuthorizationResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

V1UserAuthorizationResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

std::string ToJsonString(const V1UserAuthorizationResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserAuthorizationResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserAuthorizationResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

// Registration scenario:
//
//User sends `POST /v1/user/registration` request with this object in body. If the registration is
//successful, then the HTTP 200 is returned with V1UserRegistrationResponse object in response body.
struct V1UserRegistrationRequest {
    ::ns::V1Login login{};
    std::string name{};
    std::string email{};
    std::string phone{};
    std::string password{};
};

bool operator==(const V1UserRegistrationRequest & lhs,const V1UserRegistrationRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserRegistrationRequest& value);

V1UserRegistrationRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

V1UserRegistrationRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

V1UserRegistrationRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

V1UserRegistrationRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

std::string ToJsonString(const V1UserRegistrationRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserRegistrationRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserRegistrationRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

using V1UserRegistrationResponse = ::ns::V1UserAuthorizationResponse;

// Key-values of arbitrary types (in other words, just JSON without a schema)
struct V1UserStatus {
    USERVER_NAMESPACE::formats::json::Value extra{};
};

bool operator==(const V1UserStatus & lhs,const V1UserStatus & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserStatus& value);

V1UserStatus Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

V1UserStatus Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

V1UserStatus Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

V1UserStatus FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

std::string ToJsonString(const V1UserStatus& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatus& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserStatus& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1UserStatusByLoginRequest {
    ::ns::V1CurrentUser current_user{};
    ::ns::V1Login login{};
};

bool operator==(const V1UserStatusByLoginRequest & lhs,const V1UserStatusByLoginRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserStatusByLoginRequest& value);

V1UserStatusByLoginRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

V1UserStatusByLoginRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

V1UserStatusByLoginRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

V1UserStatusByLoginRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

std::string ToJsonString(const V1UserStatusByLoginRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusByLoginRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserStatusByLoginRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1UserStatusByLoginResponse {
    ::ns::V1UserStatus status{};
};

bool operator==(const V1UserStatusByLoginResponse & lhs,const V1UserStatusByLoginResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserStatusByLoginResponse& value);

V1UserStatusByLoginResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

V1UserStatusByLoginResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

V1UserStatusByLoginResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

V1UserStatusByLoginResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

std::string ToJsonString(const V1UserStatusByLoginResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusByLoginResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserStatusByLoginResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1UserStatusUpdateRequest {
    ::ns::V1CurrentUser current_user{};
    ::ns::V1UserStatus status{};
};

bool operator==(const V1UserStatusUpdateRequest & lhs,const V1UserStatusUpdateRequest & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserStatusUpdateRequest& value);

V1UserStatusUpdateRequest Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

V1UserStatusUpdateRequest Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

V1UserStatusUpdateRequest Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

V1UserStatusUpdateRequest FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

std::string ToJsonString(const V1UserStatusUpdateRequest& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusUpdateRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserStatusUpdateRequest& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

struct V1UserStatusUpdateResponse {
};

bool operator==(const V1UserStatusUpdateResponse & lhs,const V1UserStatusUpdateResponse & rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserStatusUpdateResponse& value);

V1UserStatusUpdateResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

V1UserStatusUpdateResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

V1UserStatusUpdateResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

V1UserStatusUpdateResponse FromJsonString(
    std::string_view json,
    USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

std::string ToJsonString(const V1UserStatusUpdateResponse& value);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusUpdateResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

void WriteToStream(
    const V1UserStatusUpdateResponse& value,
    USERVER_NAMESPACE::formats::json::StringBuilder& sw,
    bool hide_brackets = false,
    std::string_view hide_field_name = {});

}  // namespace ns

template<>
struct fmt::formatter<ns::V1LikeTriggerRequest::Animation> {
    fmt::format_context::iterator format(const ns::V1LikeTriggerRequest::Animation&, fmt::format_context&) const;

    constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) {
        return ctx.begin();
    }
};

