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
        include_dirs: list[str],
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
                operations=[],
                schemas={},
            ),
            include_dirs=include_dirs,
        )

    def should_generate_operation(self, op: model.Operation) -> bool:
        return op.x_client_codegen

    def validate_operation(self, op: common_types.Operation) -> None:
        pass

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
