#pragma once

#include <fmt/core.h>

#include <cstdint>
#include <optional>
#include <string>
#include <userver/chaotic/io/std/vector.hpp>
#include <userver/chaotic/object.hpp>
#include <userver/chaotic/type_bundle_hpp.hpp>
#include <userver/formats/json/value.hpp>

#include "min_messenger_fwd.hpp"

namespace ns {

using V1ChannelId = std::int64_t;

using V1Login = std::string;

// Current user information, including authorization token if authorized (empty otherwise)
struct V1CurrentUser {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNametoken = "token";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamelogin = "login";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamename = "name";
  std::optional<std::string> token{};
  ::ns::V1Login login{};
  std::string name{};
};

bool operator==(const V1CurrentUser& lhs, const V1CurrentUser& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1CurrentUser& value);

V1CurrentUser Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

V1CurrentUser Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

V1CurrentUser Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

V1CurrentUser FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1CurrentUser& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1ChannelMessage {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameid = "id";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNametimestamp = "timestamp";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamemessage = "message";
  ::ns::V1CurrentUser current_user{};
  std::int64_t id{};
  std::string timestamp{};
  std::string message{};
};

bool operator==(const V1ChannelMessage& lhs, const V1ChannelMessage& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessage& value);

V1ChannelMessage Parse(USERVER_NAMESPACE::formats::json::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

V1ChannelMessage Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

V1ChannelMessage Parse(USERVER_NAMESPACE::yaml_config::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

V1ChannelMessage FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessage& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1ChannelMessageByTimestampRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamechannel_id = "channel_id";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamefrom = "from";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameto = "to";
  ::ns::V1ChannelId channel_id{};
  std::string from{};
  std::optional<std::string> to{};
};

bool operator==(const V1ChannelMessageByTimestampRequest& lhs, const V1ChannelMessageByTimestampRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageByTimestampRequest& value);

V1ChannelMessageByTimestampRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                         USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

V1ChannelMessageByTimestampRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                         USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

V1ChannelMessageByTimestampRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                         USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

V1ChannelMessageByTimestampRequest FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageByTimestampRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1ChannelMessageByTimestampResponse {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamemessages = "messages";
  std::vector<::ns::V1ChannelMessage> messages{};
};

bool operator==(const V1ChannelMessageByTimestampResponse& lhs, const V1ChannelMessageByTimestampResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageByTimestampResponse& value);

V1ChannelMessageByTimestampResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                          USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

V1ChannelMessageByTimestampResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                          USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

V1ChannelMessageByTimestampResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                          USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

V1ChannelMessageByTimestampResponse FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageByTimestampResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1ChannelMessageNewRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamechannel_id = "channel_id";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamemessage = "message";
  ::ns::V1CurrentUser current_user{};
  ::ns::V1ChannelId channel_id{};
  std::string message{};
};

bool operator==(const V1ChannelMessageNewRequest& lhs, const V1ChannelMessageNewRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageNewRequest& value);

V1ChannelMessageNewRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

V1ChannelMessageNewRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

V1ChannelMessageNewRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

V1ChannelMessageNewRequest FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageNewRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

using V1MessageId = std::int64_t;

struct V1ChannelMessageNewResponse {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamemessage_id = "message_id";
  ::ns::V1MessageId message_id{};
};

bool operator==(const V1ChannelMessageNewResponse& lhs, const V1ChannelMessageNewResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageNewResponse& value);

V1ChannelMessageNewResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

V1ChannelMessageNewResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

V1ChannelMessageNewResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

V1ChannelMessageNewResponse FromJsonString(std::string_view json,
                                           USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelMessageNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1ChannelNotificationListRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamechannel_id = "channel_id";
  ::ns::V1CurrentUser current_user{};
  ::ns::V1ChannelId channel_id{};
};

bool operator==(const V1ChannelNotificationListRequest& lhs, const V1ChannelNotificationListRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationListRequest& value);

V1ChannelNotificationListRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

V1ChannelNotificationListRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

V1ChannelNotificationListRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

V1ChannelNotificationListRequest FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationListRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1ChannelNotificationListResponse {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamenotifications = "notifications";
  std::vector<std::int64_t> notifications{};
};

bool operator==(const V1ChannelNotificationListResponse& lhs, const V1ChannelNotificationListResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationListResponse& value);

V1ChannelNotificationListResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                        USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

V1ChannelNotificationListResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                        USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

V1ChannelNotificationListResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                        USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

V1ChannelNotificationListResponse FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationListResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

// Notify other user that message requires his attention.
struct V1ChannelNotificationNewRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamechannel_id = "channel_id";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamemessage_id = "message_id";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameother_user_login = "other_user_login";
  ::ns::V1CurrentUser current_user{};
  ::ns::V1ChannelId channel_id{};
  ::ns::V1MessageId message_id{};
  std::optional<::ns::V1Login> other_user_login{};
};

