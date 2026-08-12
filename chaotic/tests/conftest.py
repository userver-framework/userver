from typing import Any

import pytest

from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import parser
from chaotic.front import types as front_types


@pytest.fixture
def schema_parser():
    config = parser.ParserConfig(erase_prefix='')
    return parser.SchemaParser(
        config=config,
        full_filepath='full',
        full_vfilepath='vfull',
    )


@pytest.fixture
def clear_source_location():
    def func(child: front_types.Schema, _) -> None:
        child.source_location_ = None

    return func


@pytest.fixture
def simple_parse(clear_source_location):
    def func(input_: dict, clear=True):
        config = parser.ParserConfig(erase_prefix='')
        schema_parser = parser.SchemaParser(
            config=config,
            full_filepath='full',
            full_vfilepath='vfull',
        )
        schema_parser.parse_schema('/definitions/type', input_)
        parsed = schema_parser.parsed_schemas()
        if clear:
            for schema in parsed.schemas.values():
                schema.visit_children(clear_source_location)
                clear_source_location(schema, None)
        return parsed

    return func


@pytest.fixture
def cpp_primitive_type():
    """Factory fixture for creating CppPrimitiveType instances with common defaults."""

    def create(
        validators: cpp_types.CppPrimitiveValidator,
        raw_cpp_type_str: str,
        user_cpp_type: str | None = None,
        json_schema: front_types.Schema | None = None,
        nullable: bool = False,
        default: Any = None,
    ):
        if json_schema is None:
            json_schema = front_types.Schema()
        kwargs = {
            'raw_cpp_type': type_name.TypeName(raw_cpp_type_str),
            'user_cpp_type': user_cpp_type,
            'json_schema': json_schema,
            'nullable': nullable,
            'validators': validators,
        }

        if default is not None:
            kwargs['default'] = default

        return cpp_types.CppPrimitiveType(**kwargs)

    return create
