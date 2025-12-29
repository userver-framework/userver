#include <gtest/gtest.h>

#include <userver/formats/yaml/exception.hpp>
#include <userver/formats/yaml/serialize.hpp>
#include <userver/utest/assert_macros.hpp>

#include <formats/common/serialize_test.hpp>

USERVER_NAMESPACE_BEGIN

template <>
struct Serialization<formats::yaml::Value> : public ::testing::Test {
    constexpr static const char* kDoc = "key1: 1\nkey2: val";

    using ValueBuilder = formats::yaml::ValueBuilder;
    using Value = formats::yaml::Value;
    using Type = formats::yaml::Type;

    using ParseException = formats::yaml::ParseException;
    using TypeMismatchException = formats::yaml::TypeMismatchException;
    using OutOfBoundsException = formats::yaml::OutOfBoundsException;
    using MemberMissingException = formats::yaml::MemberMissingException;
    using BadStreamException = formats::yaml::BadStreamException;

    constexpr static auto kFromString = formats::yaml::FromString;
    constexpr static auto kFromStream = formats::yaml::FromStream;
};

INSTANTIATE_TYPED_TEST_SUITE_P(FormatsYaml, Serialization, formats::yaml::Value);

TEST(FormatsYamlDuplicateKeys, AtRoot) {
    const std::string yaml_string = R"(
        Key1: 1
        Key2: 2
        Key3: 3
        Key1: 1
    )";
    UEXPECT_THROW_MSG(
        formats::yaml::FromString(yaml_string),
        formats::yaml::ParseException,
        "Duplicate mapping key 'Key1' at path '/'"
    );
}

TEST(FormatsYamlDuplicateKeys, InObject) {
    const std::string yaml_string = R"(
        Key1:
            Key2: 2
            Key3:
                Key4: 4
                Key5: 5
                Key4: 4
        Key6: 6
        Key7: 7)";
    UEXPECT_THROW_MSG(
        formats::yaml::FromString(yaml_string),
        formats::yaml::ParseException,
        "Duplicate mapping key 'Key4' at path 'Key1.Key3'"
    );
}

TEST(FormatsYamlDuplicateKeys, InArray) {
    const std::string yaml_string = R"(
        - 1
        - 2
        - - 3
          - Key4: 1
            Key4: 1
    )";
    UEXPECT_THROW_MSG(
        formats::yaml::FromString(yaml_string),
        formats::yaml::ParseException,
        "Duplicate mapping key 'Key4' at path '[2][1]'"
    );
}

TEST(FormatsYamlDuplicateKeys, VariousScalarsInMapKeys) {
    {
        const std::string yaml_string = R"(
            1: foo
            1: foo
        )";
        UEXPECT_THROW_MSG(
            formats::yaml::FromString(yaml_string),
            formats::yaml::ParseException,
            "Duplicate mapping key '1' at path '/'"
        );
    }

    {
        const std::string yaml_string = R"(
            true: foo
            true: foo
        )";
        UEXPECT_THROW_MSG(
            formats::yaml::FromString(yaml_string),
            formats::yaml::ParseException,
            "Duplicate mapping key 'true' at path '/'"
        );
    }

    {
        const std::string yaml_string = R"(
            null: foo
            null: foo
        )";
        UEXPECT_THROW_MSG(
            formats::yaml::FromString(yaml_string),
            formats::yaml::ParseException,
            "Duplicate mapping key 'null' at path '/'"
        );
    }
}

TEST(FormatsYamlDuplicateKeys, NonScalarsInMapKeys) {
    const std::string yaml_string = R"(
        [1, 2, 3]: foo
        [1, 2, 3]: bar
    )";
    formats::yaml::Value value;
    // This check relies on implementation of FromString. Currently, we do not throw an exception in this case,
    // but we could do so in the future. In that case the test should be changed.
    // Anyway, we should not hang or crash here.
    UEXPECT_NO_THROW(value = formats::yaml::FromString(yaml_string));
    EXPECT_EQ(value.GetSize(), 2);
}

TEST(FormatsYamlDuplicateKeys, SameKeysInDifferentNestingLevels) {
    const std::string yaml_string = R"(
        Key1: &values
            Key1: &keys
                Key1: 1
                Key2: 2
                Key3: 3
            Key2: *keys
            Key3: *keys
        Key2: *values
    )";
    UEXPECT_NO_THROW(formats::yaml::FromString(yaml_string));
}

USERVER_NAMESPACE_END
