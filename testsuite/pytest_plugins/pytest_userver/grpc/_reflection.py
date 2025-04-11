import dataclasses
import inspect
from typing import Mapping
from typing import Tuple

import google.protobuf.descriptor
import google.protobuf.descriptor_pool

_MethodDescriptor = google.protobuf.descriptor.MethodDescriptor


@dataclasses.dataclass(frozen=True)
class _RawMethodInfo:
    full_rpc_name: str  # in the format "/with.package.ServiceName/MethodName"


class _FakeChannel:
    def unary_unary(self, name, *args, **kwargs):
        return _RawMethodInfo(full_rpc_name=name)

    def unary_stream(self, name, *args, **kwargs):
        return _RawMethodInfo(full_rpc_name=name)

    def stream_unary(self, name, *args, **kwargs):
        return _RawMethodInfo(full_rpc_name=name)

    def stream_stream(self, name, *args, **kwargs):
        return _RawMethodInfo(full_rpc_name=name)


def _to_method_descriptor(raw_info: _RawMethodInfo) -> _MethodDescriptor:
    full_service_name, method_name = _split_rpc_name(raw_info.full_rpc_name)
    service_descriptor = google.protobuf.descriptor_pool.Default().FindServiceByName(full_service_name)
    method_descriptor = service_descriptor.FindMethodByName(method_name)
    assert method_descriptor is not None
    return method_descriptor


def _reflect_servicer(servicer_class: type) -> Mapping[str, _MethodDescriptor]:
    service_name = servicer_class.__name__.removesuffix('Servicer')
    stub_class = getattr(inspect.getmodule(servicer_class), f'{service_name}Stub')
    return _reflect_stub(stub_class)


def _reflect_stub(stub_class: type) -> Mapping[str, _MethodDescriptor]:
    # HACK: relying on the implementation of generated *Stub classes.
    raw_infos = stub_class(_FakeChannel()).__dict__.items()

    return {python_method_name: _to_method_descriptor(raw_info) for python_method_name, raw_info in raw_infos}


def _split_rpc_name(rpc_name: str) -> Tuple[str, str]:
    normalized = rpc_name.removeprefix('/')
    results = normalized.split('/')
    if len(results) != 2:
        raise ValueError(
            f'Invalid RPC name: "{rpc_name}". RPC name must be of the form "with.package.ServiceName/MethodName"'
        )
    return results
