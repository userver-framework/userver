import json
import tempfile
from typing import Any

from chaotic import error
from chaotic.back.cpp import types
from chaotic.compilers import dynamic_config
from chaotic.front import types as front_types


def parse_variable_content(
    content: Any,
    varname: str = 'var',
) -> types.CppType:
    compiler = dynamic_config.CompilerBase(strict_parsing_default=False)
    with tempfile.NamedTemporaryFile(mode='w+', encoding='utf-8') as ofile:
        json.dump(content, ofile)
        ofile.flush()

        compiler.parse_variable(ofile.name, varname, namespace='taxi_config')
    return compiler.extract_variable_type()


def test_smoke(cpp_primitive_type):
    var = parse_variable_content({'schema': {'type': 'integer'}, 'default': 1})
    expected = cpp_primitive_type(
        validators=types.CppPrimitiveValidator(
            namespace='::taxi_config::var',
            prefix='VariableTypeRaw',
        ),
        raw_cpp_type_str='int',
    )
    var.json_schema = front_types.Schema()
    assert var == expected


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

    compiler = dynamic_config.CompilerBase(strict_parsing_default=False)
    with tempfile.NamedTemporaryFile(mode='w+', encoding='utf-8') as ofile:
        json.dump(content, ofile)
        ofile.flush()

        compiler.parse_definition(ofile.name, 'var')


def test_indirect():
    parse_variable_content({
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
    })


def test_strong_typedef_dependencies():
    try:
        parse_variable_content({
            'schema': {'type': 'string', 'x-usrv-cpp-typedef-tag': 'xxx'},
        })
        assert False
    except error.BaseError as exc:
        assert (
            'Include file "userver/utils/strong_typedef.hpp" not found' in exc.msg
            or 'Include file "userver/chaotic/io/xxx.hpp" not found' in exc.msg
        )


def test_include_dirs_none_skips_check():
    # `include_dirs=None` disables the "does the header exist" check
    # entirely, which build systems (e.g. ya.make) rely on when
    # they cannot cheaply provide the real include search path at codegen
    # time. Unlike the default `[]`, it must not raise even for a header
    # that does not exist anywhere.
    compiler = dynamic_config.CompilerBase(strict_parsing_default=False)
    with tempfile.NamedTemporaryFile(mode='w+', encoding='utf-8') as ofile:
        json.dump(
            {
                'schema': {'type': 'string', 'x-usrv-cpp-typedef-tag': 'xxx'},
                'default': '',
            },
            ofile,
        )
        ofile.flush()

        compiler.parse_variable(ofile.name, 'var', namespace='taxi_config', include_dirs=None)


def test_default_isomorphic():
    var = parse_variable_content({
        'schema': {
            'type': 'object',
            'additionalProperties': {'type': 'string'},
            'properties': {'__default__': {'type': 'string'}},
        },
        'default': {},
    })
    assert isinstance(var, types.CppStruct)
    assert var.cpp_user_name() == 'USERVER_NAMESPACE::utils::DefaultDict<std::string>', var


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
    assert var.cpp_user_name() == '::taxi_config::kebab_case::VariableTypeRaw'


def test_schema_hash_is_exposed_via_function(tmp_path):
    variable = tmp_path / 'BOOL_FLAG.yaml'
    variable.write_text(json.dumps({'schema': {'type': 'boolean'}, 'default': False}), encoding='utf-8')
    compiler = dynamic_config.Compiler(strict_parsing_default=False)
    compiler.parse_variable(str(variable), 'BOOL_FLAG', namespace='dynamic_config')
    output = tmp_path / 'generated'
    compiler.generate_variable('BOOL_FLAG', str(output), parse_extra_formats=False, namespace='dynamic_config')

    header = (output / 'include/dynamic_config/variables/BOOL_FLAG.hpp').read_text(encoding='utf-8')
    source = (output / 'src/dynamic_config/variables/BOOL_FLAG.cpp').read_text(encoding='utf-8')
    schema_hash = '7cb541e84f226754a46c21c79f131fa2898354e1242456e6fd1c162bce319553'
    assert 'USERVER_NAMESPACE::dynamic_config::SchemaHash GetSchemaHash();' in header
    assert header.count('GetSchemaHash') == 1
    assert '#include <userver/dynamic_config/snapshot.hpp>' in header
    assert schema_hash not in header
    assert 'kSchemaHash' not in header
    normalized_source = ''.join(source.split())
    assert (
        'USERVER_NAMESPACE::dynamic_config::SchemaHashbool_flag::GetSchemaHash(){'
        f'returnUSERVER_NAMESPACE::dynamic_config::SchemaHash{{"{schema_hash}"}};'
        '}'
    ) in normalized_source
    assert 'bool_flag::GetSchemaHash(),' in source
    assert source.count('GetSchemaHash') == 2
    assert 'kSchemaHash' not in source
