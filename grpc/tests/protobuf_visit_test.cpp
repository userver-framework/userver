#include <cstddef>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

#include <userver/ugrpc/protobuf_visit.hpp>

#include <tests/protobuf.grpc.pb.h>

USERVER_NAMESPACE_BEGIN

namespace {

sample::ugrpc::MessageWithDifferentTypes::NestedMessage
ConstructNestedMessage() {
  sample::ugrpc::MessageWithDifferentTypes::NestedMessage message;
  message.set_required_string("string1");
  message.set_optional_string("string2");
  // Leave required_int as empty required field
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
  message.mutable_required_nested()->set_optional_string("string2");
  // Leave required_int an empty required field
  // Leave optional_int an empty optional field

  // leave optional_nested empty

  message.mutable_required_recursive()->set_required_string("string1");
  message.mutable_required_recursive()->set_optional_string("string2");

  // leave optional_recursive empty

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

std::pair<const google::protobuf::Descriptor*,
          ugrpc::FieldsVisitor::FieldDescriptorSet>
MakeDependency(std::string_view message, std::vector<std::string_view> fields) {
  std::unordered_set<const google::protobuf::FieldDescriptor*> field_desc;
  for (const std::string_view field : fields) {
    field_desc.insert(
        ugrpc::FindField(ugrpc::FindGeneratedMessage(message), field));
  }
  return {ugrpc::FindGeneratedMessage(message), std::move(field_desc)};
}

std::pair<const google::protobuf::Descriptor*,
          ugrpc::FieldsVisitor::FieldDescriptorSet>
MakeDependency(std::string_view message, std::string_view fields_message,
               std::vector<std::string_view> fields) {
  std::unordered_set<const google::protobuf::FieldDescriptor*> field_desc;
  for (const std::string_view field : fields) {
    field_desc.insert(
        ugrpc::FindField(ugrpc::FindGeneratedMessage(fields_message), field));
  }
  return {ugrpc::FindGeneratedMessage(message), std::move(field_desc)};
}

std::unordered_map<std::string, std::unordered_set<std::string>> ToStrings(
    const ugrpc::FieldsVisitor::Dependencies& dependencies) {
  std::unordered_map<std::string, std::unordered_set<std::string>> result;
  for (const auto& [msg, fields] : dependencies) {
    for (const google::protobuf::FieldDescriptor* field : fields) {
      result[msg->full_name()].insert(field->name());
    }
  }
  return result;
}

std::unordered_set<std::string> ToStrings(
    const ugrpc::FieldsVisitor::DescriptorSet& messages) {
  std::unordered_set<std::string> result;
  for (const auto& msg : messages) {
    result.insert(msg->full_name());
  }
  return result;
}

std::unordered_set<std::string> ToStrings(
    const ugrpc::FieldsVisitor::FieldDescriptorSet& fields) {
  std::unordered_set<std::string> result;
  for (const auto& field : fields) {
    result.insert(field->name());
  }
  return result;
}

template <typename T>
std::unordered_set<T> ToSet(const std::vector<T>& vector) {
  return {vector.begin(), vector.end()};
}

void MyExpectEq(const ugrpc::FieldsVisitor::FieldDescriptorSet& val1,
                const ugrpc::FieldsVisitor::FieldDescriptorSet& val2) {
  EXPECT_EQ(ToStrings(val1), ToStrings(val2));
  EXPECT_EQ(val1, val2);
}

void MyExpectEq(const ugrpc::FieldsVisitor::DescriptorSet& val1,
                const ugrpc::FieldsVisitor::DescriptorSet& val2) {
  EXPECT_EQ(ToStrings(val1), ToStrings(val2));
  EXPECT_EQ(val1, val2);
}

void MyExpectEq(const ugrpc::FieldsVisitor::Dependencies& val1,
                const ugrpc::FieldsVisitor::Dependencies& val2) {
  EXPECT_EQ(ToStrings(val1), ToStrings(val2));
  EXPECT_EQ(val1, val2);
}

template <typename MapOrSet>
bool ContainsMessage(const MapOrSet& collection, std::string_view value) {
  return collection.find(ugrpc::FindGeneratedMessage(value)) !=
         collection.end();
}

bool FieldSelector(const google::protobuf::Descriptor&,
                   const google::protobuf::FieldDescriptor& field) {
  return field.options().GetExtension(sample::ugrpc::field).selected();
}

bool MessageSelector(const google::protobuf::Descriptor& message) {
  return message.options().GetExtension(sample::ugrpc::message).selected();
}

namespace component1 {

ugrpc::DescriptorList Get() {
  return {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1A"),
          ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1B"),
          ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1C"),
          ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1D"),
          ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1E")};
}

ugrpc::FieldsVisitor::Dependencies GetSelectedFields() {
  return {
      MakeDependency("sample.ugrpc.Msg1A", {"value1", "value2"}),
      MakeDependency("sample.ugrpc.Msg1C", {"value"}),
      MakeDependency("sample.ugrpc.Msg1D", {"value"}),
  };
}

ugrpc::FieldsVisitor::DescriptorSet GetSelectedMessages() {
  return {
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1A"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1C"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1D"),
  };
}

ugrpc::FieldsVisitor::Dependencies GetFieldsWithSelectedChildren() {
  return {
      MakeDependency("sample.ugrpc.Msg1A", {"nested"}),
      MakeDependency("sample.ugrpc.Msg1B",
                     {"recursive_1", "recursive_2", "nested_secret_1",
                      "nested_secret_2", "nested_secret_3"}),
      MakeDependency("sample.ugrpc.Msg1C",
                     {"recursive_1", "recursive_2", "nested"}),
  };
}

}  // namespace component1

namespace component2 {

ugrpc::DescriptorList Get() {
  return {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg2A")};
}

ugrpc::FieldsVisitor::Dependencies GetSelectedFields() {
  return {
      MakeDependency("sample.ugrpc.Msg2A", {"value1", "value3"}),
  };
}

ugrpc::FieldsVisitor::DescriptorSet GetSelectedMessages() {
  return {
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg2A"),
  };
}

ugrpc::FieldsVisitor::Dependencies GetFieldsWithSelectedChildren() {
  return {};
}

}  // namespace component2

namespace component3 {

ugrpc::DescriptorList Get() {
  return {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg3A"),
          ugrpc::FindGeneratedMessage("sample.ugrpc.Msg3B")};
}

ugrpc::FieldsVisitor::Dependencies GetSelectedFields() { return {}; }

ugrpc::FieldsVisitor::DescriptorSet GetSelectedMessages() { return {}; }

ugrpc::FieldsVisitor::Dependencies GetFieldsWithSelectedChildren() {
  return {};
}

}  // namespace component3

namespace component4 {

ugrpc::DescriptorList Get() {
  return {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
          ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
          ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")};
}

ugrpc::FieldsVisitor::Dependencies GetSelectedFields() {
  return {
      MakeDependency("sample.ugrpc.Msg4B", {"value"}),
  };
}

ugrpc::FieldsVisitor::DescriptorSet GetSelectedMessages() {
  return {
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
  };
}

ugrpc::FieldsVisitor::Dependencies GetFieldsWithSelectedChildren() {
  return {
      MakeDependency("sample.ugrpc.Msg4A", {"nested"}),
      MakeDependency("sample.ugrpc.Msg4B", {"nested"}),
      MakeDependency("sample.ugrpc.Msg4C", {"nested_1", "nested_2"}),
  };
}

}  // namespace component4

namespace diff_types {

ugrpc::DescriptorList Get() {
  return {ugrpc::FindGeneratedMessage("sample.ugrpc.MessageWithDifferentTypes"),
          ugrpc::FindGeneratedMessage(
              "sample.ugrpc.MessageWithDifferentTypes.NestedMessage"),
          ugrpc::FindGeneratedMessage(
              "sample.ugrpc.MessageWithDifferentTypes.NestedMapEntry"),
          ugrpc::FindGeneratedMessage(
              "sample.ugrpc.MessageWithDifferentTypes.PrimitivesMapEntry"),
          ugrpc::FindGeneratedMessage("google.protobuf.Value"),
          ugrpc::FindGeneratedMessage("google.protobuf.ListValue")};
}

ugrpc::FieldsVisitor::Dependencies GetSelectedFields() {
  return {
      MakeDependency("sample.ugrpc.MessageWithDifferentTypes",
                     {"optional_string", "optional_int", "repeated_message"}),
      MakeDependency("sample.ugrpc.MessageWithDifferentTypes.NestedMessage",
                     {"required_string", "required_int"}),
  };
}

ugrpc::FieldsVisitor::DescriptorSet GetSelectedMessages() {
  return {
      ugrpc::FindGeneratedMessage(
          "sample.ugrpc.MessageWithDifferentTypes.NestedMessage"),
  };
}

ugrpc::FieldsVisitor::Dependencies GetFieldsWithSelectedChildren() {
  return {
      MakeDependency(
          "sample.ugrpc.MessageWithDifferentTypes",
          {"required_nested", "optional_nested", "required_recursive",
           "optional_recursive", "repeated_message", "nested_map",
           "oneof_nested", "weird_map"}),
      MakeDependency("sample.ugrpc.MessageWithDifferentTypes.NestedMapEntry",
                     {"value"}),
      MakeDependency("sample.ugrpc.MessageWithDifferentTypes.WeirdMapEntry",
                     {"value"})};
}

}  // namespace diff_types

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
                                     1 +  // required_recursive
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
                                     1 +  // required_recursive
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
                                     1 +  // required_recursive
                                     1 +  // required_recursive required_string
                                     1 +  // required_recursive optional_string
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

TEST(GetFieldDescriptors, MessageWithDifferentTypes) {
  constexpr auto msg = "sample.ugrpc.MessageWithDifferentTypes";
  MyExpectEq(
      ToSet(ugrpc::GetFieldDescriptors(*ugrpc::FindGeneratedMessage(msg))),
      {
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "required_string"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "optional_string"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "required_int"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "optional_int"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "required_nested"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "optional_nested"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg),
                           "required_recursive"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg),
                           "optional_recursive"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg),
                           "repeated_primitive"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg),
                           "repeated_message"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "primitives_map"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "nested_map"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "oneof_string"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "oneof_int"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "oneof_nested"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "google_value"),
          ugrpc::FindField(ugrpc::FindGeneratedMessage(msg), "weird_map"),
      });
}

