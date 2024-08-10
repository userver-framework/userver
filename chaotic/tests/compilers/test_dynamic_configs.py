import json
import tempfile
from typing import Any

from chaotic import error
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types
from chaotic.compilers import dynamic_config


def parse_variable_content(
        content: Any, varname: str = 'var',
) -> types.CppType:
    compiler = dynamic_config.CompilerBase()
    with tempfile.NamedTemporaryFile(mode='w+', encoding='utf-8') as ofile:
        json.dump(content, ofile)
        ofile.flush()

        compiler.parse_variable(ofile.name, varname)
    return compiler.extract_variable_type()


def test_smoke():
    var = parse_variable_content({'schema': {'type': 'integer'}, 'default': 1})
    expected = types.CppPrimitiveType(
        raw_cpp_type=type_name.TypeName('int'),
        nullable=False,
        user_cpp_type=None,
        json_schema=None,
        validators=types.CppPrimitiveValidator(
            namespace='taxi_config::var', prefix='VariableTypeRaw',
        ),
    )
    assert var.without_json_schema() == expected


def test_sort():
    content = {
        'Obj': {
            'oneOf': [{'$ref': '#/Obj1'}],
            'discriminator': {'propertyName': 'foo'},
        },
        'Obj1': {
            'type': 'object',
            'additionalProperties': False,
            'properties': {'foo': {'type': 'string'}},
        },
    }

    compiler = dynamic_config.CompilerBase()
    with tempfile.NamedTemporaryFile(mode='w+', encoding='utf-8') as ofile:
        json.dump(content, ofile)
        ofile.flush()

        compiler.parse_definition(ofile.name, 'var')


def test_indirect():
    parse_variable_content(
        {
            'schema': {
                '$ref': '#/definitions/Obj',
                'definitions': {
                    'Obj': {
                        'type': 'object',
                        'additionalProperties': False,
                        'properties': {
                            'left': {
                                '$ref': '#/definitions/Obj',
                                'x-usrv-cpp-indirect': True,
                            },
                            'right': {
                                '$ref': '#/definitions/Obj',
                                'x-usrv-cpp-indirect': True,
                            },
                        },
                    },
                },
            },
            'default': 1,
        },
    )


def test_strong_typedef_dependencies():
    try:
        parse_variable_content(
            {'schema': {'type': 'string', 'x-usrv-cpp-typedef-tag': 'xxx'}},
        )
        assert False
    except error.BaseError as exc:
        assert 'Include file "userver/chaotic/io/xxx.hpp" not found' in exc.msg


def test_default_isomorphic():
    var = parse_variable_content(
        {
            'schema': {
                'type': 'object',
                'additionalProperties': {'type': 'string'},
                'properties': {'__default__': {'type': 'string'}},
            },
            'default': {},
        },
    )
    assert isinstance(var, types.CppStruct)
    assert (
        var.cpp_user_name()
        == 'USERVER_NAMESPACE::utils::DefaultDict<std::string>'
    )


def test_variable_name_invalid_symbols():
    var = parse_variable_content(
        {
            'schema': {
                'type': 'object',
                'properties': {},
                'additionalProperties': False,
            },
            'default': '',
        },
        varname='kebab-case',
    )
    assert var.cpp_user_name() == 'taxi_config::kebab_case::VariableTypeRaw'
