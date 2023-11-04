#include <gtest/gtest.h>

#include <userver/formats/json/validate.hpp>
#include <userver/formats/json/value.hpp>

USERVER_NAMESPACE_BEGIN

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
                    "width": {"type": "number"},
                    "height": {"type": "number"}
                },
                "required": ["length", "width", "height"]
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
            "width": 12.0,
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
            "width": 1.0,
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
            "width": 12.0,
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
            "width": 1.0,
            "height": 1.0
        },
        "warehouseLocation": {
            "latitude": 54.4,
            "longitude": -32.7
        }
    }
]
)"};

TEST(FormatsJsonValidate, ValidInput) {
  auto schemaDocument = formats::json::FromString(kSchemaJson);
  auto jsonDocument = formats::json::FromString(kValidInputJson);
  formats::json::Schema schema(schemaDocument);
  EXPECT_TRUE(formats::json::Validate(jsonDocument, schema));
}

TEST(FormatsJsonValidate, InvalidInput) {
  auto schemaDocument = formats::json::FromString(kSchemaJson);
  auto jsonDocument = formats::json::FromString(kInvalidInputJson);

  formats::json::Schema schema(schemaDocument);
  EXPECT_FALSE(formats::json::Validate(jsonDocument, schema));
}

USERVER_NAMESPACE_END