TEST(GetNestedMessageDescriptors, MessageWithDifferentTypes) {
  const std::string msg = "sample.ugrpc.MessageWithDifferentTypes";
  MyExpectEq(
      ToSet(ugrpc::GetNestedMessageDescriptors(
          *ugrpc::FindGeneratedMessage(msg))),
      {
          ugrpc::FindGeneratedMessage(msg),
          ugrpc::FindGeneratedMessage(msg + ".NestedMessage"),
          ugrpc::FindGeneratedMessage(msg + ".NestedMapEntry"),
          ugrpc::FindGeneratedMessage(msg + ".PrimitivesMapEntry"),
          ugrpc::FindGeneratedMessage(msg + ".WeirdMapEntry"),
          ugrpc::FindGeneratedMessage("google.protobuf.Value"),
          ugrpc::FindGeneratedMessage("google.protobuf.Struct"),
          ugrpc::FindGeneratedMessage("google.protobuf.Struct.FieldsEntry"),
          ugrpc::FindGeneratedMessage("google.protobuf.ListValue"),
      });
}

TEST(FieldsVisitorCompile, OneLeafNoSelected) {
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated("sample.ugrpc.Msg3B");
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()), {});
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             {});
  MyExpectEq(visitor.GetReverseEdges(utils::impl::InternalTag()), {});
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()), {});
  MyExpectEq(visitor.GetCompiled(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg3B")});
}

TEST(FieldsVisitorCompile, OneNonLeafNoSelected) {
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated("sample.ugrpc.Msg3A");
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()), {});
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             {});
  MyExpectEq(
      visitor.GetReverseEdges(utils::impl::InternalTag()),
      {MakeDependency("sample.ugrpc.Msg3B", "sample.ugrpc.Msg3A", {"nested"})});
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()), {});
  MyExpectEq(visitor.GetCompiled(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg3A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg3B")});
}

TEST(FieldsVisitorCompile, OneLeafSelected) {
  constexpr auto msg = "sample.ugrpc.MessageWithDifferentTypes.NestedMessage";
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated(msg);
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             {MakeDependency(msg, {"required_string", "required_int"})});
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             {});
  MyExpectEq(visitor.GetReverseEdges(utils::impl::InternalTag()), {});
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage(msg)});
  MyExpectEq(visitor.GetCompiled(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage(msg)});
}

