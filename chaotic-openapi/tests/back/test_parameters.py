import pytest

from chaotic.back.cpp import type_name
from chaotic.back.cpp import types as cpp_types
from chaotic.front import types as chaotic_types
from chaotic_openapi.back.cpp.client import types


def test_parameters(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'parameters': [
                        {
                            'in': 'query',
                            'name': 'param',
                            'description': 'parameter description',
                            'schema': {
                                'type': 'integer',
                            },
                        },
                    ],
                    'responses': {},
                },
            },
        },
    }
    assert translate_single_schema(schema).operations[0].parameters == [
        types.Parameter(
            description='parameter description',
            raw_name='param',
            cpp_name='param',
            cpp_type=cpp_types.CppPrimitiveType(
                raw_cpp_type=type_name.TypeName('int'),
                user_cpp_type=None,
                nullable=True,
                json_schema=chaotic_types.Integer(
                    type='integer',
                ),
                validators=cpp_types.CppPrimitiveValidator(
                    prefix='Parameter0',
                    namespace='::test_namespace::root_::get',
                ),
            ),
            parser='openapi::TrivialParameter<openapi::In::kQuery, kparam, int, int>',
            required=False,
            query_log_mode_hide=False,
        ),
    ]


def test_parameters_ref(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'parameters': [
                        {
                            '$ref': '#/components/parameters/Parameter',
                        },
                    ],
                    'responses': {},
                },
            },
        },
        'components': {
            'parameters': {
                'Parameter': {
                    'in': 'query',
                    'name': 'param',
                    'description': 'parameter description',
                    'schema': {
                        'type': 'integer',
                    },
                },
            },
        },
    }
    assert translate_single_schema(schema).operations[0].parameters == [
        types.Parameter(
            description='parameter description',
            raw_name='param',
            cpp_name='param',
            cpp_type=cpp_types.CppPrimitiveType(
                raw_cpp_type=type_name.TypeName('int'),
                user_cpp_type=None,
                nullable=True,
                json_schema=chaotic_types.Integer(
                    type='integer',
                ),
                validators=cpp_types.CppPrimitiveValidator(
                    prefix='ParameterParameter',
                    namespace='::test_namespace',
                ),
            ),
            parser='openapi::TrivialParameter<openapi::In::kQuery, kparam, int, int>',
            required=False,
            query_log_mode_hide=False,
        ),
    ]


