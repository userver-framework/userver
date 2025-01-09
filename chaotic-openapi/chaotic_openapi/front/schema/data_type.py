from enum import Enum


class DataType(str, Enum):
    """The data type of a schema is defined by the type keyword

    References:
        - https://swagger.io/docs/specification/data-models/data-types/
        - https://json-schema.org/draft/2020-12/json-schema-validation.html#name-type
    """

    STRING = "string"
    NUMBER = "number"
    INTEGER = "integer"
    BOOLEAN = "boolean"
    ARRAY = "array"
    OBJECT = "object"
    NULL = "null"
