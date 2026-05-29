#pragma once

#include <userver/chaotic/additional_properties.hpp>
#include <userver/chaotic/array.hpp>
#include <userver/chaotic/exception.hpp>
#include <userver/chaotic/primitive.hpp>
#include <userver/chaotic/sax_parser.hpp>
#include <userver/chaotic/validators.hpp>
#include <userver/chaotic/with_type.hpp>
#include <userver/formats/common/meta.hpp>
#include <userver/formats/parse/common_containers.hpp>
#include <userver/formats/serialize/common_containers.hpp>
#include <userver/utils/trivial_map.hpp>

#include "min_messenger.hpp"

namespace ns {

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1CurrentUser, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1CurrentUser,
        USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<
            std::string, USERVER_NAMESPACE::chaotic::MinLength<128>, USERVER_NAMESPACE::chaotic::MaxLength<128>>>,
        &::ns::V1CurrentUser::token, ::ns::V1CurrentUser::kFieldNametoken>,
    USERVER_NAMESPACE::chaotic::Field<::ns::V1CurrentUser,
                                      USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<
                                          std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
                                      &::ns::V1CurrentUser::login, ::ns::V1CurrentUser::kFieldNamelogin>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1CurrentUser, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::V1CurrentUser::name, ::ns::V1CurrentUser::kFieldNamename>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1CurrentUser>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelMessage, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessage,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1ChannelMessage::current_user, ::ns::V1ChannelMessage::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessage,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1ChannelMessage::id, ::ns::V1ChannelMessage::kFieldNameid>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessage,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::V1ChannelMessage::timestamp, ::ns::V1ChannelMessage::kFieldNametimestamp>,
    USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelMessage,
                                      USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<
                                          std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>,
                                      &::ns::V1ChannelMessage::message, ::ns::V1ChannelMessage::kFieldNamemessage>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelMessage>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelMessageByTimestampRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageByTimestampRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1ChannelMessageByTimestampRequest::channel_id,
        ::ns::V1ChannelMessageByTimestampRequest::kFieldNamechannel_id>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageByTimestampRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::V1ChannelMessageByTimestampRequest::from, ::ns::V1ChannelMessageByTimestampRequest::kFieldNamefrom>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageByTimestampRequest,
        USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::V1ChannelMessageByTimestampRequest::to, ::ns::V1ChannelMessageByTimestampRequest::kFieldNameto>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelMessageByTimestampRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelMessageByTimestampResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageByTimestampResponse,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Array<
            USERVER_NAMESPACE::chaotic::Primitive<::ns::V1ChannelMessage>, std::vector<::ns::V1ChannelMessage>>>,
        &::ns::V1ChannelMessageByTimestampResponse::messages,
        ::ns::V1ChannelMessageByTimestampResponse::kFieldNamemessages>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelMessageByTimestampResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelMessageNewRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageNewRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1ChannelMessageNewRequest::current_user, ::ns::V1ChannelMessageNewRequest::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageNewRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1ChannelMessageNewRequest::channel_id, ::ns::V1ChannelMessageNewRequest::kFieldNamechannel_id>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageNewRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>,
        &::ns::V1ChannelMessageNewRequest::message, ::ns::V1ChannelMessageNewRequest::kFieldNamemessage>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelMessageNewRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelMessageNewResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelMessageNewResponse,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1ChannelMessageNewResponse::message_id, ::ns::V1ChannelMessageNewResponse::kFieldNamemessage_id>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelMessageNewResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelNotificationListRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelNotificationListRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1ChannelNotificationListRequest::current_user,
        ::ns::V1ChannelNotificationListRequest::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelNotificationListRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1ChannelNotificationListRequest::channel_id,
        ::ns::V1ChannelNotificationListRequest::kFieldNamechannel_id>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelNotificationListRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelNotificationListResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelNotificationListResponse,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Array<
            USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>, std::vector<std::int64_t>>>,
        &::ns::V1ChannelNotificationListResponse::notifications,
        ::ns::V1ChannelNotificationListResponse::kFieldNamenotifications>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelNotificationListResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelNotificationNewRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelNotificationNewRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1ChannelNotificationNewRequest::current_user,
        ::ns::V1ChannelNotificationNewRequest::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelNotificationNewRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1ChannelNotificationNewRequest::channel_id,
        ::ns::V1ChannelNotificationNewRequest::kFieldNamechannel_id>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1ChannelNotificationNewRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1ChannelNotificationNewRequest::message_id,
        ::ns::V1ChannelNotificationNewRequest::kFieldNamemessage_id>,
    USERVER_NAMESPACE::chaotic::Field<::ns::V1ChannelNotificationNewRequest,
                                      USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<
                                          std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
                                      &::ns::V1ChannelNotificationNewRequest::other_user_login,
                                      ::ns::V1ChannelNotificationNewRequest::kFieldNameother_user_login>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelNotificationNewRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1ChannelNotificationNewResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1ChannelNotificationNewResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<
    USERVER_NAMESPACE::chaotic::Object<::ns::V1Error::Details, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1Error::Details>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1Error, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1Error, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::V1Error::code, ::ns::V1Error::kFieldNamecode>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1Error, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::V1Error::message, ::ns::V1Error::kFieldNamemessage>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1Error,
        USERVER_NAMESPACE::chaotic::Optional<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1Error::Details>>,
        &::ns::V1Error::details, ::ns::V1Error::kFieldNamedetails>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1Error>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1File, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<::ns::V1File,
                                      USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<
                                          std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
                                      &::ns::V1File::login, ::ns::V1File::kFieldNamelogin>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1File,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<
            std::string, USERVER_NAMESPACE::chaotic::MinLength<3>, USERVER_NAMESPACE::chaotic::MaxLength<256>>>,
        &::ns::V1File::filename, ::ns::V1File::kFieldNamefilename>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1File, USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::string>>,
        &::ns::V1File::content, ::ns::V1File::kFieldNamecontent>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1File>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1FileByUriRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1FileByUriRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1FileByUriRequest::current_user, ::ns::V1FileByUriRequest::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<::ns::V1FileByUriRequest,
                                      USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<
                                          std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
                                      &::ns::V1FileByUriRequest::uri, ::ns::V1FileByUriRequest::kFieldNameuri>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1FileByUriRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1FileNewResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1FileNewResponse,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1FileNewResponse::current_user, ::ns::V1FileNewResponse::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<::ns::V1FileNewResponse,
                                      USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<
                                          std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
                                      &::ns::V1FileNewResponse::uri, ::ns::V1FileNewResponse::kFieldNameuri>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1FileNewResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1LikeTriggerRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1LikeTriggerRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1LikeTriggerRequest::current_user, ::ns::V1LikeTriggerRequest::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1LikeTriggerRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<
            std::string, USERVER_NAMESPACE::chaotic::MinLength<16>, USERVER_NAMESPACE::chaotic::MaxLength<256>>>,
        &::ns::V1LikeTriggerRequest::idempotency_token, ::ns::V1LikeTriggerRequest::kFieldNameidempotency_token>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1LikeTriggerRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1LikeTriggerRequest::channel_id, ::ns::V1LikeTriggerRequest::kFieldNamechannel_id>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1LikeTriggerRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<std::int64_t>>,
        &::ns::V1LikeTriggerRequest::message_id, ::ns::V1LikeTriggerRequest::kFieldNamemessage_id>,
    USERVER_NAMESPACE::chaotic::Field<::ns::V1LikeTriggerRequest,
                                      USERVER_NAMESPACE::chaotic::Required<
                                          USERVER_NAMESPACE::chaotic::Primitive<::ns::V1LikeTriggerRequest::Animation>>,
                                      &::ns::V1LikeTriggerRequest::animation,
                                      ::ns::V1LikeTriggerRequest::kFieldNameanimation>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1LikeTriggerRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1UserAuthorizationRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserAuthorizationRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
        &::ns::V1UserAuthorizationRequest::login, ::ns::V1UserAuthorizationRequest::kFieldNamelogin>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserAuthorizationRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>>,
        &::ns::V1UserAuthorizationRequest::password, ::ns::V1UserAuthorizationRequest::kFieldNamepassword>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserAuthorizationRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1UserAuthorizationResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserAuthorizationResponse,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1UserAuthorizationResponse::current_user, ::ns::V1UserAuthorizationResponse::kFieldNamecurrent_user>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserAuthorizationResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1UserRegistrationRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserRegistrationRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
        &::ns::V1UserRegistrationRequest::login, ::ns::V1UserRegistrationRequest::kFieldNamelogin>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserRegistrationRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<1>>>,
        &::ns::V1UserRegistrationRequest::name, ::ns::V1UserRegistrationRequest::kFieldNamename>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserRegistrationRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
        &::ns::V1UserRegistrationRequest::email, ::ns::V1UserRegistrationRequest::kFieldNameemail>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserRegistrationRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
        &::ns::V1UserRegistrationRequest::phone, ::ns::V1UserRegistrationRequest::kFieldNamephone>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserRegistrationRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<6>>>,
        &::ns::V1UserRegistrationRequest::password, ::ns::V1UserRegistrationRequest::kFieldNamepassword>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserRegistrationRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<
    USERVER_NAMESPACE::chaotic::Object<::ns::V1UserStatus, USERVER_NAMESPACE::chaotic::UnknownFields::StoreJson>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserStatus>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1UserStatusByLoginRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserStatusByLoginRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1UserStatusByLoginRequest::current_user, ::ns::V1UserStatusByLoginRequest::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserStatusByLoginRequest,
        USERVER_NAMESPACE::chaotic::Required<
            USERVER_NAMESPACE::chaotic::Primitive<std::string, USERVER_NAMESPACE::chaotic::MinLength<3>>>,
        &::ns::V1UserStatusByLoginRequest::login, ::ns::V1UserStatusByLoginRequest::kFieldNamelogin>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserStatusByLoginRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1UserStatusByLoginResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserStatusByLoginResponse,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>>,
        &::ns::V1UserStatusByLoginResponse::status, ::ns::V1UserStatusByLoginResponse::kFieldNamestatus>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserStatusByLoginResponse>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1UserStatusUpdateRequest, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserStatusUpdateRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1CurrentUser>>,
        &::ns::V1UserStatusUpdateRequest::current_user, ::ns::V1UserStatusUpdateRequest::kFieldNamecurrent_user>,
    USERVER_NAMESPACE::chaotic::Field<
        ::ns::V1UserStatusUpdateRequest,
        USERVER_NAMESPACE::chaotic::Required<USERVER_NAMESPACE::chaotic::Primitive<::ns::V1UserStatus>>,
        &::ns::V1UserStatusUpdateRequest::status, ::ns::V1UserStatusUpdateRequest::kFieldNamestatus>>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserStatusUpdateRequest>);

[[maybe_unused]] USERVER_NAMESPACE::chaotic::sax::Parser<USERVER_NAMESPACE::chaotic::Object<
    ::ns::V1UserStatusUpdateResponse, USERVER_NAMESPACE::chaotic::UnknownFields::Forbid>>
    ParserOf(USERVER_NAMESPACE::chaotic::sax::Type<::ns::V1UserStatusUpdateResponse>);

}  // namespace ns

