from chaotic.back.cpp import types as cpp_types
from chaotic.back.cpp.translator import Generator
from chaotic.back.cpp.translator import GeneratorConfig
from chaotic.front import parser
from chaotic.front import ref_resolver
from chaotic.front import types


def parse(path, input_, external_schemas, external_types):
    config = parser.ParserConfig(erase_prefix='')
    schema_parser = parser.SchemaParser(
        config=config, full_filepath='full', full_vfilepath='vfull',
    )
    schema_parser.parse_schema(path, input_)
    schemas = schema_parser.parsed_schemas()
    rr = ref_resolver.RefResolver()
    resolved_schemas = rr.sort_schemas(
        schemas, external_schemas=external_schemas,
    )
    gen = Generator(
        config=GeneratorConfig(namespaces={'vfull': ''}, include_dirs=None),
    )
    types = gen.generate_types(
        resolved_schemas, external_schemas=external_types,
    )

    return resolved_schemas, types


def test_import(simple_gen):
    ext_schemas, ext_types = parse(
        '/type1', {'type': 'string'}, types.ResolvedSchemas(schemas={}), {},
    )
    assert ext_schemas.schemas == {'vfull#/type1': types.String(type='string')}
    assert ext_types == {
        '/type1': cpp_types.CppPrimitiveType(
            raw_cpp_type='std::string',
            nullable=False,
            user_cpp_type=None,
            json_schema=types.String(type='string'),
            validators=cpp_types.CppPrimitiveValidator(prefix='/type1'),
        ),
    }

    new_schemas, new_types = parse(
        '/type2', {'$ref': 'vfull#/type1'}, ext_schemas, ext_types,
    )
    assert new_schemas.schemas == {
        'vfull#/type2': types.Ref(
            ref='vfull#/type1',
            indirect=False,
            self_ref=False,
            schema=ext_schemas.schemas['vfull#/type1'],
        ),
    }
    assert new_types == {
        '/type2': cpp_types.CppRef(
            orig_cpp_type=ext_types['/type1'],
            indirect=False,
            self_ref=False,
            raw_cpp_type='',
            nullable=False,
            user_cpp_type=None,
            json_schema=new_schemas.schemas['vfull#/type2'],
        ),
    }
