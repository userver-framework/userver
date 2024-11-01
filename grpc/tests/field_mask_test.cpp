#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gmock/gmock.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/field_mask.pb.h>
#include <google/protobuf/util/field_mask_util.h>
#include <google/protobuf/util/message_differencer.h>
#include <gtest/gtest.h>

#include <userver/ugrpc/field_mask.hpp>
#include <userver/ugrpc/protobuf_visit.hpp>
#include <userver/utils/text_light.hpp>

#include <tests/protobuf.pb.h>

USERVER_NAMESPACE_BEGIN

namespace {

#if GOOGLE_PROTOBUF_VERSION >= 3013000
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISABLE_IF_OLD_PROTOBUF_TEST(test_suite, name) TEST(test_suite, name)
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISABLE_IF_OLD_PROTOBUF_TEST(test_suite, name) TEST(test_suite, DISABLED_##name)
#endif

const std::vector<std::string> kSmallFieldMask = {"abc.`def.gh`.qc gh", "value.field"};

// These should be equivalent. Depends on the order of paths in the mask.
const std::string kSmallFieldMaskCommaSeparatedString1 = "abc.`def.gh`.qc gh,value.field";
const std::string kSmallFieldMaskCommaSeparatedString2 = "value.field,abc.`def.gh`.qc gh";

// These should be equivalent. Depends on the order of paths in the mask.
const std::string kSmallFieldMaskWebSafeBase64String1 = "YWJjLmBkZWYuZ2hgLnFjIGdoLHZhbHVlLmZpZWxk";
const std::string kSmallFieldMaskWebSafeBase64String2 = "dmFsdWUuZmllbGQsYWJjLmBkZWYuZ2hgLnFjIGdo";

void EnsureEqualToSmallMask(const ugrpc::FieldMask& field_mask) {
    EXPECT_FALSE(field_mask.IsLeaf());
    EXPECT_THAT(field_mask.GetFieldNames(), testing::UnorderedElementsAreArray({"abc", "value"}));

    const ugrpc::FieldMask& abc = field_mask.GetMaskForField("abc").value();
    EXPECT_FALSE(abc.IsLeaf());
    EXPECT_THAT(abc.GetFieldNames(), testing::UnorderedElementsAreArray({"def.gh"}));

    const ugrpc::FieldMask& defgh = abc.GetMaskForField("def.gh").value();
    EXPECT_FALSE(defgh.IsLeaf());
    EXPECT_THAT(defgh.GetFieldNames(), testing::UnorderedElementsAreArray({"qc gh"}));

    const ugrpc::FieldMask& qcgh = defgh.GetMaskForField("qc gh").value();
    EXPECT_TRUE(qcgh.IsLeaf());
    EXPECT_THAT(qcgh.GetFieldNamesList(), testing::IsEmpty());

    const ugrpc::FieldMask& value = field_mask.GetMaskForField("value").value();
    EXPECT_FALSE(value.IsLeaf());
    EXPECT_THAT(value.GetFieldNames(), testing::UnorderedElementsAreArray({"field"}));

    const ugrpc::FieldMask& field = value.GetMaskForField("field").value();
    EXPECT_TRUE(field.IsLeaf());
    EXPECT_THAT(field.GetFieldNamesList(), testing::IsEmpty());
}

const std::vector<std::string> kMockFieldMask = {
    "root3.category3.field2",                 // Three levels of nesting
    "root3.category2.field1",                 // Three levels of nesting
    "root1",                                  // Value on the root level
    "root2.category1",                        // Two levels of nesting
    "root3.category3.field1",                 // Three levels of nesting
    "root2.category2",                        // Two levels of nesting
    "root3.category3.field3",                 // Three levels of nesting
    "root4.category4.`field4`",               // Duplicate, should be ommitted
    "root4.category4",                        // Covers root4.category4.field4
    "root5.`category5`.`field5`",             // Duplicate, should be ommitted
    "root5",                                  // Covers root5.category5 and root5.category5.field5
    "`root5`.category5",                      // Duplicate, should be ommitted
    "`root6 subroot1`.category6",             // Space in root path, backticks
    "`root6.subroot2`.category6",             // Dot in root path, backticks
    "root7.`category7 subcategory1`.field7",  // Space in lvl 1 path, backticks
    "root7.`category7.subcategory2`.field7",  // Dot in lvl 1 path, backticks
    "`root8`",                                // Backticks and value on the root level
    "root9.*.field"                           // Repeated start
};

const std::vector<std::string> kSimpleFieldMask = {
    "required_string",                  // Root object
    "optional_int",                     // Root object
    "required_nested.optional_string",  // Nested object
    "required_nested.optional_int",     // Nested object
    "required_recursive.required_int",  // Recursive object
    "required_recursive.optional_int",  // Recursive object
    "repeated_primitive",               // Repeated
    "nested_map",                       // Map
    "oneof_string",                     // Oneof
    "oneof_int",                        // Oneof
};

const std::vector<std::string> kHardFieldMask = {
    "required_string",                           // Root object
    "`optional_int`",                            // Root object, with backticks
    "required_nested.optional_string",           // Nested object
    "`required_nested`.`optional_int`",          // Nested object, backticks
    "required_recursive.required_int",           // Recursive object
    "required_recursive.optional_int",           // Recursive object
    "repeated_primitive.*",                      // Equivalent to repeated_primitive
    "repeated_message.*.required_string",        // Go into the repeated
    "repeated_message.*.required_int",           // Go into the repeated
    "primitives_map.key1",                       // Map keys
    "primitives_map.key2",                       // Map keys
    "nested_map.`key1 value1`.required_int",     // Map values, space in key
    "nested_map.`key1 value1`.optional_int",     // Map values, space in key
    "nested_map.`key2.value2`.required_string",  // Map values, dot in key
    "nested_map.`key2.value2`.optional_int",     // Map values, dot in key
    "nested_map.`key3.value3 field3`",           // All fields, dot and space in key
    "nested_map.*.optional_string"               // All values
};

google::protobuf::FieldMask MakeGoogleFieldMask(const std::vector<std::string>& paths) {
    google::protobuf::FieldMask field_mask;
    for (const std::string& path : paths) {
        field_mask.add_paths(path);
    }
    return field_mask;
}

sample::ugrpc::MessageWithDifferentTypes::NestedMessage* ConstructNestedMessagePtr() {
    auto* message = new sample::ugrpc::MessageWithDifferentTypes::NestedMessage();
    message->set_required_string("string1");
    message->set_optional_string("string2");
    message->set_required_int(123321);
    message->set_optional_int(456654);
    return message;
}

sample::ugrpc::MessageWithDifferentTypes::NestedMessage ConstructNestedMessage() {
    sample::ugrpc::MessageWithDifferentTypes::NestedMessage message;
    message.set_required_string("string1");
    message.set_optional_string("string2");
    message.set_required_int(123321);
    message.set_optional_int(456654);
    return message;
}

sample::ugrpc::MessageWithDifferentTypes* ConstructMessage(bool with_recursive = true) {
    auto* message = new sample::ugrpc::MessageWithDifferentTypes();

    // leave required_string empty
    message->set_optional_string("string2");

    message->set_required_int(123321);
    message->set_optional_int(456654);

    message->set_allocated_required_nested(ConstructNestedMessagePtr());
    message->set_allocated_optional_nested(ConstructNestedMessagePtr());

    if (with_recursive) {
        message->set_allocated_required_recursive(ConstructMessage(false));
        message->set_allocated_optional_recursive(ConstructMessage(false));
    }

    message->add_repeated_primitive("string1");
    message->add_repeated_primitive("string2");
    message->add_repeated_primitive("string3");

    message->mutable_repeated_message()->Add(ConstructNestedMessage());
    message->mutable_repeated_message()->Add(ConstructNestedMessage());
    message->mutable_repeated_message()->Add(ConstructNestedMessage());

    // No key1
    message->mutable_primitives_map()->insert({"key2", "value2"});
    message->mutable_primitives_map()->insert({"key3", "value3"});

    message->mutable_nested_map()->insert({"key1", ConstructNestedMessage()});
    message->mutable_nested_map()->insert({"key2", ConstructNestedMessage()});
    message->mutable_nested_map()->insert({"key3", ConstructNestedMessage()});
    message->mutable_nested_map()->insert({"key1 value1", ConstructNestedMessage()});
    // No key2.value2
    message->mutable_nested_map()->insert({"key3.value3 field3", ConstructNestedMessage()});

    // leave oneof_string empty
    message->set_oneof_int(789987);
    // leave oneof_nested empty

    message->mutable_google_value()->set_string_value("string");

    return message;
}

sample::ugrpc::MessageWithDifferentTypes ConstructTrimmedSimple() {
    sample::ugrpc::MessageWithDifferentTypes message;

    // required_string, optional_int
    message.set_optional_int(456654);

    // required_nested.optional_string, required_nested.optional_int
    message.mutable_required_nested()->set_optional_string("string2");
    message.mutable_required_nested()->set_optional_int(456654);

    // required_recursive.required_int, required_recursive.optional_int
    message.mutable_required_recursive()->set_required_int(123321);
    message.mutable_required_recursive()->set_optional_int(456654);

    // repeated_primitive
    message.add_repeated_primitive("string1");
    message.add_repeated_primitive("string2");
    message.add_repeated_primitive("string3");

    // nested_map
    message.mutable_nested_map()->insert({"key1", ConstructNestedMessage()});
    message.mutable_nested_map()->insert({"key2", ConstructNestedMessage()});
    message.mutable_nested_map()->insert({"key3", ConstructNestedMessage()});
    message.mutable_nested_map()->insert({"key1 value1", ConstructNestedMessage()});
    // No key2.value2
    message.mutable_nested_map()->insert({"key3.value3 field3", ConstructNestedMessage()});

    // oneof_string, oneof_int
    message.set_oneof_int(789987);

    return message;
}

sample::ugrpc::MessageWithDifferentTypes ConstructTrimmedHard() {
    sample::ugrpc::MessageWithDifferentTypes message;

    // required_string, optional_int
    message.set_optional_int(456654);

    // required_nested.optional_string, required_nested.optional_int
    message.mutable_required_nested()->set_optional_string("string2");
    message.mutable_required_nested()->set_optional_int(456654);

    // required_recursive.required_int, required_recursive.optional_int
    message.mutable_required_recursive()->set_required_int(123321);
    message.mutable_required_recursive()->set_optional_int(456654);

    // repeated_primitive.*
    message.add_repeated_primitive("string1");
    message.add_repeated_primitive("string2");
    message.add_repeated_primitive("string3");

    // repeated_message.*.required_string, repeated_message.*.required_int
    message.mutable_repeated_message()->Add(ConstructNestedMessage());
    message.mutable_repeated_message()->Add(ConstructNestedMessage());
    message.mutable_repeated_message()->Add(ConstructNestedMessage());
    for (auto& msg : *message.mutable_repeated_message()) {
        msg.clear_optional_string();
        msg.clear_optional_int();
    }

    // primitives_map.key1, primitives_map.key2
    message.mutable_primitives_map()->insert({"key2", "value2"});

    // nested_map.`key1 value1`.required_int, nested_map.`key1
    // value1`.optional_int,
    sample::ugrpc::MessageWithDifferentTypes::NestedMessage msg1;
    msg1.set_required_int(123321);
    msg1.set_optional_int(456654);
    message.mutable_nested_map()->insert({"key1 value1", std::move(msg1)});

    // nested_map.`key2.value2`.required_string,
    // nested_map.`key2.value2`.optional_int No key2.value2

    // nested_map.`key3.value3 field3`
    message.mutable_nested_map()->insert({"key3.value3 field3", ConstructNestedMessage()});

    // nested_map.*.optional_string
    sample::ugrpc::MessageWithDifferentTypes::NestedMessage msg;
    msg.set_optional_string("string2");
    message.mutable_nested_map()->insert({"key1", msg});
    message.mutable_nested_map()->insert({"key2", msg});
    message.mutable_nested_map()->insert({"key3", msg});

    return message;
}

void Compare(const google::protobuf::Message& val1, const google::protobuf::Message& val2) {
    EXPECT_EQ(std::string(val1.Utf8DebugString()), std::string(val2.Utf8DebugString()));
    EXPECT_TRUE(google::protobuf::util::MessageDifferencer::Equals(val1, val2));
}

template <typename T>
std::vector<std::string> ToList(const T& vector) {
    std::vector<std::string> out;
    for (const auto& item : vector) out.push_back(item);
    return out;
}

}  // namespace

TEST(FieldMaskConstructor, TestDefault) {
    const ugrpc::FieldMask field_mask;
    EXPECT_THAT(field_mask.GetFieldNamesList(), testing::IsEmpty());
    EXPECT_TRUE(field_mask.IsLeaf());
}

TEST(FieldMaskConstructor, MockFieldMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kMockFieldMask));

    EXPECT_THAT(
        field_mask.GetFieldNamesList(),
        testing::UnorderedElementsAreArray(
            {"root1", "root2", "root3", "root4", "root5", "root6 subroot1", "root6.subroot2", "root7", "root8", "root9"}
        )
    );
    EXPECT_FALSE(field_mask.IsLeaf());

    // root1
    EXPECT_THAT(field_mask.GetMaskForField("root1")->GetFieldNamesList(), testing::IsEmpty());
    EXPECT_TRUE(field_mask.GetMaskForField("root1")->IsLeaf());

    // root2
    EXPECT_THAT(
        field_mask.GetMaskForField("root2")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"category1", "category2"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root2")->IsLeaf());

    // root2.category1
    EXPECT_THAT(
        field_mask.GetMaskForField("root2")->GetMaskForField("category1")->GetFieldNamesList(), testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root2")->GetMaskForField("category1")->IsLeaf());

    // root2.category2
    EXPECT_THAT(
        field_mask.GetMaskForField("root2")->GetMaskForField("category2")->GetFieldNamesList(), testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root2")->GetMaskForField("category2")->IsLeaf());

    // root3
    EXPECT_THAT(
        field_mask.GetMaskForField("root3")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"category2", "category3"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root3")->IsLeaf());

    // root3.category2
    EXPECT_THAT(
        field_mask.GetMaskForField("root3")->GetMaskForField("category2")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"field1"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root3")->GetMaskForField("category2")->IsLeaf());

    // root3.category2.field1
    EXPECT_THAT(
        field_mask.GetMaskForField("root3")
            ->GetMaskForField("category2")
            ->GetMaskForField("field1")
            ->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root3")->GetMaskForField("category2")->GetMaskForField("field1")->IsLeaf());

    // root3.category3
    EXPECT_THAT(
        field_mask.GetMaskForField("root3")->GetMaskForField("category3")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"field1", "field2", "field3"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root3")->GetMaskForField("category3")->IsLeaf());

    // root3.category3.field1
    EXPECT_THAT(
        field_mask.GetMaskForField("root3")
            ->GetMaskForField("category3")
            ->GetMaskForField("field1")
            ->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root3")->GetMaskForField("category3")->GetMaskForField("field1")->IsLeaf());

    // root3.category3.field2
    EXPECT_THAT(
        field_mask.GetMaskForField("root3")
            ->GetMaskForField("category3")
            ->GetMaskForField("field2")
            ->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root3")->GetMaskForField("category3")->GetMaskForField("field2")->IsLeaf());

    // root3.category3.field3
    EXPECT_THAT(
        field_mask.GetMaskForField("root3")
            ->GetMaskForField("category3")
            ->GetMaskForField("field3")
            ->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root3")->GetMaskForField("category3")->GetMaskForField("field3")->IsLeaf());

    // root4
    EXPECT_THAT(
        field_mask.GetMaskForField("root4")->GetFieldNamesList(), testing::UnorderedElementsAreArray({"category4"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root4")->IsLeaf());

    // root4.category4
    EXPECT_THAT(
        field_mask.GetMaskForField("root4")->GetMaskForField("category4")->GetFieldNamesList(), testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root4")->GetMaskForField("category4")->IsLeaf());

    // root5
    EXPECT_THAT(field_mask.GetMaskForField("root5")->GetFieldNamesList(), testing::IsEmpty());
    EXPECT_TRUE(field_mask.GetMaskForField("root5")->IsLeaf());

    // root6 subroot1
    EXPECT_THAT(
        field_mask.GetMaskForField("root6 subroot1")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"category6"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root6 subroot1")->IsLeaf());

    // `root6 subroot1`.category6
    EXPECT_THAT(
        field_mask.GetMaskForField("root6 subroot1")->GetMaskForField("category6")->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root6 subroot1")->GetMaskForField("category6")->IsLeaf());

    // root6.subroot2
    EXPECT_THAT(
        field_mask.GetMaskForField("root6.subroot2")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"category6"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root6.subroot2")->IsLeaf());

    // `root6.subroot2`.category6
    EXPECT_THAT(
        field_mask.GetMaskForField("root6.subroot2")->GetMaskForField("category6")->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root6.subroot2")->GetMaskForField("category6")->IsLeaf());

    // root7
    EXPECT_THAT(
        field_mask.GetMaskForField("root7")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"category7 subcategory1", "category7.subcategory2"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root7")->IsLeaf());

    // root7.`category7 subcategory1`
    EXPECT_THAT(
        field_mask.GetMaskForField("root7")->GetMaskForField("category7 subcategory1")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"field7"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root7")->GetMaskForField("category7 subcategory1")->IsLeaf());

    // root7.`category7 subcategory1`.field7
    EXPECT_THAT(
        field_mask.GetMaskForField("root7")
            ->GetMaskForField("category7 subcategory1")
            ->GetMaskForField("field7")
            ->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root7")
                    ->GetMaskForField("category7 subcategory1")
                    ->GetMaskForField("field7")
                    ->IsLeaf());

    // root7.`category7.subcategory2`
    EXPECT_THAT(
        field_mask.GetMaskForField("root7")->GetMaskForField("category7.subcategory2")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"field7"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root7")->GetMaskForField("category7.subcategory2")->IsLeaf());

    // root7.`category7.subcategory2`.field7
    EXPECT_THAT(
        field_mask.GetMaskForField("root7")
            ->GetMaskForField("category7.subcategory2")
            ->GetMaskForField("field7")
            ->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root7")
                    ->GetMaskForField("category7.subcategory2")
                    ->GetMaskForField("field7")
                    ->IsLeaf());

    // root8
    EXPECT_THAT(field_mask.GetMaskForField("root8")->GetFieldNamesList(), testing::IsEmpty());
    EXPECT_TRUE(field_mask.GetMaskForField("root8")->IsLeaf());

    // root9
    EXPECT_THAT(field_mask.GetMaskForField("root9")->GetFieldNamesList(), testing::UnorderedElementsAreArray({"*"}));
    EXPECT_FALSE(field_mask.GetMaskForField("root9")->IsLeaf());

    // root9.*
    EXPECT_THAT(
        field_mask.GetMaskForField("root9")->GetMaskForField("*")->GetFieldNamesList(),
        testing::UnorderedElementsAreArray({"field"})
    );
    EXPECT_FALSE(field_mask.GetMaskForField("root9")->GetMaskForField("*")->IsLeaf());

    // root9.*.field
    EXPECT_THAT(
        field_mask.GetMaskForField("root9")->GetMaskForField("*")->GetMaskForField("field")->GetFieldNamesList(),
        testing::IsEmpty()
    );
    EXPECT_TRUE(field_mask.GetMaskForField("root9")->GetMaskForField("*")->GetMaskForField("field")->IsLeaf());
}

TEST(FieldMaskConstructor, SimpleFieldMask) {
    EXPECT_NO_THROW(ugrpc::FieldMask(MakeGoogleFieldMask(kSimpleFieldMask)));
}

TEST(FieldMaskConstructor, HardFieldMask) { EXPECT_NO_THROW(ugrpc::FieldMask(MakeGoogleFieldMask(kHardFieldMask))); }

TEST(FieldMaskConstructor, FromCommaSeparatedString1) {
    ugrpc::FieldMask field_mask(kSmallFieldMaskCommaSeparatedString1, ugrpc::FieldMask::Encoding::kCommaSeparated);
    EnsureEqualToSmallMask(field_mask);
}

TEST(FieldMaskConstructor, FromCommaSeparatedString2) {
    ugrpc::FieldMask field_mask(kSmallFieldMaskCommaSeparatedString2, ugrpc::FieldMask::Encoding::kCommaSeparated);
    EnsureEqualToSmallMask(field_mask);
}

TEST(FieldMaskConstructor, FromWebSafeBase64String1) {
    ugrpc::FieldMask field_mask(kSmallFieldMaskWebSafeBase64String1, ugrpc::FieldMask::Encoding::kWebSafeBase64);
    EnsureEqualToSmallMask(field_mask);
}

TEST(FieldMaskConstructor, FromWebSafeBase64String2) {
    ugrpc::FieldMask field_mask(kSmallFieldMaskWebSafeBase64String2, ugrpc::FieldMask::Encoding::kWebSafeBase64);
    EnsureEqualToSmallMask(field_mask);
}

TEST(FieldMaskToString, EmptyMask) {
    const ugrpc::FieldMask field_mask;
    EXPECT_THAT(field_mask.ToString(), "");
}

TEST(FieldMaskToString, SmallMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kSmallFieldMask));
    EXPECT_THAT(
        field_mask.ToString(),
        testing::AnyOf(kSmallFieldMaskCommaSeparatedString1, kSmallFieldMaskCommaSeparatedString2)
    );
}

TEST(FieldMaskToString, MockMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kMockFieldMask));
    EXPECT_THAT(
        utils::text::Split(field_mask.ToString(), ","),
        testing::UnorderedElementsAreArray(
            {"root1",
             "root2.category1",
             "root2.category2",
             "root3.category3.field2",
             "root3.category2.field1",
             "root3.category3.field1",
             "root3.category3.field3",
             "root4.category4",
             "root5",
             "root6 subroot1.category6",
             "`root6.subroot2`.category6",
             "root7.category7 subcategory1.field7",
             "root7.`category7.subcategory2`.field7",
             "root8",
             "root9.*.field"}
        )
    );
}

TEST(FieldMaskToString, SimpleMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kSimpleFieldMask));
    EXPECT_THAT(
        utils::text::Split(field_mask.ToString(), ","),
        testing::UnorderedElementsAreArray({
            "required_string",
            "optional_int",
            "required_nested.optional_string",
            "required_nested.optional_int",
            "required_recursive.required_int",
            "required_recursive.optional_int",
            "repeated_primitive",
            "nested_map",
            "oneof_string",
            "oneof_int",
        })
    );
}

TEST(FieldMaskToString, HardMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kHardFieldMask));
    EXPECT_THAT(
        utils::text::Split(field_mask.ToString(), ","),
        testing::UnorderedElementsAreArray(
            {"required_string",
             "optional_int",
             "required_nested.optional_string",
             "required_nested.optional_int",
             "required_recursive.required_int",
             "required_recursive.optional_int",
             "repeated_primitive.*",
             "repeated_message.*.required_string",
             "repeated_message.*.required_int",
             "primitives_map.key1",
             "primitives_map.key2",
             "nested_map.key1 value1.required_int",
             "nested_map.key1 value1.optional_int",
             "nested_map.`key2.value2`.required_string",
             "nested_map.`key2.value2`.optional_int",
             "nested_map.`key3.value3 field3`",
             "nested_map.*.optional_string"}
        )
    );
}

TEST(FieldMaskToWebSafeBase64, EmptyMask) {
    const ugrpc::FieldMask field_mask;
    EXPECT_THAT(field_mask.ToWebSafeBase64(), "");
}

TEST(FieldMaskToWebSafeBase64, SmallMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kSmallFieldMask));
    EXPECT_THAT(
        field_mask.ToWebSafeBase64(),
        testing::AnyOf(kSmallFieldMaskWebSafeBase64String1, kSmallFieldMaskWebSafeBase64String2)
    );
}

TEST(FieldMaskToWebSafeBase64, MockMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kMockFieldMask));
    EXPECT_NO_THROW(field_mask.ToWebSafeBase64());
}

TEST(FieldMaskToWebSafeBase64, SimpleMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kSimpleFieldMask));
    EXPECT_NO_THROW(field_mask.ToWebSafeBase64());
}

TEST(FieldMaskToWebSafeBase64, HardMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kHardFieldMask));
    EXPECT_NO_THROW(field_mask.ToWebSafeBase64());
}

TEST(FieldMaskAddPath, Errors) {
    ugrpc::FieldMask fm;

    EXPECT_THROW(fm.AddPath("."), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath(".."), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath(".value1"), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath("value2.."), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath(".value3."), ugrpc::FieldMask::BadPathError);

    EXPECT_THROW(fm.AddPath("`"), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath("``"), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath("`value5"), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath("`value6``"), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath("`value7`.`"), ugrpc::FieldMask::BadPathError);
    EXPECT_THROW(fm.AddPath("value8.`unclosed.field"), ugrpc::FieldMask::BadPathError);
}

TEST(FieldMaskIsPathFullyIn, MockFieldMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kMockFieldMask));
    EXPECT_FALSE(field_mask.IsPathFullyIn(""));

    // root1 root5, and root8 are specified => everything inside should be true
    EXPECT_TRUE(field_mask.IsPathFullyIn("root1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root1.category1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root1.category2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root1.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root1.category1.field1.value1"));

    EXPECT_TRUE(field_mask.IsPathFullyIn("root5"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root5.category1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root5.category2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root5.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root5.category1.field1.value1"));

    EXPECT_TRUE(field_mask.IsPathFullyIn("root8"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root8.category1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root8.category2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root8.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root8.category1.field1.value1"));

    // root2 and root4 are specified up to level 2 => nesting on the first level
    // may return false
    EXPECT_FALSE(field_mask.IsPathFullyIn("root2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root2.category1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root2.category2"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root2.category3"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root2.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root2.category2.field1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root2.category3.field1"));

    EXPECT_FALSE(field_mask.IsPathFullyIn("root4"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root4.category3"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root4.category4"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root4.category5"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root4.category3.field1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root4.category4.field1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root4.category5.field1"));

    // root6 subroot1 and root6.subroot2 are similarly specified up to level 2
    EXPECT_FALSE(field_mask.IsPathFullyIn("root6 subroot1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("`root6 subroot1`.category6"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root6 subroot1.category6"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("`root6 subroot1`.category6.field1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("`root6 subroot1`.category7"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("`root6 subroot1`.category7.field1"));

    EXPECT_FALSE(field_mask.IsPathFullyIn("`root6.subroot2`"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("`root6.subroot2`.category6"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("`root6.subroot2`.category6.field1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("`root6.subroot2`.category7"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("`root6.subroot2`.category7.field1"));

    // root3 is specified up to level 3 => nesting on levels 1 and 2 may return
    // false
    EXPECT_FALSE(field_mask.IsPathFullyIn("root3"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root3.category1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root3.category2"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root3.category3"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root3.category4"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root3.category2.field1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root3.category2.field2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root3.category3.field1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root3.category3.field2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root3.category3.field3"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root3.category3.field4"));

    // root7 is similarly specified up to level 3
    EXPECT_FALSE(field_mask.IsPathFullyIn("root7"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root7.`category7 subcategory1`"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root7.`category7.subcategory2`"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root7.`category7.subcategory3`"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root7.`category7 subcategory4`"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root7.category5"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root7.`category7 subcategory1`.field7"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root7.`category7.subcategory2`.field7"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root7.`category7 subcategory2`.field7"));

    // root6 is not specified
    EXPECT_FALSE(field_mask.IsPathFullyIn("root6"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root6.category1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root6.category1.field1"));

    // root9 is specified with a wildcards selector
    EXPECT_FALSE(field_mask.IsPathFullyIn("root9"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root9.*"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root9.*.field"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root9.*.field2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root9.*.field.subfield1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root9.*.field.subfield2"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root9.*.field2.subfield1"));
    EXPECT_FALSE(field_mask.IsPathFullyIn("root9.*.field2.subfield2"));
}

TEST(FieldMaskIsPathFullyIn, EmptyMask) {
    // Empty mask is equivalent to the mask with all fields specified
    const ugrpc::FieldMask field_mask;
    EXPECT_TRUE(field_mask.IsPathFullyIn(""));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root2"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root3"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root4"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root5"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root6"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root6.category1"));
    EXPECT_TRUE(field_mask.IsPathFullyIn("root6.category1.field1"));
}

TEST(FieldMaskIsPathFullyIn, BadPath) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kMockFieldMask));
    EXPECT_THROW(field_mask.IsPathFullyIn("`value.."), ugrpc::FieldMask::BadPathError);
}

TEST(FieldMaskIsPathPartiallyIn, MockFieldMask) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kMockFieldMask));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn(""));

    // root1 and root5 are specified => everything inside should be true
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root1.category1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root1.category2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root1.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root1.category1.field1.value1"));

    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root5"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root5.category1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root5.category2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root5.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root5.category1.field1.value1"));

    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root8"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root8.category1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root8.category2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root8.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root8.category1.field1.value1"));

    // root2 and root4 are specified up to level 2 => nesting on the first level
    // may return false
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root2.category1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root2.category2"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root2.category3"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root2.category1.field1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root2.category2.field1"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root2.category3.field1"));

    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root4"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root4.category3"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root4.category4"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root4.category5"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root4.category3.field1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root4.category4.field1"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root4.category5.field1"));

    // root6 subroot1 and root6.subroot2 are similarly specified up to level 2
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root6 subroot1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("`root6 subroot1`.category6"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("`root6 subroot1`.category6.field1"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("`root6 subroot1`.category7"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("`root6 subroot1`.category7.field1"));

    EXPECT_TRUE(field_mask.IsPathPartiallyIn("`root6.subroot2`"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("`root6.subroot2`.category6"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("`root6.subroot2`.category6.field1"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("`root6.subroot2`.category7"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("`root6.subroot2`.category7.field1"));

    // root3 is specified up to level 3 => nesting on levels 1 and 2 may return
    // false
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root3.category1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3.category2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3.category3"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root3.category4"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3.category2.field1"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root3.category2.field2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3.category3.field1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3.category3.field2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3.category3.field3"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root3.category3.field4"));

    // root7 is similarly specified up to level 3
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root7"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root7.`category7 subcategory1`"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root7.`category7.subcategory2`"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root7.`category7.subcategory3`"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root7.`category7 subcategory4`"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root7.category5"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root7.`category7 subcategory1`.field7"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root7.`category7.subcategory2`.field7"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root7.`category7 subcategory2`.field7"));

    // root6 is not specified
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root6"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root6.category1"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root6.category1.field1"));

    // root9 is specified with a wildcards selector
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root9"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root9.*"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root9.*.field"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root9.*.field2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root9.*.field.subfield1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root9.*.field.subfield2"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root9.*.field2.subfield1"));
    EXPECT_FALSE(field_mask.IsPathPartiallyIn("root9.*.field2.subfield2"));
}

TEST(FieldMaskIsPathPartiallyIn, EmptyMask) {
    // Empty mask is equivalent to the mask with all fields specified
    const ugrpc::FieldMask field_mask;
    EXPECT_TRUE(field_mask.IsPathPartiallyIn(""));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root2"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root3"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root4"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root5"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root6"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root6.category1"));
    EXPECT_TRUE(field_mask.IsPathPartiallyIn("root6.category1.field1"));
}

TEST(FieldMaskIsPathPartiallyIn, BadPath) {
    const ugrpc::FieldMask field_mask(MakeGoogleFieldMask(kMockFieldMask));
    EXPECT_THROW(field_mask.IsPathPartiallyIn("`value.."), ugrpc::FieldMask::BadPathError);
}

TEST(FieldMaskCheckValidity, Ok) {
    const google::protobuf::Descriptor* descriptor =
        ugrpc::FindGeneratedMessage("sample.ugrpc.MessageWithDifferentTypes");

    std::vector<std::string> kOkPaths = {
        "",                                    // Empty mask
        "optional_int",                        // Field at root
        "required_nested.optional_string",     // Nested field
        "repeated_primitive",                  // Repeated of primitives
        "repeated_primitive.*",                // Repeated of primitives, *
        "repeated_message",                    // Repeated of messages
        "repeated_message.*",                  // Repeated of messages, *
        "repeated_message.*.required_string",  // Repeated of messages, field
        "primitives_map",                      // Map of primitives
        "primitives_map.*",                    // Map of primitives, *
        "primitives_map.key",                  // Map of primitives, key
        "nested_map",                          // Map of messages
        "nested_map.*",                        // Map of messages, *
        "nested_map.key",                      // Map of messages, key
        "oneof_int",                           // oneof
        "google_value",                        // google value
        "weird_map",                           // Map with weird type as key
    };

    for (const std::string& path : kOkPaths) {
        ugrpc::FieldMask fm;
        EXPECT_NO_THROW(fm.AddPath(path));
        EXPECT_NO_THROW(fm.CheckValidity(descriptor));
    }
}

TEST(FieldMaskCheckValidity, Errors) {
    const google::protobuf::Descriptor* descriptor =
        ugrpc::FindGeneratedMessage("sample.ugrpc.MessageWithDifferentTypes");

    std::vector<std::string> kBadPaths = {
        "weird_field",                       // non existing field
        "required_nested.weird_field",       // non existing nested field
        "weird_field.optional_string",       // nested in non existing field
        "repeated_primitive.field",          // Repeated of primitives, field
        "repeated_primitive.*.field",        // Repeated of primitives, *, field
        "repeated_message.bad_field",        // Repeated of messages, bad field
        "repeated_message.*.bad_field",      // Repeated of messages, *, bad field
        "repeated_message.**",               // Repeated of messages, double *
        "repeated_message.required_string",  // Repeated of messages, forget *
        "primitives_map.*.field",            // Map of primitives, *, field
        "primitives_map.key.field",          // Map of primitives, key, field
        "nested_map.*.field",                // Map of messages, *, field
        "nested_map.key.field",              // Map of messages, key, field
        "weird_map.*",                       // Map with weird type as key, *
        "weird_map.key",                     // Map with weird type as key, key
        "weird_map.*.required_string",       // Map with weird type as key
    };

    for (const std::string& path : kBadPaths) {
        ugrpc::FieldMask fm;
        EXPECT_NO_THROW(fm.AddPath(path));
        EXPECT_THROW(fm.CheckValidity(descriptor), ugrpc::FieldMask::BadPathError);
    }
}

TEST(FieldMaskCheckValidity, SimpleFieldMask) {
    ugrpc::FieldMask fm(MakeGoogleFieldMask(kSimpleFieldMask));
    EXPECT_NO_THROW(fm.CheckValidity(ugrpc::FindGeneratedMessage("sample.ugrpc.MessageWithDifferentTypes")));
    EXPECT_THROW(fm.CheckValidity(ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1A")), ugrpc::FieldMask::BadPathError);
}

TEST(FieldMaskCheckValidity, HardFieldMask) {
    ugrpc::FieldMask fm(MakeGoogleFieldMask(kHardFieldMask));
    EXPECT_NO_THROW(fm.CheckValidity(ugrpc::FindGeneratedMessage("sample.ugrpc.MessageWithDifferentTypes")));
    EXPECT_THROW(fm.CheckValidity(ugrpc::FindGeneratedMessage("sample.ugrpc.Msg1A")), ugrpc::FieldMask::BadPathError);
}

TEST(FieldMaskTrim, TrivialMessage) {
    sample::ugrpc::Msg1A message;
    message.set_value1("value1");
    message.set_value2("value2");
    message.mutable_nested()->mutable_recursive_1()->set_value("value");
    message.mutable_nested()->mutable_recursive_2()->set_value("value");
    message.mutable_nested()->mutable_nested_nosecret_1()->set_value("value");
    message.mutable_nested()->mutable_nested_nosecret_2()->set_value("value");
    message.mutable_nested()->mutable_nested_nosecret_3()->set_value("value");

    ugrpc::FieldMask(MakeGoogleFieldMask({
                         "value1",
                         "nested.recursive_1.value",
                         "nested.nested_secret_1",
                         "nested.nested_secret_2.value",
                         "nested.nested_nosecret_1.value",
                         "nested.nested_nosecret_2",
                     }))
        .Trim(message);

    sample::ugrpc::Msg1A trimmed;
    trimmed.set_value1("value1");
    trimmed.mutable_nested()->mutable_recursive_1()->set_value("value");
    trimmed.mutable_nested()->mutable_nested_nosecret_1()->set_value("value");
    trimmed.mutable_nested()->mutable_nested_nosecret_2()->set_value("value");

    Compare(message, trimmed);
}

DISABLE_IF_OLD_PROTOBUF_TEST(FieldMaskTrim, SimpleFieldMask) {
    auto message = ConstructMessage();
    ugrpc::FieldMask(MakeGoogleFieldMask(kSimpleFieldMask)).Trim(*message);
    Compare(*message, ConstructTrimmedSimple());
    delete message;
}

DISABLE_IF_OLD_PROTOBUF_TEST(FieldMaskTrim, HardFieldMask) {
    auto message = ConstructMessage();
    ugrpc::FieldMask(MakeGoogleFieldMask(kHardFieldMask)).Trim(*message);
    Compare(*message, ConstructTrimmedHard());
    delete message;
}

TEST(FieldMaskGetMaskForField, NonExistingChild) {
    ugrpc::FieldMask mask(MakeGoogleFieldMask(kHardFieldMask));
    EXPECT_FALSE(mask.GetMaskForField("something-weird").has_value());
}

TEST(FieldMaskGetMaskForField, NonExistingChildOnLeaf) {
    ugrpc::FieldMask mask;
    EXPECT_TRUE(mask.GetMaskForField("something-weird").has_value());
    EXPECT_EQ(&mask.GetMaskForField("something-weird").value(), &mask.GetMaskForField("something-weird-2").value());
}

TEST(FieldMaskGetMaskForField, FallbackToWildcard) {
    const ugrpc::FieldMask mask(MakeGoogleFieldMask(kHardFieldMask));
    const utils::OptionalRef<const ugrpc::FieldMask> nested_map = mask.GetMaskForField("nested_map");
    const utils::OptionalRef<const ugrpc::FieldMask> unknown_key = nested_map->GetMaskForField("unknown_key");
    EXPECT_THAT(unknown_key->GetFieldNames(), testing::UnorderedElementsAreArray({"optional_string"}));
}

TEST(FieldMaskHasFieldName, MockFieldMask) {
    ugrpc::FieldMask mask(MakeGoogleFieldMask(kMockFieldMask));
    EXPECT_FALSE(mask.HasFieldName("something-weird"));
    EXPECT_TRUE(mask.HasFieldName("root1"));
}

TEST(FieldMaskHasFieldName, NonExistingChildOnLeaf) {
    ugrpc::FieldMask mask;
    EXPECT_TRUE(mask.HasFieldName("something-weird"));
}

USERVER_NAMESPACE_END
