#pragma once

#include "min_messenger.hpp"

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
#include <userver/chaotic/sax_parser.hpp>

namespace ns {

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1CurrentUserFieldNametoken = "token";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1CurrentUserFieldNamelogin = "login";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1CurrentUserFieldNamename = "name";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1CurrentUser, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1CurrentUser, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<128>, USERVER_NAMESPACE::chaotic::MaxLength<128>>>, &::ns::V1CurrentUser::token, k_ns_V1CurrentUserFieldNametoken>, USERVER_NAMESPACE::chaotic::Field<::ns::V1CurrentUser, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1CurrentUser::login, k_ns_V1CurrentUserFieldNamelogin>, USERVER_NAMESPACE::chaotic::Field<::ns::V1CurrentUser, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::V1CurrentUser::name, k_ns_V1CurrentUserFieldNamename>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1CurrentUser>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageFieldNameid = "id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageFieldNametimestamp = "timestamp";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageFieldNamemessage = "message";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelMessage, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessage, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1ChannelMessage::current_user, k_ns_V1ChannelMessageFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessage, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1ChannelMessage::id, k_ns_V1ChannelMessageFieldNameid>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessage, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::V1ChannelMessage::timestamp, k_ns_V1ChannelMessageFieldNametimestamp>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessage, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>, &::ns::V1ChannelMessage::message, k_ns_V1ChannelMessageFieldNamemessage>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelMessage>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageByTimestampRequestFieldNamechannel_id = "channel_id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageByTimestampRequestFieldNamefrom = "from";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageByTimestampRequestFieldNameto = "to";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelMessageByTimestampRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageByTimestampRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1ChannelMessageByTimestampRequest::channel_id, k_ns_V1ChannelMessageByTimestampRequestFieldNamechannel_id>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageByTimestampRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::V1ChannelMessageByTimestampRequest::from, k_ns_V1ChannelMessageByTimestampRequestFieldNamefrom>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageByTimestampRequest, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::V1ChannelMessageByTimestampRequest::to, k_ns_V1ChannelMessageByTimestampRequestFieldNameto>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelMessageByTimestampRequest>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageByTimestampResponseFieldNamemessages = "messages";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelMessageByTimestampResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageByTimestampResponse, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Array<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1ChannelMessage>, std::vector<::ns::V1ChannelMessage>>>, &::ns::V1ChannelMessageByTimestampResponse::messages, k_ns_V1ChannelMessageByTimestampResponseFieldNamemessages>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelMessageByTimestampResponse>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageNewRequestFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageNewRequestFieldNamechannel_id = "channel_id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageNewRequestFieldNamemessage = "message";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelMessageNewRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageNewRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1ChannelMessageNewRequest::current_user, k_ns_V1ChannelMessageNewRequestFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageNewRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1ChannelMessageNewRequest::channel_id, k_ns_V1ChannelMessageNewRequestFieldNamechannel_id>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageNewRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>, &::ns::V1ChannelMessageNewRequest::message, k_ns_V1ChannelMessageNewRequestFieldNamemessage>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelMessageNewRequest>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelMessageNewResponseFieldNamemessage_id = "message_id";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelMessageNewResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessageNewResponse, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1ChannelMessageNewResponse::message_id, k_ns_V1ChannelMessageNewResponseFieldNamemessage_id>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelMessageNewResponse>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelNotificationListRequestFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelNotificationListRequestFieldNamechannel_id = "channel_id";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelNotificationListRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationListRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1ChannelNotificationListRequest::current_user, k_ns_V1ChannelNotificationListRequestFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationListRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1ChannelNotificationListRequest::channel_id, k_ns_V1ChannelNotificationListRequestFieldNamechannel_id>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelNotificationListRequest>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelNotificationListResponseFieldNamenotifications = "notifications";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelNotificationListResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationListResponse, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Array<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>, std::vector<std::int64_t>>>, &::ns::V1ChannelNotificationListResponse::notifications, k_ns_V1ChannelNotificationListResponseFieldNamenotifications>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelNotificationListResponse>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelNotificationNewRequestFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelNotificationNewRequestFieldNamechannel_id = "channel_id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelNotificationNewRequestFieldNamemessage_id = "message_id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ChannelNotificationNewRequestFieldNameother_user_login = "other_user_login";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelNotificationNewRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationNewRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1ChannelNotificationNewRequest::current_user, k_ns_V1ChannelNotificationNewRequestFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationNewRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1ChannelNotificationNewRequest::channel_id, k_ns_V1ChannelNotificationNewRequestFieldNamechannel_id>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationNewRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1ChannelNotificationNewRequest::message_id, k_ns_V1ChannelNotificationNewRequestFieldNamemessage_id>, USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationNewRequest, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1ChannelNotificationNewRequest::other_user_login, k_ns_V1ChannelNotificationNewRequestFieldNameother_user_login>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelNotificationNewRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1ChannelNotificationNewResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1ChannelNotificationNewResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1Error::Details, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1Error::Details>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ErrorFieldNamecode = "code";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ErrorFieldNamemessage = "message";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1ErrorFieldNamedetails = "details";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1Error, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1Error, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::V1Error::code, k_ns_V1ErrorFieldNamecode>, USERVER_NAMESPACE::chaotic::Field<::ns::V1Error, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::V1Error::message, k_ns_V1ErrorFieldNamemessage>, USERVER_NAMESPACE::chaotic::Field<::ns::V1Error, USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1Error::Details>>, &::ns::V1Error::details, k_ns_V1ErrorFieldNamedetails>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1Error>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1FileFieldNamelogin = "login";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1FileFieldNamefilename = "filename";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1FileFieldNamecontent = "content";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1File, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1File, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1File::login, k_ns_V1FileFieldNamelogin>, USERVER_NAMESPACE::chaotic::Field<::ns::V1File, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>, USERVER_NAMESPACE::chaotic::MaxLength<256>>>, &::ns::V1File::filename, k_ns_V1FileFieldNamefilename>, USERVER_NAMESPACE::chaotic::Field<::ns::V1File, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>, &::ns::V1File::content, k_ns_V1FileFieldNamecontent>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1File>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1FileByUriRequestFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1FileByUriRequestFieldNameuri = "uri";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1FileByUriRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1FileByUriRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1FileByUriRequest::current_user, k_ns_V1FileByUriRequestFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1FileByUriRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1FileByUriRequest::uri, k_ns_V1FileByUriRequestFieldNameuri>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1FileByUriRequest>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1FileNewResponseFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1FileNewResponseFieldNameuri = "uri";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1FileNewResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1FileNewResponse, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1FileNewResponse::current_user, k_ns_V1FileNewResponseFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1FileNewResponse, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1FileNewResponse::uri, k_ns_V1FileNewResponseFieldNameuri>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1FileNewResponse>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1LikeTriggerRequestFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1LikeTriggerRequestFieldNameidempotency_token = "idempotency_token";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1LikeTriggerRequestFieldNamechannel_id = "channel_id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1LikeTriggerRequestFieldNamemessage_id = "message_id";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1LikeTriggerRequestFieldNameanimation = "animation";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1LikeTriggerRequest::current_user, k_ns_V1LikeTriggerRequestFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<16>, USERVER_NAMESPACE::chaotic::MaxLength<256>>>, &::ns::V1LikeTriggerRequest::idempotency_token, k_ns_V1LikeTriggerRequestFieldNameidempotency_token>, USERVER_NAMESPACE::chaotic::Field<::ns::V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1LikeTriggerRequest::channel_id, k_ns_V1LikeTriggerRequestFieldNamechannel_id>, USERVER_NAMESPACE::chaotic::Field<::ns::V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>, &::ns::V1LikeTriggerRequest::message_id, k_ns_V1LikeTriggerRequestFieldNamemessage_id>, USERVER_NAMESPACE::chaotic::Field<::ns::V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1LikeTriggerRequest::Animation>>, &::ns::V1LikeTriggerRequest::animation, k_ns_V1LikeTriggerRequestFieldNameanimation>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1LikeTriggerRequest>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserAuthorizationRequestFieldNamelogin = "login";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserAuthorizationRequestFieldNamepassword = "password";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserAuthorizationRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserAuthorizationRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1UserAuthorizationRequest::login, k_ns_V1UserAuthorizationRequestFieldNamelogin>, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserAuthorizationRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>>, &::ns::V1UserAuthorizationRequest::password, k_ns_V1UserAuthorizationRequestFieldNamepassword>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserAuthorizationRequest>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserAuthorizationResponseFieldNamecurrent_user = "current_user";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserAuthorizationResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserAuthorizationResponse, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1UserAuthorizationResponse::current_user, k_ns_V1UserAuthorizationResponseFieldNamecurrent_user>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserAuthorizationResponse>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserRegistrationRequestFieldNamelogin = "login";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserRegistrationRequestFieldNamename = "name";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserRegistrationRequestFieldNameemail = "email";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserRegistrationRequestFieldNamephone = "phone";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserRegistrationRequestFieldNamepassword = "password";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1UserRegistrationRequest::login, k_ns_V1UserRegistrationRequestFieldNamelogin>, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>, &::ns::V1UserRegistrationRequest::name, k_ns_V1UserRegistrationRequestFieldNamename>, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1UserRegistrationRequest::email, k_ns_V1UserRegistrationRequestFieldNameemail>, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1UserRegistrationRequest::phone, k_ns_V1UserRegistrationRequestFieldNamephone>, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>>, &::ns::V1UserRegistrationRequest::password, k_ns_V1UserRegistrationRequestFieldNamepassword>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserRegistrationRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserStatus, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserStatus>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserStatusByLoginRequestFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserStatusByLoginRequestFieldNamelogin = "login";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserStatusByLoginRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserStatusByLoginRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1UserStatusByLoginRequest::current_user, k_ns_V1UserStatusByLoginRequestFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserStatusByLoginRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>, &::ns::V1UserStatusByLoginRequest::login, k_ns_V1UserStatusByLoginRequestFieldNamelogin>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserStatusByLoginRequest>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserStatusByLoginResponseFieldNamestatus = "status";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserStatusByLoginResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserStatusByLoginResponse, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>>, &::ns::V1UserStatusByLoginResponse::status, k_ns_V1UserStatusByLoginResponseFieldNamestatus>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserStatusByLoginResponse>);

constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserStatusUpdateRequestFieldNamecurrent_user = "current_user";
constexpr inline USERVER_NAMESPACE::utils::StringLiteral k_ns_V1UserStatusUpdateRequestFieldNamestatus = "status";

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserStatusUpdateRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserStatusUpdateRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>, &::ns::V1UserStatusUpdateRequest::current_user, k_ns_V1UserStatusUpdateRequestFieldNamecurrent_user>, USERVER_NAMESPACE::chaotic::Field<::ns::V1UserStatusUpdateRequest, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>>, &::ns::V1UserStatusUpdateRequest::status, k_ns_V1UserStatusUpdateRequestFieldNamestatus>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserStatusUpdateRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<::ns::V1UserStatusUpdateResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<V1UserStatusUpdateResponse>);

}  // namespace ns

