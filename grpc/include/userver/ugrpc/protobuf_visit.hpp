#pragma once

/// @file userver/ugrpc/protobuf_visit.hpp
/// @brief Utilities for visiting the fields of protobufs

#include <userver/utils/function_ref.hpp>

namespace google::protobuf {

class Message;
class FieldDescriptor;

}  // namespace google::protobuf

USERVER_NAMESPACE_BEGIN

namespace ugrpc {

using MessageVisitor = utils::function_ref<void(google::protobuf::Message&)>;

using FieldVisitor = utils::function_ref<void(
    google::protobuf::Message&, const google::protobuf::FieldDescriptor&)>;

/// @brief Execute a callback for all non-empty fields of the message.
void VisitFields(google::protobuf::Message& message, FieldVisitor callback);

/// @brief Execute a callback for the message and its non-empty submessages.
void VisitMessagesRecursive(google::protobuf::Message& message,
                            MessageVisitor callback);

/// @brief Execute a callback for all fields
/// of the message and its non-empty submessages.
void VisitFieldsRecursive(google::protobuf::Message& message,
                          FieldVisitor callback);

}  // namespace ugrpc

USERVER_NAMESPACE_END
