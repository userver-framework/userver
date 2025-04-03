from chaotic import cpp_names
from chaotic.back.cpp import translator as chaotic_translator
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
        )

        for operation in service.operations:
            op = types.Operation(
                method=operation.method.upper(),
                path=operation.path,
                description=operation.description,
                parameters=[self._tr_parameter(parameter) for parameter in operation.parameters],
                request_bodies=[
                    self._tr_request_body(content_type, body) for content_type, body in operation.requestBody.items()
                ],
                responses=[],  # TODO
            )
            self._spec.operations.append(op)

        # TODO: schemas
        # TODO: responses
        # TODO: parameters

    def _tr_request_body(self, content_type: str, request_body: model.RequestBody) -> types.RequestBody:
        # TODO: $ref
        parsed_schemas = chaotic_types.ParsedSchemas(
            schemas={request_body.source_location().stringify(): request_body},
        )
        # TODO: components/schemas (external schemas)
        resolved_schemas = ref_resolver.RefResolver().sort_schemas(parsed_schemas)
        gen = chaotic_translator.Generator(
            chaotic_translator.GeneratorConfig(namespaces={request_body.source_location().filepath: ''})
        )
        gen_types = gen.generate_types(resolved_schemas)

        assert len(gen_types) == 1
        schema = list(gen_types.values())[0]

        return types.RequestBody(
            content_type=content_type,
            schema=schema,
        )

    def _tr_parameter(self, parameter: model.Parameter) -> types.Parameter:
        # TODO: type
        cpp_type = 'int'
        # if not parameter.required:
        #     cpp_type = f'std::optional<{cpp_type}>'

        if isinstance(parameter.schema, chaotic_types.Array):
            assert False
        else:
            in_ = parameter.in_
            in_ = in_[0].title() + in_[1:]
            parser = f'openapi::TrivialParameter<openapi::In::k{in_}, k{parameter.name}, int>'

        # TODO: description
        return types.Parameter(
            raw_name=parameter.name,
            cpp_name=cpp_names.cpp_identifier(parameter.name),
            cpp_type=cpp_type,
            parser=parser,
        )

    def spec(self) -> types.ClientSpec:
        return self._spec