TEST(FieldsVisitorCompile, OneNonLeafSelected) {
  const std::string msg = "sample.ugrpc.MessageWithDifferentTypes";
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated(msg);
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             diff_types::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());
  EXPECT_GT(visitor.GetReverseEdges(utils::impl::InternalTag()).size(), 3);
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage(msg),
              ugrpc::FindGeneratedMessage(msg + ".NestedMessage"),
              ugrpc::FindGeneratedMessage(msg + ".NestedMapEntry"),
              ugrpc::FindGeneratedMessage(msg + ".WeirdMapEntry")});
  EXPECT_GT(visitor.GetCompiled(utils::impl::InternalTag()).size(), 7);
}

TEST(FieldsVisitorCompile, OneLoop) {
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated("sample.ugrpc.Msg4A");
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component4::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component4::GetFieldsWithSelectedChildren());
  EXPECT_EQ(visitor.GetReverseEdges(utils::impl::InternalTag()).size(), 3);
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
  MyExpectEq(visitor.GetCompiled(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
}

TEST(FieldsVisitorCompile, TwoAB) {
  const std::string msg = "sample.ugrpc.MessageWithDifferentTypes";
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated(msg);
  visitor.CompileGenerated(msg + ".NestedMessage");
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             diff_types::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());
  EXPECT_GT(visitor.GetReverseEdges(utils::impl::InternalTag()).size(), 3);
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage(msg),
              ugrpc::FindGeneratedMessage(msg + ".NestedMessage"),
              ugrpc::FindGeneratedMessage(msg + ".NestedMapEntry"),
              ugrpc::FindGeneratedMessage(msg + ".WeirdMapEntry")});
  EXPECT_GT(visitor.GetCompiled(utils::impl::InternalTag()).size(), 7);
}

