from chaotic.back.cpp import translator
from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as front_types


def _make_const(
    const_value,
    cpp_type,
    prefix='type',
    namespace='',
):
    return cpp_types.CppConstType(
        raw_cpp_type=type_name.TypeName('::type'),
        json_schema=front_types.Schema(),
        nullable=False,
        user_cpp_type=None,
        const_value=const_value,
        cpp_type=cpp_type,
        prefix=prefix,
        namespace=namespace,
    )


def test_string_const(simple_gen):
    types = simple_gen({'type': 'string', 'const': 'active'})
    assert types == {'::type': _make_const('active', 'std::string')}


def test_integer_const(simple_gen):
    types = simple_gen({'type': 'integer', 'const': 42})
    assert types == {'::type': _make_const(42, 'int')}


def test_integer_const_int32(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int32', 'const': 42})
    assert types == {'::type': _make_const(42, 'std::int32_t')}


def test_integer_const_int64(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int64', 'const': 9000000000})
    assert types == {'::type': _make_const(9000000000, 'std::int64_t')}


def test_boolean_const_true(simple_gen):
    types = simple_gen({'type': 'boolean', 'const': True})
    assert types == {'::type': _make_const(True, 'bool')}


def test_boolean_const_false(simple_gen):
    types = simple_gen({'type': 'boolean', 'const': False})
    assert types == {'::type': _make_const(False, 'bool')}


def test_standalone_string_const(simple_gen):
    types = simple_gen({'const': 'active'})
    assert types == {'::type': _make_const('active', 'std::string')}


def test_standalone_integer_const(simple_gen):
    types = simple_gen({'const': 42})
    assert types == {'::type': _make_const(42, 'int')}


def test_standalone_boolean_const(simple_gen):
    types = simple_gen({'const': True})
    assert types == {'::type': _make_const(True, 'bool')}


def test_parser_type_string(simple_gen):
    types = simple_gen({'type': 'string', 'const': 'active'})
    cpp_type = types['::type']
    # prefix is in_local_scope() = 'type' (lowercase), so constexpr is ktypeConst
    assert cpp_type.parser_type('', '') == ('USERVER_NAMESPACE::chaotic::ConstValue<::ktypeConst>')


def test_parser_type_integer(simple_gen):
    types = simple_gen({'type': 'integer', 'const': 42})
    cpp_type = types['::type']
    assert cpp_type.parser_type('', '') == ('USERVER_NAMESPACE::chaotic::ConstValue<::ktypeConst>')


def test_parser_type_int64(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int64', 'const': 9000000000})
    cpp_type = types['::type']
    assert cpp_type.parser_type('', '') == ('USERVER_NAMESPACE::chaotic::ConstValue<::ktypeConst>')


def test_const_declaration_type_string(simple_gen):
    types = simple_gen({'type': 'string', 'const': 'active'})
    cpp_type = types['::type']
    assert cpp_type.const_declaration_type() == 'USERVER_NAMESPACE::utils::StringLiteral'


def test_const_declaration_type_bool(simple_gen):
    types = simple_gen({'type': 'boolean', 'const': True})
    cpp_type = types['::type']
    assert cpp_type.const_declaration_type() == 'bool'


def test_const_declaration_type_int64(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int64', 'const': 1})
    cpp_type = types['::type']
    assert cpp_type.const_declaration_type() == 'std::int64_t'


def test_definition_includes(simple_gen):
    types = simple_gen({'type': 'string', 'const': 'active'})
    assert 'userver/chaotic/const_value.hpp' in types['::type'].definition_includes()


def test_declaration_includes_string(simple_gen):
    types = simple_gen({'type': 'string', 'const': 'active'})
    assert 'userver/utils/string_literal.hpp' in types['::type'].declaration_includes()


def test_declaration_includes_int64(simple_gen):
    types = simple_gen({'type': 'integer', 'format': 'int64', 'const': 1})
    assert 'cstdint' in types['::type'].declaration_includes()


def test_declaration_includes_int(simple_gen):
    types = simple_gen({'type': 'integer', 'const': 1})
    assert types['::type'].declaration_includes() == ['userver/chaotic/const_value.hpp']


def test_get_const_value_string(simple_gen):
    types = simple_gen({'type': 'string', 'const': 'active'})
    assert types['::type'].get_const_value() == 'R"--(active)--"'


def test_get_const_value_bool(simple_gen):
    types = simple_gen({'type': 'boolean', 'const': False})
    assert types['::type'].get_const_value() == 'false'


def test_const_user_cpp_type_is_not_extracted():
    schema = front_types.ConstSchema(
        const='active',
        const_type=front_types.ConstType.STRING,
        x_properties={'x-usrv-cpp-type': 'CustomStatus'},
    )

    gen = translator.Generator(
        config=translator.GeneratorConfig(
            namespaces={},
            infile_to_name_func=lambda path: path,
            include_dirs=[],
            autodiscover_default_dict=False,
        ),
    )

    cpp_type = gen._gen_const(type_name.TypeName('::type'), schema)  # pylint: disable=protected-access
    assert cpp_type.user_cpp_type is None
    assert cpp_type.cpp_user_name() == '::type'
