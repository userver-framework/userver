import dataclasses
import re
from typing import Any
from typing import Dict
from typing import Optional
from typing import Union

import pydantic

from chaotic import cpp_names
from chaotic.front import parser as chaotic_parser
from chaotic.front import types
from . import errors
from . import model
from . import openapi
from . import swagger


@dataclasses.dataclass
class ParserState:
    service: model.Service
    full_filepath: str = ''


class Parser:
    def __init__(
        self,
        name: str,
    ) -> None:
        self._state = ParserState(service=model.Service(name=name))

    def parse_schema(self, schema: dict, full_filepath: str) -> None:
        self._state.full_filepath = full_filepath
        parser = self._guess_parser(schema)
        try:
            parsed = parser(**schema)
        except pydantic.ValidationError as exc:
            raise errors.convert_error(full_filepath, parser.schema_type(), exc) from None
        self._append_schema(parsed)

    @staticmethod
    def _guess_parser(schema: dict):
        if 'openapi' in schema or 'components' in schema:
            return openapi.OpenApi
        elif 'swagger' in schema or 'definitions' in schema:
            return swagger.Swagger

    def _convert_header(self, header: openapi.Header) -> model.Parameter:
        # TODO:
        return model.Parameter()

    def _convert_media_type(self, media_type: openapi.MediaType) -> model.MediaType:
        schema_ref = self._parse_schema(media_type.schema, 'TODO')
        mt = model.MediaType(schema=schema_ref, examples=media_type.examples)
        if media_type.example:
            mt.examples['example'] = media_type.example
        return mt

    RIGHT_SLASH_RE = re.compile('/[^/]*$')

    def _locate_ref(self, ref: str) -> str:
        cur = re.sub(self.RIGHT_SLASH_RE, '/', self._state.full_filepath)
        return self._normalize_ref(cur + ref)

    def _convert_parameter(self, parameter: Union[openapi.Parameter, openapi.Ref], infile_path: str) -> model.Parameter:
        if isinstance(parameter, openapi.Ref):
            print(parameter, self._state.full_filepath)
            return self._state.service.parameters[self._locate_ref(parameter.ref)]

        p = model.Parameter(
            name=parameter.name,
            in_=model.In(parameter.in_),
            description=parameter.description,
            examples=parameter.examples,
            deprecated=parameter.deprecated,
            required=parameter.required,
            allowEmptyValue=parameter.allowEmptyValue,
            style=model.Style(parameter.style),
            schema=self._parse_schema(parameter.schema_, infile_path),
        )
        return p

    def _append_schema(self, parsed: Union[openapi.OpenApi, swagger.Swagger]) -> None:
        components_schemas: Dict[str, Any] = {}
        components_schemas_path = ''
        if isinstance(parsed, openapi.OpenApi):
            components_schemas = parsed.components.schemas
            components_schemas_path = '/components/schemas'

            for path, path_item in parsed.paths.items():
                self._append_openapi_operation(path, 'get', path_item.get)
                self._append_openapi_operation(path, 'post', path_item.post)
                self._append_openapi_operation(path, 'put', path_item.put)
                self._append_openapi_operation(path, 'delete', path_item.delete)
                self._append_openapi_operation(path, 'options', path_item.options)
                self._append_openapi_operation(path, 'head', path_item.head)
                self._append_openapi_operation(path, 'patch', path_item.patch)
                self._append_openapi_operation(path, 'trace', path_item.trace)

            # components/responses
            for name, response in parsed.components.responses.items():
                self._state.service.responses[self._state.full_filepath + '#/compoents/responses/' + name] = (
                    model.Response(
                        description=response.description,
                        headers={key: self._convert_header(value) for (key, value) in response.headers.items()},
                        content={key: self._convert_media_type(value) for (key, value) in response.content.items()},
                    )
                )

            # components/parameters
            for name, parameter in parsed.components.parameters.items():
                infile_path = '/components/parameters/' + name
                self._state.service.parameters[self._state.full_filepath + '#' + infile_path] = self._convert_parameter(
                    parameter, infile_path
                )

        elif isinstance(parsed, swagger.Swagger):
            components_schemas = parsed.definitions
            components_schemas_path = '/definitions'

            for path, path_item in parsed.paths.items():
                self._append_swagger_operation(parsed.basePath + path, 'get', path_item.get)
                self._append_swagger_operation(parsed.basePath + path, 'post', path_item.post)
                self._append_swagger_operation(parsed.basePath + path, 'put', path_item.put)
                self._append_swagger_operation(parsed.basePath + path, 'delete', path_item.delete)
                self._append_swagger_operation(parsed.basePath + path, 'options', path_item.options)
                self._append_swagger_operation(parsed.basePath + path, 'head', path_item.head)
                self._append_swagger_operation(parsed.basePath + path, 'patch', path_item.patch)
        else:
            assert False

        # components/schemas or definitions
        parser = chaotic_parser.SchemaParser(
            config=chaotic_parser.ParserConfig(erase_prefix=''),
            full_filepath=self._state.full_filepath,
            full_vfilepath=self._state.full_filepath,
        )
        for name, schema in components_schemas.items():
            infile_path = components_schemas_path + '/' + name
            parser.parse_schema(infile_path, schema)
        parsed_schemas = parser.parsed_schemas()
        for name, schema in parsed_schemas.schemas.items():
            self._state.service.schemas[name] = schema

    @staticmethod
    def _gen_operation_id(path: str, method: str) -> str:
        return cpp_names.camel_case((path + '_' + method).replace('/', '_'))

    REF_SHRINK_RE = re.compile('/[^/]+/../')

    @staticmethod
    def _normalize_ref(ref: str) -> None:
        while Parser.REF_SHRINK_RE.search(ref):
            ref = re.sub(Parser.REF_SHRINK_RE, '/', ref)
        return ref

    def _parse_schema(self, schema: Any, infile_path: str) -> Union[types.Schema, types.Ref]:
        parser = chaotic_parser.SchemaParser(
            config=chaotic_parser.ParserConfig(erase_prefix=''),
            full_filepath=self._state.full_filepath,
            full_vfilepath=self._state.full_filepath,
        )
        parser.parse_schema(infile_path, schema)
        parsed_schemas = parser.parsed_schemas()
        assert len(parsed_schemas.schemas) == 1
        schema_ref = list(parsed_schemas.schemas.values())[0]

        if isinstance(schema_ref, types.Ref):
            return types.Ref(
                self._normalize_ref(schema_ref.ref),
                indirect=schema_ref.indirect,
                self_ref=schema_ref.self_ref,
            )
        else:
            return schema_ref

    def _append_openapi_operation(
        self,
        path: str,
        method: str,
        operation: Optional[openapi.Operation],
    ) -> None:
        if not operation:
            return

        requestBody = {}
        if operation.requestBody:
            for content_type, media_type in operation.requestBody.content.items():
                requestBody[content_type] = self._parse_schema(
                    media_type.schema_,
                    f'/paths/{path}/{method}/requestBody/content/[{content_type}]/schema',
                )

        infile_path = f'/paths/{path}/{method}/parameters/'
        self._state.service.operations.append(
            model.Operation(
                description=operation.description,
                path=path,
                method=method,
                operationId=(operation.operationId or self._gen_operation_id(path, method)),
                parameters=[
                    self._convert_parameter(parameter, infile_path + str(num))
                    for num, parameter in enumerate(operation.parameters)
                ],
                requestBody=requestBody,
            )
        )

    def _append_swagger_operation(
        self,
        path: str,
        method: str,
        operation: Optional[swagger.Operation],
    ) -> None:
        if not operation:
            return

        requestBody = {}

        body_parameter: Optional[swagger.Parameter] = None
        body_parameter_num = 0
        for i, parameter in enumerate(operation.parameters):
            if parameter.in_ == swagger.In.body:
                body_parameter = parameter
                body_parameter_num = i
        if body_parameter:
            infile_path = f'/paths/{path}/{method}/parameters/{body_parameter_num}/schema'
            schema = self._parse_schema(body_parameter.schema_, infile_path)
            requestBody['application/json'] = schema

        self._state.service.operations.append(
            model.Operation(
                description=operation.description,
                path=path,
                method=method,
                operationId=(operation.operationId or self._gen_operation_id(path, method)),
                parameters=[],  # TODO
                requestBody=requestBody,
            )
        )

    def service(self) -> model.Service:
        return self._state.service