TEST(FieldsVisitorCompile, TwoBA) {
  const std::string msg = "sample.ugrpc.MessageWithDifferentTypes";
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated(msg + ".NestedMessage");
  visitor.CompileGenerated(msg);
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             diff_types::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());
  EXPECT_GT(visitor.GetReverseEdges(utils::impl::InternalTag()).size(), 3);
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage(msg),
              ugrpc::FindGeneratedMessage(msg + ".NestedMessage"),
              ugrpc::FindGeneratedMessage(msg + ".NestedMapEntry"),
              ugrpc::FindGeneratedMessage(msg + ".WeirdMapEntry")});
  EXPECT_GT(visitor.GetCompiled(utils::impl::InternalTag()).size(), 7);
}

TEST(FieldsVisitorCompile, ThreeABC) {
  const std::string msg = "sample.ugrpc.MessageWithDifferentTypes";
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated("sample.ugrpc.Msg4A");
  visitor.CompileGenerated("sample.ugrpc.Msg4B");
  visitor.CompileGenerated("sample.ugrpc.Msg4C");
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component4::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component4::GetFieldsWithSelectedChildren());
  EXPECT_EQ(visitor.GetReverseEdges(utils::impl::InternalTag()).size(), 3);
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
  MyExpectEq(visitor.GetCompiled(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
}

TEST(FieldsVisitorCompile, ThreeACB) {
  const std::string msg = "sample.ugrpc.MessageWithDifferentTypes";
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated("sample.ugrpc.Msg4A");
  visitor.CompileGenerated("sample.ugrpc.Msg4C");
  visitor.CompileGenerated("sample.ugrpc.Msg4B");
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component4::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component4::GetFieldsWithSelectedChildren());
  EXPECT_EQ(visitor.GetReverseEdges(utils::impl::InternalTag()).size(), 3);
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
  MyExpectEq(visitor.GetCompiled(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
}

TEST(FieldsVisitorCompile, ThreeCAB) {
  const std::string msg = "sample.ugrpc.MessageWithDifferentTypes";
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.CompileGenerated("sample.ugrpc.Msg4C");
  visitor.CompileGenerated("sample.ugrpc.Msg4A");
  visitor.CompileGenerated("sample.ugrpc.Msg4B");
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component4::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component4::GetFieldsWithSelectedChildren());
  EXPECT_EQ(visitor.GetReverseEdges(utils::impl::InternalTag()).size(), 3);
  MyExpectEq(visitor.GetPropagated(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
  MyExpectEq(visitor.GetCompiled(utils::impl::InternalTag()),
             {ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4A"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4B"),
              ugrpc::FindGeneratedMessage("sample.ugrpc.Msg4C")});
}

TEST(FieldsVisitorConstructor, TestComponent1) {
  ugrpc::FieldsVisitor visitor(FieldSelector, component1::Get());
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component1::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component1::GetFieldsWithSelectedChildren());
}

TEST(FieldsVisitorConstructor, TestComponent2) {
  ugrpc::FieldsVisitor visitor(FieldSelector, component2::Get());
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component2::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component2::GetFieldsWithSelectedChildren());
}

