import re
import typing
from typing import Union

from chaotic import cpp_names
from chaotic.back.cpp import translator as chaotic_translator
from chaotic.back.cpp import types as cpp_types
from chaotic.front import ref_resolver
from chaotic.front import types as chaotic_types
from chaotic_openapi.back.cpp_client import types
from chaotic_openapi.front import model


class Translator:
    def __init__(self, service: model.Service, cpp_namespace: str) -> None:
        self._spec = types.ClientSpec(
            client_name=service.name,
            description=service.description,
            cpp_namespace=cpp_namespace,
            operations=[],
            schemas={},
        )

        # components/schemas
        parsed_schemas = chaotic_types.ParsedSchemas(
            schemas={str(schema.source_location()): schema for schema in service.schemas.values()},
        )
        resolved_schemas = ref_resolver.RefResolver().sort_schemas(parsed_schemas)
        gen = chaotic_translator.Generator(
            chaotic_translator.GeneratorConfig(
                namespaces={schema.source_location().filepath: '' for schema in service.schemas.values()},
                infile_to_name_func=self.map_infile_path_to_cpp_type,
            )
        )
        self._spec.schemas = gen.generate_types(resolved_schemas)
        self._raw_schemas = {str(schema.source_location()): schema for schema in service.schemas.values()}

        # components/responses
        self._raw_responses = {f'{name}': response for name, response in service.responses.items()}

        # components/requestBodies
        self._raw_request_bodies = {f'{name}': body for name, body in service.requestBodies.items()}

        for operation in service.operations:
            if isinstance(operation.requestBody, model.Ref):
                request_bodies = [
                    self._translate_request_body(body) for body in self._raw_request_bodies[operation.requestBody.ref]
                ]
            else:
                request_bodies = [self._translate_request_body(body) for body in operation.requestBody]

            op = types.Operation(
                method=operation.method.upper(),
                path=operation.path,
                description=operation.description,
                parameters=[self._translate_parameter(parameter) for parameter in operation.parameters],
                request_bodies=request_bodies,
                responses=[self._translate_response(r, status) for status, r in operation.responses.items()],
            )
            self._spec.operations.append(op)

        # TODO: responses
        # TODO: parameters

    def map_infile_path_to_cpp_type(self, name: str) -> str:
        if name.startswith('/components/schemas/'):
            return '{}::{}'.format(
                self._spec.cpp_namespace,
                name.split('/')[-1],
            )

        match = re.fullmatch('/paths/\\[([^\\]]*)\\]/([a-zA-Z]*)/requestBody/content/\\[([^\\]]*)\\]/schema', name)
        if match:
            return '{}::{}_{}::Body{}'.format(
                self._spec.cpp_namespace,
                match.group(1)[1:],
                match.group(2),
                cpp_names.camel_case(
                    cpp_names.cpp_identifier(match.group(3)),
                ),
            )

        match = re.fullmatch(
            '/paths/\\[([^\\]]*)\\]/([a-zA-Z]*)/responses/([0-9]*)/headers/([a-zA-Z0-9_]*)/schema', name
        )
        if match:
            return '{}::{}_{}::Response{}Header{}'.format(
                self._spec.cpp_namespace,
                match.group(1)[:1],
                match.group(2),
                match.group(3),
                cpp_names.camel_case(
                    cpp_names.cpp_identifier(match.group(4)),
                ),
            )

        match = re.fullmatch('/components/responses/([a-zA-Z_0-9]*)/content/(.*)/schema', name)
        if match:
            return '{}::Response{}Body{}'.format(
                self._spec.cpp_namespace,
                match.group(1),
                cpp_names.camel_case(
                    cpp_names.cpp_identifier(match.group(2)),
                ),
            )

        match = re.fullmatch('/paths/\\[([^\\]]*)\\]/([a-zA-Z]*)/parameters/([0-9]*)/schema', name)
        if match:
            return '{}::{}_{}::Parameter{}'.format(
                self._spec.cpp_namespace,
                match.group(1)[1:],
                match.group(2),
                match.group(3),
            )

        match = re.fullmatch('/paths/\\[([^\\]]*)\\]/([a-zA-Z]*)/responses/([0-9]*)/content/(.*)/schema', name)
        if match:
            return '{}::{}_{}::Response{}Body{}'.format(
                self._spec.cpp_namespace,
                match.group(1)[1:],
                match.group(2),
                match.group(3),
                cpp_names.camel_case(
                    cpp_names.cpp_identifier(match.group(4)),
                ),
            )

        match = re.fullmatch('/components/requestBodies/([a-zA-Z0-9_]*)/content/\\[([^\\]]*)\\]/schema', name)
        if match:
            return '{}::{}{}'.format(
                self._spec.cpp_namespace,
                cpp_names.cpp_identifier(match.group(1)),
                cpp_names.camel_case(
                    cpp_names.cpp_identifier(match.group(2)),
                ),
            )

        match = re.fullmatch('/components/parameters/([a-zA-Z0-9_]*)/schema', name)
        if match:
            return '{}::Parameter{}'.format(
                self._spec.cpp_namespace,
                cpp_names.camel_case(
                    cpp_names.cpp_identifier(match.group(1)),
                ),
            )
        match = re.fullmatch('/components/headers/([a-zA-Z0-9]*)/schema', name)
        if match:
            return '{}::Header{}'.format(
                self._spec.cpp_namespace,
                cpp_names.camel_case(
                    cpp_names.cpp_identifier(match.group(1)),
                ),
            )

        assert False, name
        return name

    def _translate_single_schema(self, schema: chaotic_types.Schema) -> cpp_types.CppType:
        parsed_schemas = chaotic_types.ParsedSchemas(
            schemas={str(schema.source_location()): schema},
        )
        # TODO: external components/schemas
        resolved_schemas = ref_resolver.RefResolver().sort_schemas(
            parsed_schemas,
            external_schemas=chaotic_types.ResolvedSchemas(
                self._raw_schemas,
            ),
        )
        gen = chaotic_translator.Generator(
            chaotic_translator.GeneratorConfig(
                namespaces={schema.source_location().filepath: ''},
                infile_to_name_func=self.map_infile_path_to_cpp_type,
            )
        )
        gen_types = gen.generate_types(
            resolved_schemas,
            external_schemas=self._spec.schemas,
        )

        assert len(gen_types) == 1
        return list(gen_types.values())[0]

    def _translate_response(
        self,
        response: Union[model.Response, model.Ref],
        status: int,
    ) -> types.Response:
        if isinstance(response, model.Ref):
            response = self._raw_responses[response.ref]

        headers = []
        for name, header in response.headers.items():
            headers.append(self._translate_parameter(header))

        return types.Response(
            status=status,
            headers=headers,
            body={
                content_type: self._translate_single_schema(schema.schema)
                for content_type, schema in response.content.items()
            },
        )

    def _translate_request_body(self, request_body: model.RequestBody) -> types.Body:
        parsed_schemas = chaotic_types.ParsedSchemas(
            schemas={str(request_body.schema.source_location()): request_body.schema},
        )
        # TODO: components/schemas (external schemas)
        resolved_schemas = ref_resolver.RefResolver().sort_schemas(
            parsed_schemas,
            external_schemas=chaotic_types.ResolvedSchemas(
                self._raw_schemas,
            ),
        )
        gen = chaotic_translator.Generator(
            chaotic_translator.GeneratorConfig(
                namespaces={request_body.schema.source_location().filepath: ''},
                infile_to_name_func=self.map_infile_path_to_cpp_type,
            )
        )
        gen_types = gen.generate_types(
            resolved_schemas,
            external_schemas=self._spec.schemas,
        )

        assert len(gen_types) == 1
        schema = list(gen_types.values())[0]

        return types.Body(
            content_type=request_body.content_type,
            schema=schema,
        )

    def _translate_parameter(self, parameter: model.Parameter) -> types.Parameter:
        in_ = parameter.in_
        in_str = in_[0].title() + in_[1:]

        if isinstance(parameter.schema, chaotic_types.Array):
            delimiter = {
                model.Style.matrix: ',',
                model.Style.label: '.',
                model.Style.form: ',',
                model.Style.simple: ',',
                model.Style.spaceDelimited: ' ',
                model.Style.pipeDelimited: '|',
            }[parameter.style]

            cpp_type = self._translate_single_schema(parameter.schema)
            cpp_type = typing.cast(cpp_types.CppArray, cpp_type)

            raw_item_type = cpp_type.items.cpp_global_name()
            user_item_type = cpp_type.items.cpp_user_name()

            parser = f"openapi::ArrayParameter<openapi::In::k{in_str}, k{parameter.name}, '{delimiter}', {raw_item_type}, {user_item_type}>"
        else:
            cpp_type = self._translate_single_schema(parameter.schema)
            user_type = cpp_type

            parser = f'openapi::TrivialParameter<openapi::In::k{in_str}, k{parameter.name}, {cpp_type.cpp_global_name()}, {user_type.cpp_global_name()}>'

        if not parameter.required:
            cpp_type.nullable = True

        return types.Parameter(
            description=parameter.description,
            raw_name=parameter.name,
            cpp_name=cpp_names.cpp_identifier(parameter.name),
            cpp_type=cpp_type,
            parser=parser,
            required=parameter.required,
        )

    def spec(self) -> types.ClientSpec:
        return self._spec
