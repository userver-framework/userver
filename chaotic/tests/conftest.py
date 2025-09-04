from typing import Any
from typing import Optional

import pytest

from chaotic.back.cpp import type_name
from chaotic.back.cpp.types import CppPrimitiveType
from chaotic.back.cpp.types import CppPrimitiveValidator
from chaotic.front import parser
from chaotic.front.types import Schema


@pytest.fixture
def schema_parser():
    config = parser.ParserConfig(erase_prefix='')
    return parser.SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )


@pytest.fixture
def simple_parse():
    def func(input_: dict):
        config = parser.ParserConfig(erase_prefix='')
        schema_parser = parser.SchemaParser(
            config=config,
            full_filepath='full',
            full_vfilepath='vfull',
        )
        schema_parser.parse_schema('/definitions/type', input_)
        return schema_parser.parsed_schemas()

    return func


@pytest.fixture
def cpp_primitive_type():
    """Factory fixture for creating CppPrimitiveType instances with common defaults."""

    def create(
        validators: CppPrimitiveValidator,
        raw_cpp_type_str: str,
        user_cpp_type: Optional[str] = None,
        json_schema: Optional[Schema] = None,
        nullable: bool = False,
        default: Any = None,
    ):
        kwargs = {
            'raw_cpp_type': type_name.TypeName(raw_cpp_type_str),
            'user_cpp_type': user_cpp_type,
            'json_schema': json_schema,
            'nullable': nullable,
            'validators': validators,
        }

        if default is not None:
            kwargs['default'] = default

        return CppPrimitiveType(**kwargs)

    return create