TEST(FieldsVisitorConstructor, TestComponent3) {
  ugrpc::FieldsVisitor visitor(FieldSelector, component3::Get());
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component3::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component3::GetFieldsWithSelectedChildren());
}

TEST(FieldsVisitorConstructor, TestComponent4) {
  ugrpc::FieldsVisitor visitor(FieldSelector, component4::Get());
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component4::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component4::GetFieldsWithSelectedChildren());
}

TEST(FieldsVisitorConstructor, TestDiffTypes) {
  ugrpc::FieldsVisitor visitor(FieldSelector, diff_types::Get());
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             diff_types::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());
}

TEST(FieldsVisitorConstructor, TestPartialComponent) {
  ugrpc::DescriptorList messages = {
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1A"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1C"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1D"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1E")};

  ugrpc::FieldsVisitor visitor(FieldSelector, messages);
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             component1::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component1::GetFieldsWithSelectedChildren());
}

TEST(FieldsVisitorConstructor, TestMultipleComponents) {
  ugrpc::DescriptorList messages;
  for (const auto& msg : component1::Get()) messages.push_back(msg);
  for (const auto& msg : component2::Get()) messages.push_back(msg);
  for (const auto& msg : component3::Get()) messages.push_back(msg);
  for (const auto& msg : component4::Get()) messages.push_back(msg);

  ugrpc::FieldsVisitor::Dependencies selected_fields;
  selected_fields.merge(component1::GetSelectedFields());
  selected_fields.merge(component2::GetSelectedFields());
  selected_fields.merge(component3::GetSelectedFields());
  selected_fields.merge(component4::GetSelectedFields());

  ugrpc::FieldsVisitor::Dependencies fields_with_selected_children;
  fields_with_selected_children.merge(
      component1::GetFieldsWithSelectedChildren());
  fields_with_selected_children.merge(
      component2::GetFieldsWithSelectedChildren());
  fields_with_selected_children.merge(
      component3::GetFieldsWithSelectedChildren());
  fields_with_selected_children.merge(
      component4::GetFieldsWithSelectedChildren());

  ugrpc::FieldsVisitor visitor(FieldSelector, messages);
  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             selected_fields);
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             fields_with_selected_children);
}

TEST(FieldsVisitorConstructor, TestAllMessageTypes) {
  ugrpc::FieldsVisitor visitor(FieldSelector);

  const ugrpc::FieldsVisitor::Dependencies& sf =
      visitor.GetSelectedFields(utils::impl::InternalTag());
  EXPECT_TRUE(ContainsMessage(sf, "sample.ugrpc.MessageWithDifferentTypes"));
  EXPECT_TRUE(ContainsMessage(
      sf, "sample.ugrpc.MessageWithDifferentTypes.NestedMessage"));

  const ugrpc::FieldsVisitor::Dependencies& fwsc =
      visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag());
  EXPECT_TRUE(ContainsMessage(fwsc, "sample.ugrpc.MessageWithDifferentTypes"));
  EXPECT_TRUE(ContainsMessage(
      fwsc, "sample.ugrpc.MessageWithDifferentTypes.NestedMapEntry"));
}

