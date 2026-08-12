"""Functions for processing gRPC service descriptors."""

from collections.abc import Iterable
import typing

import google.protobuf.descriptor as descriptor

from proto_structs.descriptors import type_mapping
from proto_structs.models import type_ref


def collect_rpc_input_output_types(file: descriptor.FileDescriptor) -> Iterable[type_ref.TypeReference]:
    """Collects input and output types of all gRPC service methods in the file."""
    services = typing.cast(dict[str, descriptor.ServiceDescriptor], file.services_by_name)
    for service in services.values():
        methods: list[descriptor.MethodDescriptor] = service.methods
        for method in methods:
            input_type: descriptor.Descriptor = method.input_type
            output_type: descriptor.Descriptor = method.output_type
            for desc in [input_type, output_type]:
                yield type_mapping.parse_struct_reference(desc)
