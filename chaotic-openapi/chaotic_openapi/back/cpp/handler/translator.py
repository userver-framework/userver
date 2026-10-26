from chaotic import layout
from chaotic_openapi.back.cpp.common import types as common_types
from chaotic_openapi.back.cpp.common.translator import BaseTranslator
from chaotic_openapi.back.cpp.handler.types import ServerSpec
from chaotic_openapi.front import model


class HandlerTranslator(BaseTranslator):
    def __init__(
        self,
        service: model.Service,
        *,
        cpp_namespace: str,
        include_dirs: list[str] | None,
        closure: layout.Closure | None = None,
        include_dir: str | None = None,
    ) -> None:
        super().__init__(
            service,
            spec=ServerSpec(
                service_name=service.name,
                cpp_namespace=cpp_namespace,
                include_dir=include_dir or '',
                closure=closure,
            ),
            include_dirs=include_dirs,
            closure=closure,
        )

    def should_generate_operation(self, op: model.Operation) -> bool:
        return op.x_handler_codegen

    def validate_operation(self, op: common_types.Operation) -> None:
        pass
