#include <cstddef>

#include <google/protobuf/message.h>
#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

#include <userver/ugrpc/protobuf_visit.hpp>

#include <tests/protobuf.pb.h>

USERVER_NAMESPACE_BEGIN

namespace {

sample::ugrpc::MessageWithDifferentTypes::NestedMessage
ConstructNestedMessage() {
  sample::ugrpc::MessageWithDifferentTypes::NestedMessage message;
  message.set_required_string("string1");
  // Leave required_int as empty required field
  message.set_optional_string("string2");
  // Leave optional_int as empty optional field
  return message;
}

sample::ugrpc::MessageWithDifferentTypes ConstructMessage() {
  sample::ugrpc::MessageWithDifferentTypes message;

  message.set_required_string("string1");
  message.set_optional_string("string2");

  message.set_required_int(123321);
  message.set_optional_int(456654);

  message.mutable_required_nested()->set_required_string("string1");
  // Leave required_int an empty required field
  message.mutable_required_nested()->set_optional_string("string2");
  // Leave optional_int an empty optional field

  // leave optional_nested empty

  // leave recursive messages empty: the recursion should ignore them

  message.add_repeated_primitive("string1");
  message.add_repeated_primitive("string2");

  message.mutable_repeated_message()->Add(ConstructNestedMessage());
  message.mutable_repeated_message()->Add(ConstructNestedMessage());

  message.mutable_primitives_map()->insert({"key1", "value1"});
  message.mutable_primitives_map()->insert({"key2", "value2"});

  message.mutable_nested_map()->insert({"key1", ConstructNestedMessage()});
  message.mutable_nested_map()->insert({"key2", ConstructNestedMessage()});

  // leave oneof_string empty: it should be ignored
  message.set_oneof_int(789987);
  // leave oneof_nested empty: it should be ignored

  message.mutable_google_value()->set_string_value("string");

  return message;
}

}  // namespace

TEST(VisitFields, TestEmptyMessage) {
  std::size_t calls = 0;
  sample::ugrpc::MessageWithDifferentTypes message;
  ugrpc::VisitFields(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });
  ASSERT_EQ(calls, 0);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, sample::ugrpc::MessageWithDifferentTypes()));
}

TEST(VisitFields, TestMessage) {
  std::size_t calls = 0;
  auto message = ConstructMessage();
  ugrpc::VisitFields(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });
  const std::size_t expected_calls = 1 +  // required_string
                                     1 +  // optional_string
                                     1 +  // required_int
                                     1 +  // optional_int
                                     1 +  // required_nested
                                     1 +  // repeated_primitive
                                     1 +  // repeated_message
                                     1 +  // primitives_map
                                     1 +  // nested_map
                                     1 +  // oneof_int
                                     1;   // google_value
  ASSERT_EQ(calls, expected_calls);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, ConstructMessage()));
}

TEST(VisitMessagesRecursive, TestEmptyMessage) {
  std::size_t calls = 0;
  sample::ugrpc::MessageWithDifferentTypes message;
  ugrpc::VisitMessagesRecursive(
      message, [&calls](google::protobuf::Message&) { ++calls; });
  ASSERT_EQ(calls, 1);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, sample::ugrpc::MessageWithDifferentTypes()));
}

TEST(VisitMessagesRecursive, TestMessage) {
  std::size_t calls = 0;
  auto message = ConstructMessage();
  ugrpc::VisitMessagesRecursive(
      message, [&calls](google::protobuf::Message&) { ++calls; });
  const std::size_t expected_calls = 1 +  // root object
                                     1 +  // required_nested
                                     2 +  // repeated_message
                                     2 +  // primitives_map ({ key, value })
                                     2 +  // nested_map ({ key, value })
                                     2 +  // nested_map values
                                     1;   // google_value
  ASSERT_EQ(calls, expected_calls);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, ConstructMessage()));
}

TEST(VisitFieldsRecursive, TestEmptyMessage) {
  std::size_t calls = 0;
  sample::ugrpc::MessageWithDifferentTypes message;
  ugrpc::VisitFieldsRecursive(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });
  ASSERT_EQ(calls, 0);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, sample::ugrpc::MessageWithDifferentTypes()));
}

TEST(VisitFieldsRecursive, TestMessage) {
  std::size_t calls = 0;
  auto message = ConstructMessage();
  ugrpc::VisitFieldsRecursive(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });
  const std::size_t expected_calls = 1 +  // required_string
                                     1 +  // optional_string
                                     1 +  // required_int
                                     1 +  // optional_int
                                     1 +  // required_nested
                                     1 +  // required_nested required_string
                                     1 +  // required_nested optional_string
                                     1 +  // repeated_primitive
                                     1 +  // repeated_message
                                     2 +  // repeated_message required_string
                                     2 +  // repeated_message optional_string
                                     1 +  // primitives_map
                                     2 +  // primitives_map (keys)
                                     2 +  // primitives_map (values)
                                     1 +  // nested_map
                                     2 +  // nested_map (keys)
                                     2 +  // nested_map (values)
                                     2 +  // nested_map (values) required_string
                                     2 +  // nested_map (values) optional_string
                                     1 +  // oneof_int
                                     1 +  // google_value
                                     1;   // google_value actual value
  ASSERT_EQ(calls, expected_calls);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, ConstructMessage()));
}

USERVER_NAMESPACE_END
