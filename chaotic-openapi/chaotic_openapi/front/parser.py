import dataclasses
import re
import typing
from typing import Any
from typing import Callable
from typing import Dict
from typing import List
from typing import Optional
from typing import Tuple
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
        assert False, schema

    def _convert_openapi_header(
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
        return self._convert_openapi_parameter(parameter, infile_path)

    def _convert_swagger_header(self, name: str, header: swagger.Header, infile_path: str) -> model.Parameter:
        header_dict = header.model_dump(by_alias=True, exclude_none=True)
        return model.Parameter(
            name=name,
            in_=model.In.header,
            description=header.description or '',
            examples={},
            deprecated=False,
            required=False,
            allowEmptyValue=False,
            style=model.Style.simple,
            schema=self._parse_schema(header_dict, infile_path + '/schema'),
        )

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
        if ref.startswith('#'):
            return self._state.full_filepath + ref
        cur = re.sub(self.RIGHT_SLASH_RE, '/', self._state.full_filepath)
        return chaotic_parser.SchemaParser._normalize_ref(cur + ref)

    def _convert_openapi_parameter(
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

    def _is_swagger_request_body(
        self,
        parameter: Union[swagger.Parameter, swagger.Ref],
        global_params: Dict[str, Union[model.Parameter, List[model.RequestBody]]],
    ) -> bool:
        if isinstance(parameter, swagger.Ref):
            return isinstance(global_params[self._locate_ref(parameter.ref)], list)

        return parameter.in_ in [swagger.In.body, swagger.In.formData]

    def _convert_swagger_request_body(
        self,
        request_body: Union[swagger.Parameter, swagger.Ref],
        infile_path: str,
        consumes: List[str] = [],
    ) -> Union[List[model.RequestBody], model.Ref]:
        if isinstance(request_body, swagger.Ref):
            ref = ref_resolver.normalize_ref(
                self._state.full_filepath,
                request_body.ref,
            )
            return model.Ref(ref)

        if request_body.in_ == swagger.In.body:
            return [
                model.RequestBody(
                    content_type=mime,
                    schema=self._parse_schema(request_body.schema_, infile_path + '/schema'),
                    required=True,
                )
                for mime in consumes
            ]

        assert request_body.in_ == swagger.In.formData

        schema = self._parse_schema(
            request_body.model_dump(
                by_alias=True,
                exclude={'name', 'in_', 'description', 'required', 'allowEmptyValue', 'collectionFormat'},
                exclude_unset=True,
            ),
            infile_path,
        )
        return [
            model.RequestBody(
                content_type=mime,
                schema=schema,
                required=True,
            )
            for mime in consumes
        ]

    def _convert_swagger_parameter(
        self, parameter: Union[swagger.Parameter, swagger.Ref], infile_path: str
    ) -> model.Parameter:
        if isinstance(parameter, swagger.Ref):
            return self._state.service.parameters[self._locate_ref(parameter.ref)]

        schema = self._parse_schema(
            parameter.model_dump(
                by_alias=True,
                exclude={'name', 'in_', 'description', 'required', 'allowEmptyValue', 'collectionFormat'},
                exclude_unset=True,
            ),
            infile_path,
        )

        style: model.Style
        if parameter.in_ == swagger.In.query:
            style = model.Style.form
        else:
            style = model.Style.simple

        p = model.Parameter(
            name=parameter.name,
            in_=model.In(parameter.in_),
            description=parameter.description or '',
            examples={},
            deprecated=False,
            required=parameter.required,
            allowEmptyValue=parameter.allowEmptyValue,
            style=style,
            schema=schema,
        )

        return p

    def _convert_openapi_response(
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
                name: self._convert_openapi_header(name, header, infile_path + f'/headers/{name}')
                for name, header in response.headers.items()
            },
            content=content,  # TODO
        )

    def _convert_swagger_response(
        self, response: Union[swagger.Response, swagger.Ref], produces: List[str], infile_path: str
    ) -> Union[model.Response, model.Ref]:
        assert infile_path.count('#') <= 1

        if isinstance(response, swagger.Ref):
            ref = ref_resolver.normalize_ref(
                self._state.full_filepath,
                response.ref,
            )
            assert ref.count('#') == 1, ref
            return model.Ref(ref)

        if response.schema_:
            schema = self._parse_schema(response.schema_, infile_path + '/schema')
            content = {
                mime: model.MediaType(schema=schema, examples=response.examples.get(mime, {})) for mime in produces
            }
        else:
            content = {}
        return model.Response(
            description=response.description,
            headers={
                name: self._convert_swagger_header(name, header, infile_path + f'/headers/{name}')
                for name, header in response.headers.items()
            },
            content=content,
        )

    def _convert_openapi_request_body(
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

    def _convert_openapi_flows(self, flows: openapi.OAuthFlows) -> List[model.Flow]:
        model_flows: List[model.Flow] = []
        if flows.implicit:
            implicit = flows.implicit
            refreshUrl = implicit.refreshUrl or ''
            model_flows.append(model.ImplicitFlow(refreshUrl, implicit.scopes, implicit.authorizationUrl))
        if flows.password:
            pwd = flows.password
            refreshUrl = pwd.refreshUrl or ''
            model_flows.append(model.PasswordFlow(refreshUrl, pwd.scopes, pwd.tokenUrl))
        if flows.clientCredentials:
            creds: openapi.ClientCredFlow = flows.clientCredentials
            refreshUrl = creds.refreshUrl or ''
            model_flows.append(model.ClientCredFlow(refreshUrl, creds.scopes, creds.tokenUrl))
        if flows.authorizationCode:
            authCode = flows.authorizationCode
            refreshUrl = authCode.refreshUrl or ''
            model_flows.append(
                model.AuthCodeFlow(refreshUrl, authCode.scopes, authCode.authorizationUrl, authCode.tokenUrl)
            )

        return model_flows

    def _convert_openapi_securuty(
        self, security_scheme: Union[openapi.SecurityScheme, openapi.Ref], flows_scopes: Optional[List[str]] = None
    ) -> model.Security:
        if isinstance(security_scheme, openapi.Ref):
            return self._state.service.security[self._locate_ref(security_scheme.ref)]

        description = security_scheme.description or ''
        if security_scheme.type == openapi.SecurityType.http:
            assert security_scheme.scheme_
            return model.HttpSecurity(description, security_scheme.scheme_, security_scheme.bearerFormat)
        elif security_scheme.type == openapi.SecurityType.apiKey:
            assert security_scheme.name
            assert security_scheme.in_
            security_in = model.SecurityIn(security_scheme.in_.name)
            return model.ApiKeySecurity(description, security_scheme.name, security_in)
        elif security_scheme.type == openapi.SecurityType.oauth2:
            assert security_scheme.flows
            flows = self._convert_openapi_flows(security_scheme.flows)
            if flows_scopes:
                for flow in flows:
                    flow.scopes = {key: flow.scopes[key] for key in flows_scopes if key in flow.scopes}
            return model.OAuthSecurity(description, flows)
        elif security_scheme.type == openapi.SecurityType.openIdConnect:
            assert security_scheme.openIdConnectUrl
            return model.OpenIdConnectSecurity(description, security_scheme.openIdConnectUrl)
        else:
            assert False

    def _convert_swagger_security(
        self, security_def: swagger.SecurityDef, flows_scopes: Optional[List[str]] = None
    ) -> model.Security:
        description = security_def.description or ''
        if security_def.type == swagger.SecurityType.basic:
            return model.Security(description)
        elif security_def.type == swagger.SecurityType.apiKey:
            assert security_def.name
            assert security_def.in_
            security_in = model.SecurityIn(security_def.in_.name)
            return model.ApiKeySecurity(description, security_def.name, security_in)
        elif security_def.type == swagger.SecurityType.oauth2:
            flow: model.Flow
            if security_def.flow == swagger.OAuthFlow.implicit:
                assert security_def.authorizationUrl
                flow = model.ImplicitFlow('', security_def.scopes, security_def.authorizationUrl)
            elif security_def.flow == swagger.OAuthFlow.password:
                assert security_def.tokenUrl
                flow = model.PasswordFlow('', security_def.scopes, security_def.tokenUrl)
            elif security_def.flow == swagger.OAuthFlow.application:
                assert security_def.tokenUrl
                flow = model.ClientCredFlow('', security_def.scopes, security_def.tokenUrl)
            elif security_def.flow == swagger.OAuthFlow.accessCode:
                assert security_def.authorizationUrl
                assert security_def.tokenUrl
                flow = model.AuthCodeFlow('', security_def.scopes, security_def.authorizationUrl, security_def.tokenUrl)
            else:
                assert False

            if flows_scopes:
                flow.scopes = {key: flow.scopes[key] for key in flows_scopes if key in flow.scopes}
            return model.OAuthSecurity(description, [flow])
        else:
            assert False

    def _append_schema(
        self,
        parsed: Union[openapi.OpenApi, swagger.Swagger],
    ) -> None:
        components_schemas: Dict[str, Any] = {}
        components_schemas_path = ''
        if isinstance(parsed, openapi.OpenApi):
            parsed = typing.cast(openapi.OpenApi, parsed)
            components_schemas = parsed.components.schemas
            components_schemas_path = '/components/schemas'

            # components/headers
            for name, header in parsed.components.headers.items():
                infile_path = '/components/headers/' + name
                self._state.service.headers[self._state.full_filepath + '#' + infile_path] = (
                    self._convert_openapi_header(
                        name,
                        header,
                        infile_path,
                    )
                )

            # components/securitySchemes
            default_security = parsed.security
            security_schemas = parsed.components.securitySchemes
            for name, sec_scheme in security_schemas.items():
                infile_path = f'/components/securitySchemes/{name}'
                security_scheme = self._convert_openapi_securuty(sec_scheme)
                self._state.service.security[self._state.full_filepath + '#' + infile_path] = security_scheme

            def _convert_op_security(security: Optional[openapi.Security]) -> List[model.Security]:
                if not security:
                    security = default_security

                securities: List[model.Security] = []
                for name, scopes in security.items():
                    securities.append(self._convert_openapi_securuty(security_schemas[name], scopes))

                return securities

            # components/requestBodies
            for name, requestBody in parsed.components.requestBodies.items():
                infile_path = f'/components/requestBodies/{name}'
                body = self._convert_openapi_request_body(
                    requestBody,
                    infile_path,
                )
                assert not isinstance(body, model.Ref)
                self._state.service.requestBodies[self._state.full_filepath + '#' + infile_path] = body

            # components/parameters
            for name, parameter in parsed.components.parameters.items():
                infile_path = '/components/parameters/' + name
                self._state.service.parameters[self._state.full_filepath + '#' + infile_path] = (
                    self._convert_openapi_parameter(parameter, infile_path)
                )

            # components/responses
            for name, response in parsed.components.responses.items():
                infile_path = f'/components/responses/{name}'
                model_resp = self._convert_openapi_response(response, infile_path)
                assert isinstance(model_resp, model.Response)
                self._state.service.responses[self._state.full_filepath + '#' + infile_path] = model_resp

            # paths
            for path, path_item in parsed.paths.items():
                infile_path = f'/paths/[{path}]'
                path_params: Dict[Tuple[str, model.In], model.Parameter] = {}
                for i, path_parameter in enumerate(path_item.parameters):
                    param = self._convert_openapi_parameter(path_parameter, infile_path + f'/parameters/{i}')
                    path_params[(param.name, param.in_)] = param

                self._append_openapi_operation(path, 'get', path_item.get, _convert_op_security, path_params)
                self._append_openapi_operation(path, 'post', path_item.post, _convert_op_security, path_params)
                self._append_openapi_operation(path, 'put', path_item.put, _convert_op_security, path_params)
                self._append_openapi_operation(path, 'delete', path_item.delete, _convert_op_security, path_params)
                self._append_openapi_operation(path, 'options', path_item.options, _convert_op_security, path_params)
                self._append_openapi_operation(path, 'head', path_item.head, _convert_op_security, path_params)
                self._append_openapi_operation(path, 'patch', path_item.patch, _convert_op_security, path_params)
                self._append_openapi_operation(path, 'trace', path_item.trace, _convert_op_security, path_params)
            self._make_sure_operations_are_unique()
        elif isinstance(parsed, swagger.Swagger):
            parsed = typing.cast(swagger.Swagger, parsed)
            components_schemas = parsed.definitions
            components_schemas_path = '/definitions'

            # consumes
            consumes = parsed.consumes

            # parameters
            global_params: Dict[str, Union[model.Parameter, List[model.RequestBody]]] = {}
            for name, sw_parameter in parsed.parameters.items():
                if self._is_swagger_request_body(sw_parameter, global_params):
                    infile_path = '/requestBodies/' + name
                    sw_bodies = self._convert_swagger_request_body(sw_parameter, infile_path, consumes)
                    assert not isinstance(sw_bodies, model.Ref)
                    self._state.service.requestBodies[self._state.full_filepath + '#' + infile_path] = sw_bodies
                    global_params[self._state.full_filepath + f'#/parameters/{name}'] = sw_bodies
                else:
                    infile_path = '/parameters/' + name
                    sw_param = self._convert_swagger_parameter(sw_parameter, infile_path)
                    self._state.service.parameters[self._state.full_filepath + '#' + infile_path] = sw_param
                    global_params[self._state.full_filepath + f'#/parameters/{name}'] = sw_param

            # produces
            produces = parsed.produces

            # responses
            for name, sw_response in parsed.responses.items():
                infile_path = f'/responses/{name}'
                model_resp = self._convert_swagger_response(sw_response, produces, infile_path)
                assert isinstance(model_resp, model.Response)
                self._state.service.responses[self._state.full_filepath + '#' + infile_path] = model_resp

            # securityDefinitions
            default_security = parsed.security
            security_defs = parsed.securityDefinitions
            for name, sec_def in security_defs.items():
                infile_path = f'/securityDefinitions/{name}'
                security_def = self._convert_swagger_security(sec_def)
                self._state.service.security[self._state.full_filepath + '#' + infile_path] = security_def

            def _convert_op_security(security: Optional[swagger.Security]) -> List[model.Security]:
                if not security:
                    security = default_security

                securities: List[model.Security] = []
                for name, scopes in security.items():
                    securities.append(self._convert_swagger_security(security_defs[name], scopes))

                return securities

            # paths
            for sw_path, sw_path_item in parsed.paths.items():
                infile_path = f'/paths/[{sw_path}]'
                sw_path_params: Dict[Tuple[str, model.In], model.Parameter] = {}
                sw_path_body: Union[List[model.RequestBody], model.Ref] = []
                for i, sw_path_parameter in enumerate(sw_path_item.parameters):
                    if self._is_swagger_request_body(sw_path_parameter, global_params):
                        sw_path_body = self._convert_swagger_request_body(
                            sw_path_parameter, infile_path + f'/requestBodies/{i}'
                        )
                    else:
                        sw_param = self._convert_swagger_parameter(sw_path_parameter, infile_path + f'/parameters/{i}')
                        sw_path_params[(sw_param.name, sw_param.in_)] = sw_param

                def _convert_op_params(
                    op_params: swagger.Parameters,
                    infile_path: str,
                    consumes: List[str],
                ) -> Tuple[List[model.Parameter], Union[List[model.RequestBody], model.Ref]]:
                    params = sw_path_params.copy()
                    body = sw_path_body
                    for i, sw_parameter in enumerate(op_params):
                        if self._is_swagger_request_body(sw_parameter, global_params):
                            body = self._convert_swagger_request_body(
                                sw_parameter, infile_path + '/requestBody', consumes
                            )
                        else:
                            param = self._convert_swagger_parameter(sw_parameter, infile_path + f'/parameters/{i}')
                            params[(param.name, param.in_)] = param

                    return list(params.values()), body

                self._append_swagger_operation(
                    parsed.basePath + sw_path, 'get', sw_path_item.get, _convert_op_security, _convert_op_params
                )
                self._append_swagger_operation(
                    parsed.basePath + sw_path, 'post', sw_path_item.post, _convert_op_security, _convert_op_params
                )
                self._append_swagger_operation(
                    parsed.basePath + sw_path, 'put', sw_path_item.put, _convert_op_security, _convert_op_params
                )
                self._append_swagger_operation(
                    parsed.basePath + sw_path, 'delete', sw_path_item.delete, _convert_op_security, _convert_op_params
                )
                self._append_swagger_operation(
                    parsed.basePath + sw_path, 'options', sw_path_item.options, _convert_op_security, _convert_op_params
                )
                self._append_swagger_operation(
                    parsed.basePath + sw_path, 'head', sw_path_item.head, _convert_op_security, _convert_op_params
                )
                self._append_swagger_operation(
                    parsed.basePath + sw_path, 'patch', sw_path_item.patch, _convert_op_security, _convert_op_params
                )
            self._make_sure_operations_are_unique()
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

    def _make_sure_operations_are_unique(self) -> None:
        seen = set()
        for operation in self._state.service.operations:
            new = (operation.path, operation.method.upper())
            if new in seen:
                raise Exception(f'Operation {operation.method.upper()} {operation.path} is duplicated')
            seen.add(new)

    @staticmethod
    def _gen_operation_id(path: str, method: str) -> str:
        return cpp_names.camel_case((path + '_' + method).replace('/', '_'))

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
                chaotic_parser.SchemaParser._normalize_ref(schema_ref.ref),
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
        security_converter: Callable[[Optional[openapi.Security]], List[model.Security]],
        path_params: Dict[Tuple[str, model.In], model.Parameter],
    ) -> None:
        if not operation:
            return

        infile_path = f'/paths/[{path}]/{method}'

        if operation.requestBody:
            requestBody = self._convert_openapi_request_body(
                operation.requestBody,
                infile_path + '/requestBody',
            )
        else:
            requestBody = []

        params = path_params.copy()
        for i, parameter in enumerate(operation.parameters):
            param = self._convert_openapi_parameter(parameter, infile_path + f'/parameters/{i}')
            params[(param.name, param.in_)] = param

        self._state.service.operations.append(
            model.Operation(
                description=operation.description,
                path=path,
                method=method,
                operationId=(operation.operationId or self._gen_operation_id(path, method)),
                parameters=list(params.values()),
                requestBody=requestBody,
                responses={
                    int(status): self._convert_openapi_response(response, infile_path + f'/responses/{status}')
                    for status, response in operation.responses.items()
                },
                security=security_converter(operation.security),
            )
        )

    def _append_swagger_operation(
        self,
        path: str,
        method: str,
        operation: Optional[swagger.Operation],
        security_converter: Callable[[Optional[swagger.Security]], List[model.Security]],
        params_converter: Callable[
            [swagger.Parameters, str, List[str]],
            Tuple[List[model.Parameter], Union[List[model.RequestBody], model.Ref]],
        ],
    ) -> None:
        if not operation:
            return

        infile_path = f'/paths/[{path}]/{method}'

        params, body = params_converter(operation.parameters, infile_path, operation.consumes)

        self._state.service.operations.append(
            model.Operation(
                description=operation.description,
                path=path,
                method=method,
                operationId=(operation.operationId or self._gen_operation_id(path, method)),
                parameters=params,
                requestBody=body,
                responses={
                    int(status): self._convert_swagger_response(
                        response, operation.produces, infile_path + f'/responses/{status}'
                    )
                    for status, response in operation.responses.items()
                },
                security=security_converter(operation.security),
            )
        )

    def service(self) -> model.Service:
        return self._state.service