bool operator==(const V1ChannelNotificationNewRequest& lhs, const V1ChannelNotificationNewRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationNewRequest& value);

V1ChannelNotificationNewRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

V1ChannelNotificationNewRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

V1ChannelNotificationNewRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

V1ChannelNotificationNewRequest FromJsonString(std::string_view json,
                                               USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationNewRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1ChannelNotificationNewResponse {};

bool operator==(const V1ChannelNotificationNewResponse& lhs, const V1ChannelNotificationNewResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationNewResponse& value);

V1ChannelNotificationNewResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

V1ChannelNotificationNewResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

V1ChannelNotificationNewResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

V1ChannelNotificationNewResponse FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1ChannelNotificationNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

// Structure to report any error.
//
// The simplest way to report error in userver is via throwing some exception derived from
// server::handlers::CustomHandlerException. For example:
//
// throw server::handlers::ClientError(                            // writes 'client_error' to V1Error.code
//    server::handlers::ExternalBody{"We do not accept key 42"},  // goes to V1Error.message
//    server::handlers::InternalMessage{"User sent a 42 key"}     // goes to server logs
//);
struct V1Error {
  // Any additional info
  struct Details {
    USERVER_NAMESPACE::formats::json::Value extra;
  };

  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecode = "code";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamemessage = "message";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamedetails = "details";
  std::string code{};
  std::string message{};
  std::optional<::ns::V1Error::Details> details{};
};

bool operator==(const V1Error::Details& lhs, const V1Error::Details& rhs);

bool operator==(const V1Error& lhs, const V1Error& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1Error::Details& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1Error& value);

V1Error::Details Parse(USERVER_NAMESPACE::formats::json::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1Error::Details>);

V1Error Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<V1Error>);

V1Error::Details Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1Error::Details>);

V1Error Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<V1Error>);

V1Error::Details Parse(USERVER_NAMESPACE::yaml_config::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1Error::Details>);

V1Error Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<V1Error>);

V1Error FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1Error>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1Error::Details& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1Error& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1File {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamelogin = "login";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamefilename = "filename";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecontent = "content";
  ::ns::V1Login login{};
  std::string filename{};
  std::string content{};
};

bool operator==(const V1File& lhs, const V1File& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1File& value);

V1File Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<V1File>);

V1File Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<V1File>);

V1File Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<V1File>);

V1File FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1File>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1File& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1FileByUriRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameuri = "uri";
  ::ns::V1CurrentUser current_user{};
  std::string uri{};
};

bool operator==(const V1FileByUriRequest& lhs, const V1FileByUriRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1FileByUriRequest& value);

V1FileByUriRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                         USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

V1FileByUriRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                         USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

V1FileByUriRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                         USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

V1FileByUriRequest FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1FileByUriRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

using V1FileByUriResponse = ::ns::V1File;

using V1FileNewRequest = ::ns::V1File;

struct V1FileNewResponse {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameuri = "uri";
  ::ns::V1CurrentUser current_user{};
  std::string uri{};
};

bool operator==(const V1FileNewResponse& lhs, const V1FileNewResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1FileNewResponse& value);

V1FileNewResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                        USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

V1FileNewResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                        USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

V1FileNewResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                        USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

V1FileNewResponse FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1FileNewResponse& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

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

  static constexpr Animation kAnimationValues[] = {
      Animation::kLike, Animation::kDislike, Animation::kHeart, Animation::kShit,
      Animation::kOkay, Animation::kLol,     Animation::kSmile,
  };

  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameidempotency_token = "idempotency_token";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamechannel_id = "channel_id";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamemessage_id = "message_id";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameanimation = "animation";
  ::ns::V1CurrentUser current_user{};
  std::string idempotency_token{};
  ::ns::V1ChannelId channel_id{};
  ::ns::V1MessageId message_id{};
  ::ns::V1LikeTriggerRequest::Animation animation{};
};

bool operator==(const V1LikeTriggerRequest& lhs, const V1LikeTriggerRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1LikeTriggerRequest::Animation& value);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1LikeTriggerRequest& value);

V1LikeTriggerRequest::Animation Parse(USERVER_NAMESPACE::formats::json::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

V1LikeTriggerRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                           USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

V1LikeTriggerRequest::Animation Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

V1LikeTriggerRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                           USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

V1LikeTriggerRequest::Animation Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

V1LikeTriggerRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                           USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

V1LikeTriggerRequest FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>);