def test_parameters_schemas_ref(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'parameters': [
                        {
                            'in': 'query',
                            'name': 'param',
                            'description': 'parameter description',
                            'schema': {
                                '$ref': '#/components/schemas/Parameter',
                            },
                        },
                    ],
                    'responses': {},
                },
            },
        },
        'components': {
            'schemas': {
                'Parameter': {
                    'type': 'integer',
                },
            },
        },
    }
    assert translate_single_schema(schema) == types.ClientSpec(
        client_name='test',
        cpp_namespace='test_namespace',
        dynamic_config='',
        operations=[
            types.Operation(
                method='GET',
                path='/',
                operation_id=None,
                parameters=[
                    types.Parameter(
                        description='parameter description',
                        raw_name='param',
                        cpp_name='param',
                        cpp_type=cpp_types.CppRef(
                            raw_cpp_type=type_name.TypeName('::test_namespace::root_::get::Parameter0'),
                            user_cpp_type=None,
                            nullable=True,
                            json_schema=chaotic_types.Ref(
                                ref='<inline>#/components/schemas/Parameter',
                                indirect=False,
                                self_ref=False,
                                schema_=chaotic_types.Integer(
                                    type='integer',
                                ),
                            ),
                            indirect=False,
                            self_ref=False,
                            orig_cpp_type=cpp_types.CppPrimitiveType(
                                raw_cpp_type=type_name.TypeName('int'),
                                user_cpp_type=None,
                                nullable=False,
                                json_schema=chaotic_types.Integer(
                                    type='integer',
                                ),
                                validators=cpp_types.CppPrimitiveValidator(
                                    prefix='Parameter',
                                    namespace='::test_namespace',
                                ),
                            ),
                        ),
                        parser='openapi::TrivialParameter<openapi::In::kQuery, kparam, int, int>',
                        required=False,
                        query_log_mode_hide=False,
                    ),
                ],
                request_bodies=[],
                responses=[],
            ),
        ],
        internal_schemas={
            '::test_namespace::root_::get::Parameter0': cpp_types.CppRef(
                raw_cpp_type=type_name.TypeName('::test_namespace::root_::get::Parameter0'),
                json_schema=chaotic_types.Ref(
                    ref='<inline>#/components/schemas/Parameter',
                    indirect=False,
                    self_ref=False,
                    schema_=chaotic_types.Integer(
                        type='integer',
                        default=None,
                        nullable=False,
                        minimum=None,
                        maximum=None,
                        exclusiveMinimum=None,
                        exclusiveMaximum=None,
                        enum=None,
                        format=None,
                    ),
                ),
                nullable=True,
                user_cpp_type=None,
                orig_cpp_type=cpp_types.CppPrimitiveType(
                    raw_cpp_type=type_name.TypeName('int'),
                    json_schema=chaotic_types.Integer(
                        type='integer',
                        default=None,
                        nullable=False,
                        minimum=None,
                        maximum=None,
                        exclusiveMinimum=None,
                        exclusiveMaximum=None,
                        enum=None,
                        format=None,
                    ),
                    nullable=False,
                    user_cpp_type=None,
                    default=None,
                    validators=cpp_types.CppPrimitiveValidator(
                        min=None,
                        max=None,
                        exclusiveMin=None,
                        exclusiveMax=None,
                        minLength=None,
                        maxLength=None,
                        pattern=None,
                        namespace='::test_namespace',
                        prefix='Parameter',
                    ),
                ),
                indirect=False,
                self_ref=False,
                cpp_name=None,
            ),
        },
        schemas={
            '::test_namespace::Parameter': cpp_types.CppPrimitiveType(
                raw_cpp_type=type_name.TypeName('int'),
                json_schema=chaotic_types.Integer(
                    type='integer',
                ),
                nullable=False,
                user_cpp_type=None,
                default=None,
                validators=cpp_types.CppPrimitiveValidator(
                    prefix='Parameter',
                    namespace='::test_namespace',
                ),
            ),
        },
    )


def test_parameters_too_complex_schema(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/': {
                'get': {
                    'parameters': [
                        {
                            'in': 'query',
                            'name': 'param',
                            'description': 'parameter description',
                            'schema': {
                                'type': 'object',
                                'properties': {},
                                'additionalProperties': False,
                            },
                        },
                    ],
                    'responses': {},
                },
            },
        },
    }
    with pytest.raises(Exception) as exc:
        translate_single_schema(schema)
    assert (
        str(exc.value)
        == """
===============================================================
Unhandled error while processing <inline>
Path "/paths/[/]/get/parameters/0/schema", Format "jsonschema"
Error:
Unsupported parameter type for parameter "param"
==============================================================="""
    )


def test_dash_in_name(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            '/{a-b-c}': {
                'get': {
                    'parameters': [
                        {
                            'in': 'path',
                            'name': 'a-b-c',
                            'description': 'parameter description',
                            'schema': {
                                'type': 'integer',
                            },
                        },
                    ],
                    'responses': {},
                },
            },
        },
    }
    spec = translate_single_schema(schema)
    assert spec.operations[0].parameters[0].raw_name == 'a_b_c'
    assert spec.operations[0].path == '/{a_b_c}'


def test_regex_in_path(translate_single_schema):
    schema = {
        'openapi': '3.0.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {
            'foo/{a-b-c}/bar': {
                'get': {
                    'parameters': [
                        {
                            'in': 'path',
                            'name': 'a-b-c',
                            'description': 'parameter description',
                            'schema': {
                                'type': 'integer',
                            },
                        },
                    ],
                    'responses': {},
                },
            },
        },
    }
    spec = translate_single_schema(schema)
    assert spec.operations[0].cpp_namespace() == 'foo_a_b_c_bar::get'


