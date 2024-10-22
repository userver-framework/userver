#include <userver/formats/json/schema.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <userver/formats/json/value.hpp>
#include <userver/utest/assert_macros.hpp>

USERVER_NAMESPACE_BEGIN

namespace {

constexpr std::string_view kSchemaJson{R"(
{
    "$schema": "http://json-schema.org/draft-04/schema#",
    "title": "Product set",
    "type": "array",
    "items": {
        "title": "Product",
        "type": "object",
        "properties": {
            "id": {
                "description": "The unique identifier for a product",
                "type": "number"
            },
            "name": {
                "type": "string"
            },
            "price": {
                "type": "number",
                "minimum": 0,
                "exclusiveMinimum": true
            },
            "tags": {
                "type": "array",
                "items": {
                    "type": "string"
                },
                "minItems": 1,
                "uniqueItems": true
            },
            "dimensions": {
                "type": "object",
                "properties": {
                    "length": {"type": "number"},
                    "height": {"type": "number"}
                },
                "required": ["length", "height"]
            },
            "warehouseLocation": {
                "description": "Coordinates of the warehouse with the product"
            }
        },
        "required": ["id", "name", "price"]
    }
}
)"};

constexpr std::string_view kValidInputJson{R"(
[
    {
        "id": 2,
        "name": "An ice sculpture",
        "price": 12.50,
        "tags": ["cold", "ice"],
        "dimensions": {
            "length": 7.0,
            "height": 9.5
        },
        "warehouseLocation": {
            "latitude": -78.75,
            "longitude": 20.4
        }
    },
    {
        "id": 3,
        "name": "A blue mouse",
        "price": 25.50,
        "dimensions": {
            "length": 3.1,
            "height": 1.0
        },
        "warehouseLocation": {
            "latitude": 54.4,
            "longitude": -32.7
        }
    }
]
)"};

constexpr std::string_view kInvalidInputJson{R"(
[
    {
        "id_unknown" : 2,
        "name": "An ice sculpture",
        "price": 12.50,
        "tags": ["cold", "ice"],
        "dimensions": {
            "length": 7.0,
            "height": 9.5
        },
        "warehouseLocation": {
            "latitude": -78.75,
            "longitude": 20.4
        }
    },
    {
        "id": 3,
        "name": "A blue mouse",
        "price": 25.50,
        "dimensions": {
            "length": 3.1,
            "height": 1.0
        },
        "warehouseLocation": {
            "latitude": 54.4,
            "longitude": -32.7
        }
    }
]
)"};

constexpr std::string_view kInvalidInputAtRootJson{"42"};

}  // namespace

TEST(FormatsJsonSchema, ValidInput) {
    auto schema_document = formats::json::FromString(kSchemaJson);
    auto json_document = formats::json::FromString(kValidInputJson);

    const formats::json::Schema schema(schema_document);
    auto validation_result = schema.Validate(json_document);

    EXPECT_FALSE(validation_result.IsError());
    UEXPECT_NO_THROW(std::move(validation_result).ThrowIfError());
}

TEST(FormatsJsonSchema, InvalidInput) {
    auto schema_document = formats::json::FromString(kSchemaJson);
    auto json_document = formats::json::FromString(kInvalidInputJson);

    const formats::json::Schema schema(schema_document);
    auto validation_result = schema.Validate(json_document);

    EXPECT_TRUE(validation_result.IsError());
    UEXPECT_THROW_MSG(
        std::move(validation_result).ThrowIfError(),
        formats::json::SchemaValidationException,
        R"(Error at path '0': {"missing":["id"]})"
    ) << "\n==========\n"
      << "This is a golden test. If message starts to change between RapidJSON "
         "versions, then add #if checks on RapidJSON version";
}

TEST(FormatsJsonSchema, InvalidInputAtRoot) {
    auto schema_document = formats::json::FromString(kSchemaJson);
    auto json_document = formats::json::FromString(kInvalidInputAtRootJson);

    const formats::json::Schema schema(schema_document);
    auto validation_result = schema.Validate(json_document);

    EXPECT_TRUE(validation_result.IsError());
    UEXPECT_THROW_MSG(
        std::move(validation_result).ThrowIfError(),
        formats::json::SchemaValidationException,
        R"(Error at path '/': {"expected":["array"],"actual":"integer"})"
    ) << "\n==========\n"
      << "This is a golden test. If message starts to change between RapidJSON "
         "versions, then add #if checks on RapidJSON version";
}

TEST(FormatsJsonSchema, Sample) {
    /// [sample]
    const formats::json::Schema schema(formats::json::FromString(R"(
    {
      "type": "object",
      "properties": {
        "length": {"type": "number"},
        "height": {"type": "number"}
      },
      "required": ["length", "height"]
    }
  )"));

    {
        const formats::json::Value valid_json = formats::json::FromString(R"(
      {
        "length": 10,
        "height": 30
      }
    )");
        auto result = schema.Validate(valid_json);
        EXPECT_TRUE(result);
        EXPECT_FALSE(result.IsError());
        UEXPECT_NO_THROW(std::move(result).ThrowIfError());
    }

    {
        const formats::json::Value invalid_json = formats::json::FromString(R"(
      {
        "length": "WHAT",
        "height": 30
      }
    )");
        auto result = schema.Validate(invalid_json);
        EXPECT_FALSE(result);
        ASSERT_TRUE(result.IsError());

        const auto error = std::move(result).GetError();
        // The exact format of error details is unspecified.
        EXPECT_THAT(std::string{error.GetValuePath()}, testing::HasSubstr("length"));
        EXPECT_THAT(std::string{error.GetSchemaPath()}, testing::HasSubstr("properties"));
        EXPECT_THAT(
            std::string{error.GetDetailsString()},
            testing::AllOf(testing::HasSubstr("number"), testing::HasSubstr("string"))
        );

        UEXPECT_THROW_MSG(
            error.Throw(),
            formats::json::SchemaValidationException,
            "Error at path 'length': "
            R"({"expected":["number"],"actual":"string"})"
        );
    }
    /// [sample]
}

USERVER_NAMESPACE_END