V1LikeTriggerRequest::Animation FromString(std::string_view value,
                                           USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

V1LikeTriggerRequest::Animation Parse(std::string_view value,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1LikeTriggerRequest::Animation& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1LikeTriggerRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

std::string ToString(V1LikeTriggerRequest::Animation value);

// Authorization scenario:
//
// User sends `POST /v1/user/authorization` request with this object in body. If the authorization is
// successful (the user previously registered), then the HTTP 200 is returned with
// V1UserAuthorizationResponse object in response body.
struct V1UserAuthorizationRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamelogin = "login";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamepassword = "password";
  ::ns::V1Login login{};
  std::string password{};
};

bool operator==(const V1UserAuthorizationRequest& lhs, const V1UserAuthorizationRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserAuthorizationRequest& value);

V1UserAuthorizationRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

V1UserAuthorizationRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

V1UserAuthorizationRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

V1UserAuthorizationRequest FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserAuthorizationRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1UserAuthorizationResponse {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  ::ns::V1CurrentUser current_user{};
};

bool operator==(const V1UserAuthorizationResponse& lhs, const V1UserAuthorizationResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserAuthorizationResponse& value);

V1UserAuthorizationResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

V1UserAuthorizationResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

V1UserAuthorizationResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

V1UserAuthorizationResponse FromJsonString(std::string_view json,
                                           USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserAuthorizationResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

// Registration scenario:
//
// User sends `POST /v1/user/registration` request with this object in body. If the registration is
// successful, then the HTTP 200 is returned with V1UserRegistrationResponse object in response body.
struct V1UserRegistrationRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamelogin = "login";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamename = "name";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNameemail = "email";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamephone = "phone";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamepassword = "password";
  ::ns::V1Login login{};
  std::string name{};
  std::string email{};
  std::string phone{};
  std::string password{};
};

bool operator==(const V1UserRegistrationRequest& lhs, const V1UserRegistrationRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserRegistrationRequest& value);

V1UserRegistrationRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

V1UserRegistrationRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

V1UserRegistrationRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

V1UserRegistrationRequest FromJsonString(std::string_view json,
                                         USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserRegistrationRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

using V1UserRegistrationResponse = ::ns::V1UserAuthorizationResponse;

// Key-values of arbitrary types (in other words, just JSON without a schema)
struct V1UserStatus {
  USERVER_NAMESPACE::formats::json::Value extra;
};

bool operator==(const V1UserStatus& lhs, const V1UserStatus& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1UserStatus& value);

V1UserStatus Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

V1UserStatus Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

V1UserStatus Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

V1UserStatus FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1UserStatus>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatus& value, USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1UserStatusByLoginRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamelogin = "login";
  ::ns::V1CurrentUser current_user{};
  ::ns::V1Login login{};
};

bool operator==(const V1UserStatusByLoginRequest& lhs, const V1UserStatusByLoginRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusByLoginRequest& value);

V1UserStatusByLoginRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

V1UserStatusByLoginRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

V1UserStatusByLoginRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

V1UserStatusByLoginRequest FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusByLoginRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1UserStatusByLoginResponse {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamestatus = "status";
  ::ns::V1UserStatus status{};
};

bool operator==(const V1UserStatusByLoginResponse& lhs, const V1UserStatusByLoginResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusByLoginResponse& value);

V1UserStatusByLoginResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

V1UserStatusByLoginResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

V1UserStatusByLoginResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

V1UserStatusByLoginResponse FromJsonString(std::string_view json,
                                           USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusByLoginResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1UserStatusUpdateRequest {
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamecurrent_user = "current_user";
  static constexpr USERVER_NAMESPACE::utils::StringLiteral kFieldNamestatus = "status";
  ::ns::V1CurrentUser current_user{};
  ::ns::V1UserStatus status{};
};

bool operator==(const V1UserStatusUpdateRequest& lhs, const V1UserStatusUpdateRequest& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusUpdateRequest& value);

V1UserStatusUpdateRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

V1UserStatusUpdateRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

V1UserStatusUpdateRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

V1UserStatusUpdateRequest FromJsonString(std::string_view json,
                                         USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusUpdateRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

struct V1UserStatusUpdateResponse {};

bool operator==(const V1UserStatusUpdateResponse& lhs, const V1UserStatusUpdateResponse& rhs);

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusUpdateResponse& value);

V1UserStatusUpdateResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

V1UserStatusUpdateResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

V1UserStatusUpdateResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

V1UserStatusUpdateResponse FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>);

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1UserStatusUpdateResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>);

}  // namespace ns

template <>
struct fmt::formatter<ns::V1LikeTriggerRequest::Animation> {
  fmt::format_context::iterator format(const ns::V1LikeTriggerRequest::Animation&, fmt::format_context&) const;

  constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context& ctx) { return ctx.begin(); }
};

