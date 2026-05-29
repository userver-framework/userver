#pragma once

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/array.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "min_messenger.hpp"

namespace ns {

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1CurrentUser_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("token").Case("login").Case("name");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1CurrentUser Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1CurrentUser>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1CurrentUser res;

  res.token =
      value["token"]
          .template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<
              std::string, USERVER_NAMESPACE::chaotic::MinLength<128>, USERVER_NAMESPACE::chaotic::MaxLength<128>>>>();
  res.login =
      value["login"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();
  res.name = value["name"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::string>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1CurrentUser_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelMessage_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("current_user").Case("id").Case("timestamp").Case("message");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelMessage Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessage>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelMessage res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.id = value["id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();
  res.timestamp = value["timestamp"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::string>>();
  res.message =
      value["message"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1ChannelMessage_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelMessageByTimestampRequest_PropertiesNames =
    [](auto selector) {
      return selector().template Type<std::string_view>().Case("channel_id").Case("from").Case("to");
    };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelMessageByTimestampRequest Parse(Value value,
                                         USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelMessageByTimestampRequest res;

  res.channel_id = value["channel_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();
  res.from = value["from"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::string>>();
  res.to = value["to"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value,
                                                             k__ns__V1ChannelMessageByTimestampRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelMessageByTimestampResponse_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("messages"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelMessageByTimestampResponse Parse(Value value,
                                          USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageByTimestampResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelMessageByTimestampResponse res;

  res.messages =
      value["messages"]
          .template As<USERVER_NAMESPACE::chaotic::Array<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1ChannelMessage>,
                                                         std::vector<::ns::V1ChannelMessage>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(
      value, k__ns__V1ChannelMessageByTimestampResponse_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelMessageNewRequest_PropertiesNames =
    [](auto selector) {
      return selector().template Type<std::string_view>().Case("current_user").Case("channel_id").Case("message");
    };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelMessageNewRequest Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelMessageNewRequest res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.channel_id = value["channel_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();
  res.message =
      value["message"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1ChannelMessageNewRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelMessageNewResponse_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("message_id"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelMessageNewResponse Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1ChannelMessageNewResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelMessageNewResponse res;

  res.message_id = value["message_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1ChannelMessageNewResponse_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelNotificationListRequest_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("current_user").Case("channel_id"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelNotificationListRequest Parse(Value value,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelNotificationListRequest res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.channel_id = value["channel_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value,
                                                             k__ns__V1ChannelNotificationListRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelNotificationListResponse_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("notifications"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelNotificationListResponse Parse(Value value,
                                        USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationListResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelNotificationListResponse res;

  res.notifications =
      value["notifications"]
          .template As<USERVER_NAMESPACE::chaotic::Array<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>,
                                                         std::vector<std::int64_t>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value,
                                                             k__ns__V1ChannelNotificationListResponse_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelNotificationNewRequest_PropertiesNames =
    [](auto selector) {
      return selector()
          .template Type<std::string_view>()
          .Case("current_user")
          .Case("channel_id")
          .Case("message_id")
          .Case("other_user_login");
    };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelNotificationNewRequest Parse(Value value,
                                      USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelNotificationNewRequest res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.channel_id = value["channel_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();
  res.message_id = value["message_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();
  res.other_user_login =
      value["other_user_login"]
          .template As<std::optional<
              USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value,
                                                             k__ns__V1ChannelNotificationNewRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1ChannelNotificationNewResponse_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>(); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1ChannelNotificationNewResponse Parse(Value value,
                                       USERVER_NAMESPACE::formats::parse::To<V1ChannelNotificationNewResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1ChannelNotificationNewResponse res;

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value,
                                                             k__ns__V1ChannelNotificationNewResponse_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1Error__Details_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>();
};

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1Error_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("code").Case("message").Case("details");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1Error::Details Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1Error::Details>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1Error::Details res;

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      Parse(std::move(value), USERVER_NAMESPACE::formats::parse::To<USERVER_NAMESPACE::formats::json::Value>()),
      k__ns__V1Error__Details_PropertiesNames);

  return res;
}

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1Error Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1Error>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1Error res;

  res.code = value["code"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::string>>();
  res.message = value["message"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::string>>();
  res.details =
      value["details"].template As<std::optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1Error::Details>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1Error_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1File_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("login").Case("filename").Case("content");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1File Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1File>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1File res;

  res.login =
      value["login"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();
  res.filename =
      value["filename"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>,
                                                             USERVER_NAMESPACE::chaotic::MaxLength<256>>>();
  res.content = value["content"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::string>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1File_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1FileByUriRequest_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("current_user").Case("uri");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1FileByUriRequest Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1FileByUriRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1FileByUriRequest res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.uri =
      value["uri"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1FileByUriRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1FileNewResponse_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>().Case("current_user").Case("uri");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1FileNewResponse Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1FileNewResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1FileNewResponse res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.uri =
      value["uri"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1FileNewResponse_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialBiMap k__ns__V1LikeTriggerRequest__Animation_Mapping =
    [](auto selector) {
      return selector()
          .template Type<V1LikeTriggerRequest::Animation, std::string_view>()
          .Case(V1LikeTriggerRequest::Animation::kLike, "like")
          .Case(V1LikeTriggerRequest::Animation::kDislike, "dislike")
          .Case(V1LikeTriggerRequest::Animation::kHeart, "heart")
          .Case(V1LikeTriggerRequest::Animation::kShit, "shit")
          .Case(V1LikeTriggerRequest::Animation::kOkay, "okay")
          .Case(V1LikeTriggerRequest::Animation::kLol, "LOL")
          .Case(V1LikeTriggerRequest::Animation::kSmile, "smile");
    };

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1LikeTriggerRequest_PropertiesNames = [](auto selector) {
  return selector()
      .template Type<std::string_view>()
      .Case("current_user")
      .Case("idempotency_token")
      .Case("channel_id")
      .Case("message_id")
      .Case("animation");
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1LikeTriggerRequest::Animation Parse(Value val,
                                      USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest::Animation>) {
  const auto value = val.template As<std::string>();
  const auto result = k__ns__V1LikeTriggerRequest__Animation_Mapping.TryFindBySecond(value);
  if (result.has_value()) {
    return *result;
  }
  USERVER_NAMESPACE::chaotic::ThrowForValue(
      fmt::format("Invalid enum value ({}) for type ::ns::V1LikeTriggerRequest::Animation", value), val);
}

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1LikeTriggerRequest Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1LikeTriggerRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1LikeTriggerRequest res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.idempotency_token =
      value["idempotency_token"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<16>,
                                                             USERVER_NAMESPACE::chaotic::MaxLength<256>>>();
  res.channel_id = value["channel_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();
  res.message_id = value["message_id"].template As<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>();
  res.animation =
      value["animation"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1LikeTriggerRequest::Animation>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1LikeTriggerRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserAuthorizationRequest_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("login").Case("password"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserAuthorizationRequest Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserAuthorizationRequest res;

  res.login =
      value["login"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();
  res.password =
      value["password"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1UserAuthorizationRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserAuthorizationResponse_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("current_user"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserAuthorizationResponse Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserAuthorizationResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserAuthorizationResponse res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1UserAuthorizationResponse_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserRegistrationRequest_PropertiesNames =
    [](auto selector) {
      return selector().template Type<std::string_view>().Case("login").Case("name").Case("email").Case("phone").Case(
          "password");
    };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserRegistrationRequest Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserRegistrationRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserRegistrationRequest res;

  res.login =
      value["login"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();
  res.name =
      value["name"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>();
  res.email =
      value["email"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();
  res.phone =
      value["phone"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();
  res.password =
      value["password"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1UserRegistrationRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserStatus_PropertiesNames = [](auto selector) {
  return selector().template Type<std::string_view>();
};

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserStatus Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserStatus>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserStatus res;

  res.extra = USERVER_NAMESPACE::chaotic::ExtractAdditionalPropertiesTrue(
      Parse(std::move(value), USERVER_NAMESPACE::formats::parse::To<USERVER_NAMESPACE::formats::json::Value>()),
      k__ns__V1UserStatus_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserStatusByLoginRequest_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("current_user").Case("login"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserStatusByLoginRequest Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserStatusByLoginRequest res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.login =
      value["login"]
          .template As<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1UserStatusByLoginRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserStatusByLoginResponse_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("status"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserStatusByLoginResponse Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserStatusByLoginResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserStatusByLoginResponse res;

  res.status = value["status"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1UserStatusByLoginResponse_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserStatusUpdateRequest_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>().Case("current_user").Case("status"); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserStatusUpdateRequest Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateRequest>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserStatusUpdateRequest res;

  res.current_user = value["current_user"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>();
  res.status = value["status"].template As<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>>();

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1UserStatusUpdateRequest_PropertiesNames);

  return res;
}

static constexpr USERVER_NAMESPACE::utils::TrivialSet k__ns__V1UserStatusUpdateResponse_PropertiesNames =
    [](auto selector) { return selector().template Type<std::string_view>(); };

template <USERVER_NAMESPACE::formats::common::IsFormatValue Value>
V1UserStatusUpdateResponse Parse(Value value, USERVER_NAMESPACE::formats::parse::To<V1UserStatusUpdateResponse>) {
  value.CheckNotMissing();
  value.CheckObjectOrNull();

  V1UserStatusUpdateResponse res;

  USERVER_NAMESPACE::chaotic::ValidateNoAdditionalProperties(value, k__ns__V1UserStatusUpdateResponse_PropertiesNames);

  return res;
}

}  // namespace ns