TEST(FieldsVisitorVisit, TestEmptyMessage) {
  std::size_t calls = 0;
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  sample::ugrpc::MessageWithDifferentTypes message;
  visitor.Visit(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });
  ASSERT_EQ(calls, 0);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, sample::ugrpc::MessageWithDifferentTypes()));
}

TEST(FieldsVisitorVisit, TestMessage) {
  std::size_t calls = 0;
  auto message = ConstructMessage();
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.Visit(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });

  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             diff_types::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());

  const std::size_t expected_calls = 1 +  // optional_string
                                     1 +  // optional_int
                                     1;   // repeated_message
  ASSERT_EQ(calls, expected_calls);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, ConstructMessage()));
}

TEST(FieldsVisitorVisitRecursive, TestEmptyMessage) {
  std::size_t calls = 0;
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  sample::ugrpc::MessageWithDifferentTypes message;
  visitor.VisitRecursive(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });
  ASSERT_EQ(calls, 0);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, sample::ugrpc::MessageWithDifferentTypes()));
}

TEST(FieldsVisitorVisitRecursive, TestMessage) {
  std::size_t calls = 0;
  auto message = ConstructMessage();
  ugrpc::FieldsVisitor visitor(FieldSelector, ugrpc::DescriptorList{});
  visitor.VisitRecursive(
      message, [&calls](google::protobuf::Message&,
                        const google::protobuf::FieldDescriptor&) { ++calls; });

  MyExpectEq(visitor.GetSelectedFields(utils::impl::InternalTag()),
             diff_types::GetSelectedFields());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());

  const std::size_t expected_calls = 1 +  // optional_string
                                     1 +  // optional_int
                                     1 +  // required_nested required_string
                                     1 +  // required_recursive optional_string
                                     1 +  // repeated_message
                                     2 +  // repeated_message required_string
                                     2;   // nested_map (values) required_string
  ASSERT_EQ(calls, expected_calls);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, ConstructMessage()));
}

TEST(MessagesVisitorConstructor, TestComponent1) {
  ugrpc::MessagesVisitor visitor(MessageSelector, component1::Get());
  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             component1::GetSelectedMessages());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component1::GetFieldsWithSelectedChildren());
}

TEST(MessagesVisitorConstructor, TestComponent2) {
  ugrpc::MessagesVisitor visitor(MessageSelector, component2::Get());
  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             component2::GetSelectedMessages());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component2::GetFieldsWithSelectedChildren());
}

TEST(MessagesVisitorConstructor, TestComponent3) {
  ugrpc::MessagesVisitor visitor(MessageSelector, component3::Get());
  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             component3::GetSelectedMessages());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component3::GetFieldsWithSelectedChildren());
}

TEST(MessagesVisitorConstructor, TestComponent4) {
  ugrpc::MessagesVisitor visitor(MessageSelector, component4::Get());
  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             component4::GetSelectedMessages());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component4::GetFieldsWithSelectedChildren());
}

TEST(MessagesVisitorConstructor, TestDiffTypes) {
  ugrpc::MessagesVisitor visitor(MessageSelector, diff_types::Get());
  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             diff_types::GetSelectedMessages());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());
}

TEST(MessagesVisitorConstructor, TestPartialComponent) {
  ugrpc::DescriptorList messages = {
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1A"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1C"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1D"),
      ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1E")};
  ugrpc::MessagesVisitor visitor(MessageSelector, messages);
  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             component1::GetSelectedMessages());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             component1::GetFieldsWithSelectedChildren());
}

