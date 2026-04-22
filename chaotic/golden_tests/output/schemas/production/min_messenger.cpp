#include <userver/chaotic/type_bundle_cpp.hpp>

#include "min_messenger.hpp"
#include "min_messenger_parsers.ipp"
#include "min_messenger_sax_parsers.hpp"

namespace ns {

V1CurrentUser FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1CurrentUser, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                         USERVER_NAMESPACE::chaotic::sax::Parser<V1CurrentUser>>>(json);
}

bool operator==(const V1CurrentUser& lhs, const V1CurrentUser& rhs) {
  return lhs.token == rhs.token && lhs.login == rhs.login && lhs.name == rhs.name && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1CurrentUser& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1CurrentUser Parse(USERVER_NAMESPACE::formats::json::Value json,
                    USERVER_NAMESPACE::formats::parse::To<V1CurrentUser> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1CurrentUser Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                    USERVER_NAMESPACE::formats::parse::To<V1CurrentUser> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1CurrentUser Parse(USERVER_NAMESPACE::yaml_config::Value json,
                    USERVER_NAMESPACE::formats::parse::To<V1CurrentUser> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1CurrentUser& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  if (value.token) {
    vb["token"] = USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<128>,
                                                        USERVER_NAMESPACE::chaotic::MaxLength<128>>{*value.token};
  }

  vb["login"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.login};

  vb["name"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{value.name};

  return vb.ExtractValue();
}

V1ChannelMessage FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelMessage, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                            USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelMessage>>>(json);
}

bool operator==(const V1ChannelMessage& lhs, const V1ChannelMessage& rhs) {
  return lhs.current_user == rhs.current_user && lhs.id == rhs.id && lhs.timestamp == rhs.timestamp &&
         lhs.message == rhs.message && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessage& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelMessage Parse(USERVER_NAMESPACE::formats::json::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelMessage Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelMessage Parse(USERVER_NAMESPACE::yaml_config::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelMessage& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.id};

  vb["timestamp"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{value.timestamp};

  vb["message"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>{value.message};

  return vb.ExtractValue();
}

V1ChannelMessageByTimestampRequest FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelMessageByTimestampRequest,
      USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
          USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelMessageByTimestampRequest>>>(json);
}

bool operator==(const V1ChannelMessageByTimestampRequest& lhs, const V1ChannelMessageByTimestampRequest& rhs) {
  return lhs.channel_id == rhs.channel_id && lhs.from == rhs.from && lhs.to == rhs.to && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageByTimestampRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelMessageByTimestampRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                         USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelMessageByTimestampRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                         USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelMessageByTimestampRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                         USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelMessageByTimestampRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["channel_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.channel_id};

  vb["from"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{value.from};

  if (value.to) {
    vb["to"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{*value.to};
  }

  return vb.ExtractValue();
}

V1ChannelMessageByTimestampResponse FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelMessageByTimestampResponse,
      USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
          USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelMessageByTimestampResponse>>>(json);
}

bool operator==(const V1ChannelMessageByTimestampResponse& lhs, const V1ChannelMessageByTimestampResponse& rhs) {
  return lhs.messages == rhs.messages && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageByTimestampResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelMessageByTimestampResponse Parse(
    USERVER_NAMESPACE::formats::json::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelMessageByTimestampResponse Parse(
    USERVER_NAMESPACE::formats::yaml::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelMessageByTimestampResponse Parse(
    USERVER_NAMESPACE::yaml_config::Value json,
    USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelMessageByTimestampResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["messages"] = USERVER_NAMESPACE::chaotic::Array<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1ChannelMessage>,
                                                     std::vector<::ns::V1ChannelMessage>>{value.messages};

  return vb.ExtractValue();
}

V1ChannelMessageNewRequest FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelMessageNewRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                      USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelMessageNewRequest>>>(json);
}

bool operator==(const V1ChannelMessageNewRequest& lhs, const V1ChannelMessageNewRequest& rhs) {
  return lhs.current_user == rhs.current_user && lhs.channel_id == rhs.channel_id && lhs.message == rhs.message && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageNewRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelMessageNewRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelMessageNewRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelMessageNewRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelMessageNewRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["channel_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.channel_id};

  vb["message"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>{value.message};

  return vb.ExtractValue();
}

V1ChannelMessageNewResponse FromJsonString(std::string_view json,
                                           USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelMessageNewResponse, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                       USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelMessageNewResponse>>>(json);
}

bool operator==(const V1ChannelMessageNewResponse& lhs, const V1ChannelMessageNewResponse& rhs) {
  return lhs.message_id == rhs.message_id && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelMessageNewResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelMessageNewResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelMessageNewResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelMessageNewResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelMessageNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["message_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.message_id};

  return vb.ExtractValue();
}

V1ChannelNotificationListRequest FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelNotificationListRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                            USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelNotificationListRequest>>>(
      json);
}

bool operator==(const V1ChannelNotificationListRequest& lhs, const V1ChannelNotificationListRequest& rhs) {
  return lhs.current_user == rhs.current_user && lhs.channel_id == rhs.channel_id && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationListRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelNotificationListRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelNotificationListRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelNotificationListRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelNotificationListRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["channel_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.channel_id};

  return vb.ExtractValue();
}

V1ChannelNotificationListResponse FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelNotificationListResponse,
      USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
          USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelNotificationListResponse>>>(json);
}

bool operator==(const V1ChannelNotificationListResponse& lhs, const V1ChannelNotificationListResponse& rhs) {
  return lhs.notifications == rhs.notifications && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationListResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelNotificationListResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                        USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelNotificationListResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                        USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelNotificationListResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                        USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelNotificationListResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["notifications"] =
      USERVER_NAMESPACE::chaotic::Array<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>, std::vector<std::int64_t>>{
          value.notifications};

  return vb.ExtractValue();
}

V1ChannelNotificationNewRequest FromJsonString(std::string_view json,
                                               USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelNotificationNewRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                           USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelNotificationNewRequest>>>(
      json);
}

bool operator==(const V1ChannelNotificationNewRequest& lhs, const V1ChannelNotificationNewRequest& rhs) {
  return lhs.current_user == rhs.current_user && lhs.channel_id == rhs.channel_id && lhs.message_id == rhs.message_id &&
         lhs.other_user_login == rhs.other_user_login && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationNewRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelNotificationNewRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelNotificationNewRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelNotificationNewRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelNotificationNewRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["channel_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.channel_id};

  vb["message_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.message_id};

  if (value.other_user_login) {
    vb["other_user_login"] =
        USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{
            *value.other_user_login};
  }

  return vb.ExtractValue();
}

V1ChannelNotificationNewResponse FromJsonString(
    std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1ChannelNotificationNewResponse, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                            USERVER_NAMESPACE::chaotic::sax::Parser<V1ChannelNotificationNewResponse>>>(
      json);
}

bool operator==(const V1ChannelNotificationNewResponse& lhs, const V1ChannelNotificationNewResponse& rhs) {
  (void)lhs;
  (void)rhs;

  return

      true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1ChannelNotificationNewResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1ChannelNotificationNewResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1ChannelNotificationNewResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1ChannelNotificationNewResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1ChannelNotificationNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  return vb.ExtractValue();
}

V1Error FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1Error>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1Error,
      USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<USERVER_NAMESPACE::chaotic::sax::Parser<V1Error>>>(
      json);
}

bool operator==(const V1Error::Details& lhs, const V1Error::Details& rhs) {
  (void)lhs;
  (void)rhs;

  return

      lhs.extra == rhs.extra &&

      true;
}

bool operator==(const V1Error& lhs, const V1Error& rhs) {
  return lhs.code == rhs.code && lhs.message == rhs.message && lhs.details == rhs.details && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1Error::Details& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1Error& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1Error::Details Parse(USERVER_NAMESPACE::formats::json::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1Error::Details> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1Error Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<V1Error> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1Error::Details Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1Error::Details> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1Error Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<V1Error> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1Error::Details Parse(USERVER_NAMESPACE::yaml_config::Value json,
                       USERVER_NAMESPACE::formats::parse::To<V1Error::Details> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

V1Error Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<V1Error> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1Error::Details& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = value.extra;

  return vb.ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1Error& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["code"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{value.code};

  vb["message"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{value.message};

  if (value.details) {
    vb["details"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1Error::Details>{*value.details};
  }

  return vb.ExtractValue();
}

V1File FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1File>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1File,
      USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<USERVER_NAMESPACE::chaotic::sax::Parser<V1File>>>(
      json);
}

bool operator==(const V1File& lhs, const V1File& rhs) {
  return lhs.login == rhs.login && lhs.filename == rhs.filename && lhs.content == rhs.content && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh, const V1File& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1File Parse(USERVER_NAMESPACE::formats::json::Value json, USERVER_NAMESPACE::formats::parse::To<V1File> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1File Parse(USERVER_NAMESPACE::formats::yaml::Value json, USERVER_NAMESPACE::formats::parse::To<V1File> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1File Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<V1File> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1File& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["login"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.login};

  vb["filename"] = USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>,
                                                         USERVER_NAMESPACE::chaotic::MaxLength<256>>{value.filename};

  vb["content"] = USERVER_NAMESPACE::chaotic::Primitive<std::string>{value.content};

  return vb.ExtractValue();
}

V1FileByUriRequest FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1FileByUriRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                              USERVER_NAMESPACE::chaotic::sax::Parser<V1FileByUriRequest>>>(json);
}

bool operator==(const V1FileByUriRequest& lhs, const V1FileByUriRequest& rhs) {
  return lhs.current_user == rhs.current_user && lhs.uri == rhs.uri && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1FileByUriRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1FileByUriRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                         USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1FileByUriRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                         USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1FileByUriRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                         USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1FileByUriRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["uri"] = USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.uri};

  return vb.ExtractValue();
}

V1FileNewResponse FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1FileNewResponse, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                             USERVER_NAMESPACE::chaotic::sax::Parser<V1FileNewResponse>>>(json);
}

bool operator==(const V1FileNewResponse& lhs, const V1FileNewResponse& rhs) {
  return lhs.current_user == rhs.current_user && lhs.uri == rhs.uri && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1FileNewResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1FileNewResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                        USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1FileNewResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                        USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1FileNewResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                        USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1FileNewResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["uri"] = USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.uri};

  return vb.ExtractValue();
}

V1LikeTriggerRequest FromJsonString(std::string_view json,
                                    USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                USERVER_NAMESPACE::chaotic::sax::Parser<V1LikeTriggerRequest>>>(json);
}

bool operator==(const V1LikeTriggerRequest& lhs, const V1LikeTriggerRequest& rhs) {
  return lhs.current_user == rhs.current_user && lhs.idempotency_token == rhs.idempotency_token &&
         lhs.channel_id == rhs.channel_id && lhs.message_id == rhs.message_id && lhs.animation == rhs.animation && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1LikeTriggerRequest::Animation& value) {
  return lh << ToString(value);
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1LikeTriggerRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1LikeTriggerRequest::Animation Parse(USERVER_NAMESPACE::formats::json::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1LikeTriggerRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                           USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1LikeTriggerRequest::Animation Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1LikeTriggerRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                           USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1LikeTriggerRequest::Animation Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

V1LikeTriggerRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                           USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

V1LikeTriggerRequest::Animation Convert(std::string_view value,
                                        USERVER_NAMESPACE::chaotic::convert::To<V1LikeTriggerRequest::Animation>) {
  const auto result = k__ns__V1LikeTriggerRequest__Animation_Mapping.TryFindBySecond(value);
  if (result.has_value()) {
    return *result;
  }
  throw std::runtime_error(
      fmt::format("Invalid enum value ({}) for type ::ns::V1LikeTriggerRequest::Animation", value));
}

std::optional<V1LikeTriggerRequest::Animation> TryConvert(
    std::string_view value, USERVER_NAMESPACE::chaotic::convert::To<V1LikeTriggerRequest::Animation>) noexcept {
  return k__ns__V1LikeTriggerRequest__Animation_Mapping.TryFindBySecond(value);
}

V1LikeTriggerRequest::Animation Parse(std::string_view value,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>) {
  return Convert(value, USERVER_NAMESPACE::chaotic::convert::To<V1LikeTriggerRequest::Animation>{});
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    const V1LikeTriggerRequest::Animation& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  return USERVER_NAMESPACE::formats::json::ValueBuilder(ToString(value)).ExtractValue();
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1LikeTriggerRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["idempotency_token"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<16>,
                                            USERVER_NAMESPACE::chaotic::MaxLength<256>>{value.idempotency_token};

  vb["channel_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.channel_id};

  vb["message_id"] = USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>{value.message_id};

  vb["animation"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1LikeTriggerRequest::Animation>{value.animation};

  return vb.ExtractValue();
}

std::string ToString(V1LikeTriggerRequest::Animation value) {
  const auto result = k__ns__V1LikeTriggerRequest__Animation_Mapping.TryFindByFirst(value);
  if (result.has_value()) {
    return std::string{*result};
  }
  throw std::runtime_error(fmt::format("Invalid enum value: {}", static_cast<int>(value)));
}

V1UserAuthorizationRequest FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserAuthorizationRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                      USERVER_NAMESPACE::chaotic::sax::Parser<V1UserAuthorizationRequest>>>(json);
}

bool operator==(const V1UserAuthorizationRequest& lhs, const V1UserAuthorizationRequest& rhs) {
  return lhs.login == rhs.login && lhs.password == rhs.password && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserAuthorizationRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserAuthorizationRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserAuthorizationRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserAuthorizationRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserAuthorizationRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["login"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.login};

  vb["password"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>{value.password};

  return vb.ExtractValue();
}

V1UserAuthorizationResponse FromJsonString(std::string_view json,
                                           USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserAuthorizationResponse, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                       USERVER_NAMESPACE::chaotic::sax::Parser<V1UserAuthorizationResponse>>>(json);
}

bool operator==(const V1UserAuthorizationResponse& lhs, const V1UserAuthorizationResponse& rhs) {
  return lhs.current_user == rhs.current_user && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserAuthorizationResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserAuthorizationResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserAuthorizationResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserAuthorizationResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserAuthorizationResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  return vb.ExtractValue();
}

V1UserRegistrationRequest FromJsonString(std::string_view json,
                                         USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                     USERVER_NAMESPACE::chaotic::sax::Parser<V1UserRegistrationRequest>>>(json);
}

bool operator==(const V1UserRegistrationRequest& lhs, const V1UserRegistrationRequest& rhs) {
  return lhs.login == rhs.login && lhs.name == rhs.name && lhs.email == rhs.email && lhs.phone == rhs.phone &&
         lhs.password == rhs.password && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserRegistrationRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserRegistrationRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserRegistrationRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserRegistrationRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserRegistrationRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["login"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.login};

  vb["name"] = USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>{value.name};

  vb["email"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.email};

  vb["phone"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.phone};

  vb["password"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>{value.password};

  return vb.ExtractValue();
}

V1UserStatus FromJsonString(std::string_view json, USERVER_NAMESPACE::formats::parse::To<V1UserStatus>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserStatus, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                        USERVER_NAMESPACE::chaotic::sax::Parser<V1UserStatus>>>(json);
}

bool operator==(const V1UserStatus& lhs, const V1UserStatus& rhs) {
  (void)lhs;
  (void)rhs;

  return

      lhs.extra == rhs.extra &&

      true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatus& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserStatus Parse(USERVER_NAMESPACE::formats::json::Value json,
                   USERVER_NAMESPACE::formats::parse::To<V1UserStatus> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserStatus Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                   USERVER_NAMESPACE::formats::parse::To<V1UserStatus> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserStatus Parse(USERVER_NAMESPACE::yaml_config::Value json, USERVER_NAMESPACE::formats::parse::To<V1UserStatus> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserStatus& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = value.extra;

  return vb.ExtractValue();
}

V1UserStatusByLoginRequest FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserStatusByLoginRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                      USERVER_NAMESPACE::chaotic::sax::Parser<V1UserStatusByLoginRequest>>>(json);
}

bool operator==(const V1UserStatusByLoginRequest& lhs, const V1UserStatusByLoginRequest& rhs) {
  return lhs.current_user == rhs.current_user && lhs.login == rhs.login && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusByLoginRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserStatusByLoginRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserStatusByLoginRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserStatusByLoginRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserStatusByLoginRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["login"] =
      USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>{value.login};

  return vb.ExtractValue();
}

V1UserStatusByLoginResponse FromJsonString(std::string_view json,
                                           USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserStatusByLoginResponse, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                       USERVER_NAMESPACE::chaotic::sax::Parser<V1UserStatusByLoginResponse>>>(json);
}

bool operator==(const V1UserStatusByLoginResponse& lhs, const V1UserStatusByLoginResponse& rhs) {
  return lhs.status == rhs.status && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusByLoginResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserStatusByLoginResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserStatusByLoginResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserStatusByLoginResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                  USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserStatusByLoginResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["status"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>{value.status};

  return vb.ExtractValue();
}

V1UserStatusUpdateRequest FromJsonString(std::string_view json,
                                         USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserStatusUpdateRequest, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                     USERVER_NAMESPACE::chaotic::sax::Parser<V1UserStatusUpdateRequest>>>(json);
}

bool operator==(const V1UserStatusUpdateRequest& lhs, const V1UserStatusUpdateRequest& rhs) {
  return lhs.current_user == rhs.current_user && lhs.status == rhs.status && true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusUpdateRequest& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserStatusUpdateRequest Parse(USERVER_NAMESPACE::formats::json::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserStatusUpdateRequest Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserStatusUpdateRequest Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserStatusUpdateRequest& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  vb["current_user"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>{value.current_user};

  vb["status"] = USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>{value.status};

  return vb.ExtractValue();
}

V1UserStatusUpdateResponse FromJsonString(std::string_view json,
                                          USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>) {
  return USERVER_NAMESPACE::formats::json::parser::ParseToType<
      V1UserStatusUpdateResponse, USERVER_NAMESPACE::chaotic::sax::impl::RemoveUserTypeParser<
                                      USERVER_NAMESPACE::chaotic::sax::Parser<V1UserStatusUpdateResponse>>>(json);
}

bool operator==(const V1UserStatusUpdateResponse& lhs, const V1UserStatusUpdateResponse& rhs) {
  (void)lhs;
  (void)rhs;

  return

      true;
}

USERVER_NAMESPACE::logging::LogHelper& operator<<(USERVER_NAMESPACE::logging::LogHelper& lh,
                                                  const V1UserStatusUpdateResponse& value) {
  return lh << ToString(USERVER_NAMESPACE::formats::json::ValueBuilder(value).ExtractValue());
}

V1UserStatusUpdateResponse Parse(USERVER_NAMESPACE::formats::json::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::json::Value>(json, to);
}

V1UserStatusUpdateResponse Parse(USERVER_NAMESPACE::formats::yaml::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse> to) {
  return Parse<USERVER_NAMESPACE::formats::yaml::Value>(json, to);
}

V1UserStatusUpdateResponse Parse(USERVER_NAMESPACE::yaml_config::Value json,
                                 USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse> to) {
  return Parse<USERVER_NAMESPACE::yaml_config::Value>(json, to);
}

USERVER_NAMESPACE::formats::json::Value Serialize(
    [[maybe_unused]] const V1UserStatusUpdateResponse& value,
    USERVER_NAMESPACE::formats::serialize::To<USERVER_NAMESPACE::formats::json::Value>) {
  USERVER_NAMESPACE::formats::json::ValueBuilder vb = USERVER_NAMESPACE::formats::common::Type::kObject;

  return vb.ExtractValue();
}

}  // namespace ns

fmt::format_context::iterator fmt::formatter<ns::V1LikeTriggerRequest::Animation>::format(
    const ns::V1LikeTriggerRequest::Animation& value, fmt::format_context& ctx) const {
  return fmt::format_to(ctx.out(), "{}", ToString(value));
}