USER_TYPE = 'my::AccountId'
USER_TYPE_INCLUDE = 'userver/chaotic/io/my/account_id.hpp'


@pytest.fixture
def user_type_include_dir(tmp_path):
    header = tmp_path / USER_TYPE_INCLUDE
    header.parent.mkdir(parents=True)
    header.touch()
    return str(tmp_path)


def _single_parameter_schema(parameter: dict, components: dict | None = None) -> dict:
    return {
        'openapi': '3.1.0',
        'info': {'title': '', 'version': '1.0'},
        'paths': {'/{param}': {'get': {'parameters': [parameter], 'responses': {}}}},
        'components': components or {},
    }


@pytest.mark.parametrize('in_', ['query', 'path', 'header'])
def test_parameter_user_cpp_type(translate_single_schema, user_type_include_dir, in_):
    schema = _single_parameter_schema({
        'in': in_,
        'name': 'param',
        'required': True,
        'schema': {'type': 'string', 'x-usrv-cpp-type': USER_TYPE},
    })

    parameter = translate_single_schema(schema, [user_type_include_dir]).operations[0].parameters[0]

    assert parameter.cpp_type.cpp_user_name() == USER_TYPE
    assert parameter.parser == (
        f'openapi::TrivialParameter<openapi::In::k{in_.title()}, kparam, std::string, {USER_TYPE}>'
    )
    assert USER_TYPE_INCLUDE in parameter.declaration_includes()


def test_parameter_ref_to_user_cpp_type(translate_single_schema, user_type_include_dir):
    schema = _single_parameter_schema(
        {'in': 'query', 'name': 'param', 'schema': {'$ref': '#/components/schemas/AccountId'}},
        components={'schemas': {'AccountId': {'type': 'string', 'x-usrv-cpp-type': USER_TYPE}}},
    )

    parameter = translate_single_schema(schema, [user_type_include_dir]).operations[0].parameters[0]

    assert parameter.cpp_type.cpp_user_name() == USER_TYPE
    assert parameter.parser.endswith(f', {USER_TYPE}>')
    assert USER_TYPE_INCLUDE in parameter.declaration_includes()
    assert 'optional' in parameter.declaration_includes()


def test_parameter_string_format(translate_single_schema):
    schema = _single_parameter_schema({
        'in': 'path',
        'name': 'param',
        'required': True,
        'schema': {'type': 'string', 'format': 'uuid'},
    })

    parameter = translate_single_schema(schema).operations[0].parameters[0]

    assert parameter.parser == (
        'openapi::TrivialParameter<openapi::In::kPath, kparam, std::string, boost::uuids::uuid>'
    )
    assert 'userver/chaotic/io/boost/uuids/uuid.hpp' in parameter.declaration_includes()


def test_parameter_user_cpp_type_over_string_format(translate_single_schema, user_type_include_dir):
    schema = _single_parameter_schema(
        {'in': 'query', 'name': 'param', 'schema': {'$ref': '#/components/schemas/AccountId'}},
        components={
            'schemas': {'AccountId': {'type': 'string', 'format': 'uuid', 'x-usrv-cpp-type': USER_TYPE}},
        },
    )

    parameter = translate_single_schema(schema, [user_type_include_dir]).operations[0].parameters[0]

    assert parameter.parser.endswith(f', {USER_TYPE}, boost::uuids::uuid>')


def test_parameter_array_items_user_cpp_type_over_string_format(translate_single_schema, user_type_include_dir):
    schema = _single_parameter_schema({
        'in': 'query',
        'name': 'param',
        'schema': {
            'type': 'array',
            'items': {'type': 'string', 'format': 'uuid', 'x-usrv-cpp-type': USER_TYPE},
        },
    })

    parameter = translate_single_schema(schema, [user_type_include_dir]).operations[0].parameters[0]

    assert parameter.parser == (
        f"openapi::ArrayParameter<openapi::In::kQuery, kparam, ',', std::string, {USER_TYPE}, boost::uuids::uuid>"
    )
