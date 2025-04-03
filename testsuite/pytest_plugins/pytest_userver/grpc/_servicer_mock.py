import contextlib
import dataclasses
import functools
import inspect
from typing import Any
from typing import Callable
from typing import Collection
from typing import Dict
from typing import NoReturn
from typing import Optional

import google.protobuf.descriptor
import google.protobuf.descriptor_pool
import google.protobuf.message
import grpc

import testsuite.utils.callinfo

from ._mocked_errors import MockedError
from ._reflection import _reflect_servicer

Handler = Callable[[Any, grpc.aio.ServicerContext], Any]
MockDecorator = Callable[[Handler], testsuite.utils.callinfo.AsyncCallQueue]
AsyncExcAppend = Callable[[Exception], None]

_ERROR_CODE_KEY = 'x-testsuite-error-code'
_MethodDescriptor = google.protobuf.descriptor.MethodDescriptor


@dataclasses.dataclass
class _ServiceMockState:
    mocked_methods: Dict[str, Handler] = dataclasses.field(default_factory=dict)
    asyncexc_append: Optional[AsyncExcAppend] = None


class _ServiceMock:
    def __init__(
        self,
        *,
        servicer: object,
        adder: Callable[[object, grpc.aio.Server], None],
        service_name: str,
        known_methods: Collection[str],
        state: _ServiceMockState,
    ) -> None:
        self._servicer = servicer
        self._adder = adder
        self._service_name = service_name
        self._known_methods = known_methods
        self._state = state

    @property
    def servicer(self) -> object:
        return self._servicer

    @property
    def service_name(self) -> str:
        return self._service_name

    @property
    def known_methods(self) -> Collection[str]:
        return self._known_methods

    def add_to_server(self, server: grpc.aio.Server) -> None:
        self._adder(self._servicer, server)

    def reset_handlers(self) -> None:
        self._state.mocked_methods.clear()

    def install_handler(self, method_name: str, /) -> MockDecorator:
        def decorator(func):
            if method_name not in self._known_methods:
                raise RuntimeError(
                    f'Trying to mock unknown grpc method {method_name} in service {self._service_name}. '
                    f'Known methods are: {", ".join(self._known_methods)}',
                )

            wrapped = testsuite.utils.callinfo.acallqueue(func)
            self._state.mocked_methods[method_name] = wrapped
            return wrapped

        return decorator

    def set_asyncexc_append(self, asyncexc_append: Optional[AsyncExcAppend]) -> None:
        self._state.asyncexc_append = asyncexc_append


async def _raise_unimplemented_error(
    context: grpc.aio.ServicerContext,
    method_descriptor: _MethodDescriptor,
    state: _ServiceMockState,
) -> NoReturn:
    message = f"Trying to call a missing grpc mock for '{_get_full_method_name(method_descriptor)}' method"
    if state.asyncexc_append is not None:
        state.asyncexc_append(NotImplementedError(message))
    # This error is identical to the builtin pytest error.
    await context.abort(grpc.StatusCode.UNIMPLEMENTED, 'Method not found!')


def _is_error_status_set(context: grpc.aio.ServicerContext) -> bool:
    code = context.code()
    return code != grpc.StatusCode.OK and code is not None and code != 0


@contextlib.asynccontextmanager
async def _handle_exceptions(context: grpc.aio.ServicerContext, state: _ServiceMockState):
    try:
        yield
    except MockedError as exc:
        await context.abort(code=grpc.StatusCode.UNKNOWN, trailing_metadata=((_ERROR_CODE_KEY, exc.ERROR_CODE),))
    except Exception as exc:  # pylint: disable=broad-except
        if not _is_error_status_set(context) and state.asyncexc_append is not None:
            state.asyncexc_append(exc)
        raise


def _wrap_grpc_method(
    python_method_name: str,
    method_descriptor: _MethodDescriptor,
    default_unimplemented_method: Handler,
    state: _ServiceMockState,
):
    if method_descriptor.server_streaming:

        @functools.wraps(default_unimplemented_method)
        async def run_stream_response_method(self, request_or_stream, context: grpc.aio.ServicerContext):
            method = state.mocked_methods.get(python_method_name)
            if method is None:
                await _raise_unimplemented_error(context, method_descriptor, state)

            async with _handle_exceptions(context, state):
                async for response in await method(request_or_stream, context):
                    yield _check_is_valid_response(response, method_descriptor)

        return run_stream_response_method
    else:

        @functools.wraps(default_unimplemented_method)
        async def run_unary_response_method(self, request_or_stream, context: grpc.aio.ServicerContext):
            async with _handle_exceptions(context, state):
                method = state.mocked_methods.get(python_method_name)
                if method is None:
                    await _raise_unimplemented_error(context, method_descriptor, state)

                response = method(request_or_stream, context)
                if inspect.isawaitable(response):
                    response = await response
                return _check_is_valid_response(response, method_descriptor)

        return run_unary_response_method


def _check_is_servicer_class(servicer_class: type) -> None:
    if not isinstance(servicer_class, type):
        raise ValueError(f'Expected *Servicer class (type), got: {servicer_class} ({type(servicer_class)})')
    if not servicer_class.__name__.endswith('Servicer'):
        raise ValueError(f'Expected *Servicer class, got: {servicer_class}')


def _check_is_valid_response(
    response: object,
    method_descriptor: _MethodDescriptor,
) -> Optional[google.protobuf.message.Message]:
    if not isinstance(response, google.protobuf.message.Message):
        raise ValueError(
            f'In grpc_mockserver handler for {_get_full_method_name(method_descriptor)}: '
            'Expected a protobuf Message response, '
            f'got: {response!r} ({type(response).__qualname__})'
        )
    descriptor = type(response).DESCRIPTOR
    assert isinstance(descriptor, google.protobuf.descriptor.Descriptor)
    if descriptor.full_name != method_descriptor.output_type.full_name:
        raise ValueError(
            f'In grpc_mockserver handler for {_get_full_method_name(method_descriptor)}: '
            f'Expected a response of type "{method_descriptor.output_type.full_name}", '
            f'got: "{descriptor.full_name}"'
        )
    return response


def _get_full_method_name(method_descriptor: _MethodDescriptor) -> str:
    """
    Protobuf returns "namespace.ServiceName.MethodName", we need "namespace.ServiceName/MethodName".
    """
    return f'{method_descriptor.containing_service.full_name}/{method_descriptor.name}'


def _create_servicer_mock(servicer_class: type, /) -> _ServiceMock:
    _check_is_servicer_class(servicer_class)
    reflection = _reflect_servicer(servicer_class)

    if reflection:
        any_method_descriptor: _MethodDescriptor = next(iter(reflection.values()))
        service_name = any_method_descriptor.containing_service.full_name
        assert '/' not in service_name, service_name
    else:
        service_name = 'UNKNOWN'

    adder = getattr(inspect.getmodule(servicer_class), f'add_{servicer_class.__name__}_to_server')

    state = _ServiceMockState()

    methods = {}
    for attname, value in servicer_class.__dict__.items():
        if callable(value) and not attname.startswith('_'):
            methods[attname] = _wrap_grpc_method(
                python_method_name=attname,
                method_descriptor=reflection[attname],
                default_unimplemented_method=value,
                state=state,
            )
    mocked_servicer_class = type(
        f'Mock{servicer_class.__name__}',
        (servicer_class,),
        methods,
    )
    servicer = mocked_servicer_class()

    return _ServiceMock(
        servicer=servicer,
        adder=adder,
        service_name=service_name,
        known_methods=frozenset(methods),
        state=state,
    )
