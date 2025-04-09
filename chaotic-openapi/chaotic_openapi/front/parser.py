import dataclasses
import re
import typing
from typing import Any
from typing import Dict
from typing import List
from typing import Optional
from typing import Union

import pydantic

from chaotic import cpp_names
from chaotic.front import parser as chaotic_parser
from chaotic.front import types
from . import errors
from . import model
from . import openapi
from . import ref_resolver
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

    def _convert_header(
        self,
        name: str,
        header: Union[openapi.Header, openapi.Ref],
        infile_path: str,
    ) -> model.Parameter:
        if isinstance(header, openapi.Ref):
            return self._state.service.headers[self._locate_ref(header.ref)]

        # Header is a Parameter w/o name and in
        # We're lazy, convert it using model_dump()
        header_dict = header.model_dump(by_alias=True)
        header_dict['name'] = name
        header_dict['in'] = 'header'

        parameter = openapi.Parameter(**header_dict)
        return self._convert_parameter(parameter, infile_path)

    def _convert_media_type(
        self,
        media_type: openapi.MediaType,
        infile_path: str,
    ) -> model.MediaType:
        assert '#' not in infile_path, infile_path

        schema_ref = self._parse_schema(media_type.schema_, infile_path + '/schema')
        mt = model.MediaType(schema=schema_ref, examples=media_type.examples)
        if media_type.example:
            mt.examples['example'] = media_type.example
        return mt

    RIGHT_SLASH_RE = re.compile('/[^/]*$')

    def _locate_ref(self, ref: str) -> str:
        cur = re.sub(self.RIGHT_SLASH_RE, '/', self._state.full_filepath)
        return self._normalize_ref(cur + ref)

    def _convert_parameter(
        self,
        parameter: Union[openapi.Parameter, openapi.Ref],
        infile_path: str,
    ) -> model.Parameter:
        if isinstance(parameter, openapi.Ref):
            return self._state.service.parameters[self._locate_ref(parameter.ref)]
        p = model.Parameter(
            name=parameter.name,
            in_=model.In(parameter.in_),
            description=parameter.description or '',
            examples=parameter.examples,
            deprecated=parameter.deprecated,
            required=parameter.required,
            allowEmptyValue=parameter.allowEmptyValue,
            style=model.Style(parameter.style),
            schema=self._parse_schema(parameter.schema_, infile_path + '/schema'),
        )
        return p

    def _convert_response(
        self,
        response: Union[openapi.Response, openapi.Ref],
        infile_path: str,
    ) -> Union[model.Response, model.Ref]:
        assert infile_path.count('#') <= 1

        if isinstance(response, openapi.Ref):
            ref = ref_resolver.normalize_ref(
                self._state.full_filepath,
                response.ref,
            )
            assert ref.count('#') == 1, ref
            return model.Ref(ref)

        content = {}
        for content_type, openapi_content in response.content.items():
            content[content_type] = self._convert_media_type(
                openapi_content,
                infile_path + f'/content/{content_type}',
            )
        return model.Response(
            description=response.description,
            headers={
                name: self._convert_header(name, header, infile_path + f'/headers/{name}')
                for name, header in response.headers.items()
            },
            content=content,  # TODO
        )

    def _convert_request_body(
        self,
        request_body: Union[openapi.RequestBody, openapi.Ref],
        infile_path: str,
    ) -> Union[List[model.RequestBody], model.Ref]:
        if isinstance(request_body, openapi.Ref):
            ref = ref_resolver.normalize_ref(
                self._state.full_filepath,
                request_body.ref,
            )
            return model.Ref(ref)

        requestBody = []
        for content_type, media_type in request_body.content.items():
            schema = self._parse_schema(
                media_type.schema_,
                f'{infile_path}/content/[{content_type}]/schema',
            )
            requestBody.append(
                model.RequestBody(
                    content_type=content_type,
                    schema=schema,
                    required=request_body.required,
                )
            )
        return requestBody

    def _append_schema(
        self,
        parsed: Union[openapi.OpenApi, swagger.Swagger],
    ) -> None:
        components_schemas: Dict[str, Any] = {}
        components_schemas_path = ''
        if isinstance(parsed, openapi.OpenApi):
            components_schemas = parsed.components.schemas
            components_schemas_path = '/components/schemas'

            # components/headers
            for name, header in parsed.components.headers.items():
                infile_path = '/components/headers/' + name
                self._state.service.headers[self._state.full_filepath + '#' + infile_path] = self._convert_header(
                    name,
                    header,
                    infile_path,
                )

            # components/requestBodies
            for name, requestBody in parsed.components.requestBodies.items():
                infile_path = f'/components/requestBodies/{name}'
                body = self._convert_request_body(
                    requestBody,
                    infile_path,
                )
                assert not isinstance(body, model.Ref)
                self._state.service.requestBodies[self._state.full_filepath + '#' + infile_path] = body

            # components/parameters
            for name, parameter in parsed.components.parameters.items():
                infile_path = '/components/parameters/' + name
                self._state.service.parameters[self._state.full_filepath + '#' + infile_path] = self._convert_parameter(
                    parameter, infile_path
                )

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
                infile_path = f'/components/responses/{name}'
                self._state.service.responses[self._state.full_filepath + '#' + infile_path] = model.Response(
                    description=response.description,
                    headers={
                        key: self._convert_header(key, value, infile_path + f'/headers/{key}')
                        for (key, value) in response.headers.items()
                    },
                    content={
                        key: self._convert_media_type(value, infile_path + f'/content/{key}')
                        for (key, value) in response.content.items()
                    },
                )

        elif isinstance(parsed, swagger.Swagger):
            parsed = typing.cast(swagger.Swagger, parsed)
            components_schemas = parsed.definitions
            components_schemas_path = '/definitions'

            for sw_path, sw_path_item in parsed.paths.items():
                self._append_swagger_operation(parsed.basePath + sw_path, 'get', sw_path_item.get)
                self._append_swagger_operation(parsed.basePath + sw_path, 'post', sw_path_item.post)
                self._append_swagger_operation(parsed.basePath + sw_path, 'put', sw_path_item.put)
                self._append_swagger_operation(parsed.basePath + sw_path, 'delete', sw_path_item.delete)
                self._append_swagger_operation(parsed.basePath + sw_path, 'options', sw_path_item.options)
                self._append_swagger_operation(parsed.basePath + sw_path, 'head', sw_path_item.head)
                self._append_swagger_operation(parsed.basePath + sw_path, 'patch', sw_path_item.patch)
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
    def _normalize_ref(ref: str) -> str:
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
            ref = types.Ref(
                self._normalize_ref(schema_ref.ref),
                indirect=schema_ref.indirect,
                self_ref=schema_ref.self_ref,
            )
            ref._source_location = schema_ref._source_location  # type: ignore
            return ref
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

        if operation.requestBody:
            requestBody = self._convert_request_body(
                operation.requestBody,
                f'/paths/[{path}]/{method}/requestBody',
            )
        else:
            requestBody = []

        infile_path = f'/paths/[{path}]/{method}'
        self._state.service.operations.append(
            model.Operation(
                description=operation.description,
                path=path,
                method=method,
                operationId=(operation.operationId or self._gen_operation_id(path, method)),
                parameters=[
                    self._convert_parameter(parameter, infile_path + f'/parameters/{num}')
                    for num, parameter in enumerate(operation.parameters)
                ],
                requestBody=requestBody,
                responses={
                    int(status): self._convert_response(response, infile_path + f'/responses/{status}')
                    for status, response in operation.responses.items()
                },
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

        requestBody = []

        body_parameter: Optional[swagger.Parameter] = None
        body_parameter_num = 0
        for i, parameter in enumerate(operation.parameters):
            if parameter.in_ == swagger.In.body:
                body_parameter = parameter
                body_parameter_num = i
        if body_parameter:
            infile_path = f'/paths/[{path}]/{method}/parameters/{body_parameter_num}/schema'
            schema = self._parse_schema(body_parameter.schema_, infile_path)
            requestBody.append(
                model.RequestBody(
                    content_type='application/json',
                    schema=schema,
                    required=True,
                )
            )

        self._state.service.operations.append(
            model.Operation(
                description=operation.description,
                path=path,
                method=method,
                operationId=(operation.operationId or self._gen_operation_id(path, method)),
                parameters=[],  # TODO
                requestBody=requestBody,
                responses={},  # TODO
            )
        )

    def service(self) -> model.Service:
        return self._state.service