TEST(MessagesVisitorConstructor, TestMultipleComponents) {
  ugrpc::DescriptorList messages;
  for (const auto& msg : component1::Get()) messages.push_back(msg);
  for (const auto& msg : component2::Get()) messages.push_back(msg);
  for (const auto& msg : component3::Get()) messages.push_back(msg);
  for (const auto& msg : component4::Get()) messages.push_back(msg);

  ugrpc::MessagesVisitor::DescriptorSet selected_messages;
  selected_messages.merge(component1::GetSelectedMessages());
  selected_messages.merge(component2::GetSelectedMessages());
  selected_messages.merge(component3::GetSelectedMessages());
  selected_messages.merge(component4::GetSelectedMessages());

  ugrpc::MessagesVisitor::Dependencies fields_with_selected_children;
  fields_with_selected_children.merge(
      component1::GetFieldsWithSelectedChildren());
  fields_with_selected_children.merge(
      component2::GetFieldsWithSelectedChildren());
  fields_with_selected_children.merge(
      component3::GetFieldsWithSelectedChildren());
  fields_with_selected_children.merge(
      component4::GetFieldsWithSelectedChildren());

  ugrpc::MessagesVisitor visitor(MessageSelector, messages);
  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             selected_messages);
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             fields_with_selected_children);
}

TEST(MessagesVisitorConstructor, TestAllMessageTypes) {
  ugrpc::MessagesVisitor visitor(MessageSelector);

  const ugrpc::MessagesVisitor::DescriptorSet& sm =
      visitor.GetSelectedMessages(utils::impl::InternalTag());
  EXPECT_TRUE(ContainsMessage(
      sm, "sample.ugrpc.MessageWithDifferentTypes.NestedMessage"));

  const ugrpc::MessagesVisitor::Dependencies& fwsc =
      visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag());
  EXPECT_TRUE(ContainsMessage(fwsc, "sample.ugrpc.MessageWithDifferentTypes"));
  EXPECT_TRUE(ContainsMessage(
      fwsc, "sample.ugrpc.MessageWithDifferentTypes.NestedMapEntry"));
}

TEST(MessagesVisitorVisit, TestEmptyMessage) {
  std::size_t calls = 0;
  ugrpc::MessagesVisitor visitor(MessageSelector, ugrpc::DescriptorList{});
  sample::ugrpc::MessageWithDifferentTypes::NestedMessage message;
  visitor.Visit(message, [&calls](google::protobuf::Message&) { ++calls; });
  ASSERT_EQ(calls, 1);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, sample::ugrpc::MessageWithDifferentTypes::NestedMessage()));
}

TEST(MessagesVisitorVisit, TestMessage) {
  std::size_t calls = 0;
  auto message = ConstructMessage();
  ugrpc::MessagesVisitor visitor(MessageSelector, ugrpc::DescriptorList{});
  visitor.Visit(*message.mutable_required_nested(),
                [&calls](google::protobuf::Message&) { ++calls; });
  ASSERT_EQ(calls, 1);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, ConstructMessage()));
}

TEST(MessagesVisitorVisitRecursive, TestEmptyMessage) {
  std::size_t calls = 0;
  ugrpc::MessagesVisitor visitor(MessageSelector, ugrpc::DescriptorList{});
  sample::ugrpc::MessageWithDifferentTypes::NestedMessage message;
  visitor.VisitRecursive(message,
                         [&calls](google::protobuf::Message&) { ++calls; });
  ASSERT_EQ(calls, 1);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, sample::ugrpc::MessageWithDifferentTypes::NestedMessage()));
}

TEST(MessagesVisitorVisitRecursive, TestMessage) {
  std::size_t calls = 0;
  auto message = ConstructMessage();
  ugrpc::MessagesVisitor visitor(MessageSelector, ugrpc::DescriptorList{});
  visitor.VisitRecursive(message,
                         [&calls](google::protobuf::Message&) { ++calls; });

  MyExpectEq(visitor.GetSelectedMessages(utils::impl::InternalTag()),
             diff_types::GetSelectedMessages());
  MyExpectEq(visitor.GetFieldsWithSelectedChildren(utils::impl::InternalTag()),
             diff_types::GetFieldsWithSelectedChildren());

  const std::size_t expected_calls = 1 +  // required_nested
                                     2 +  // repeated_message
                                     2;   // nested_map values
  ASSERT_EQ(calls, expected_calls);
  ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(
      message, ConstructMessage()));
}

USERVER_NAMESPACE_END
