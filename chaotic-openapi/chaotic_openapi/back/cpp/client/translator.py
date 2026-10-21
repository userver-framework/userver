from typing import NoReturn

from chaotic import error as chaotic_error
from chaotic_openapi.back.cpp.client import middleware
from chaotic_openapi.back.cpp.client import types
from chaotic_openapi.back.cpp.common import types as common_types
from chaotic_openapi.back.cpp.common.translator import BaseTranslator
from chaotic_openapi.front import base_model
from chaotic_openapi.front import model


class Translator(BaseTranslator):
    def __init__(
        self,
        service: model.Service,
        *,
        cpp_namespace: str,
        dynamic_config: str,
        dynamic_config_schema_hash: str = '',
        include_dirs: list[str] | None,
        middleware_plugins: list[middleware.MiddlewarePlugin],
    ) -> None:
        self._middleware_plugins = middleware_plugins
        super().__init__(
            service,
            spec=types.ClientSpec(
                client_name=service.name,
                description=service.description,
                cpp_namespace=cpp_namespace,
                dynamic_config=dynamic_config,
                dynamic_config_schema_hash=dynamic_config_schema_hash,
                operations=[],
                schemas={},
            ),
            include_dirs=include_dirs,
        )

    def should_generate_operation(self, op: model.Operation) -> bool:
        return op.x_client_codegen

    def validate_operation(self, op: common_types.Operation) -> None:
        if op.method not in common_types.HTTP_METHOD_ENUMERATORS:
            self._raise_unsupported_operation(
                op,
                f'HTTP method "{op.method}" is not supported by the generated client yet. '
                f'Set "x-usrv-client-codegen: false" on the operation to skip it',
            )
        if op.method == common_types.HTTP_METHOD_WITHOUT_REQUEST_BODY and op.sends_request_body():
            self._raise_unsupported_operation(op, 'a HEAD request cannot carry a request body, remove "requestBody"')

    @staticmethod
    def _raise_unsupported_operation(op: common_types.Operation, msg: str) -> NoReturn:
        raise chaotic_error.BaseError(
            full_filepath=op.path,
            infile_path=f'/{op.method.lower()}',
            schema_type='openapi/swagger',
            msg=f'Operation "{op.method} {op.path}": {msg}',
        )

    def _translate_middlewares(self, middlewares: base_model.XMiddlewares) -> list[middleware.Middleware]:
        extra_members = middlewares.__pydantic_extra__ or {}

        result = []
        for plugin in self._middleware_plugins:
            field = plugin.field
            if field not in extra_members:
                continue

            mw = plugin.create({field: extra_members[field]})
            result.append(mw)
            del extra_members[field]

        if extra_members:
            raise Exception(f'Unknown member(s) in x-taxi-middlewares: {extra_members}')

        return result
