#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
This script is a Protobuf protoc plugin that generates userver asynchronous
wrappers for gRPC clients.

For each `path/X.proto` file that contains gRPC services, we generate
`path/X_{client,handler}.usrv.pb.{hpp,cpp}` using
`{client,handler}.usrv.pb.{hpp,cpp}.jinja` templates.
"""

import enum
import itertools
import os
import sys
from typing import Optional

from google.protobuf.compiler import plugin_pb2 as plugin
import jinja2


class Mode(enum.Enum):
    Service = enum.auto()
    Client = enum.auto()

    def is_service(self) -> bool:
        return self == self.Service

    def is_client(self) -> bool:
        return self == self.Client


def _grpc_to_cpp_name(in_str):
    return in_str.replace('.', '::')


def _to_package_prefix(package):
    return f'{package}.' if package else ''


def _generate_code(jinja_env, proto_file, response, mode: Optional[Mode]):
    if not proto_file.service:
        return

    data = {
        'source_file': proto_file.name,
        'source_file_without_ext': proto_file.name.replace('.proto', ''),
        'package_prefix': _to_package_prefix(proto_file.package),
        'namespace': _grpc_to_cpp_name(proto_file.package),
        'services': [],
    }

    data['services'].extend(proto_file.service)

    if mode is None:
        src_files = ['client', 'service']
    elif mode.is_service():
        src_files = ['service']
    elif mode.is_client():
        src_files = ['client']
    else:
        raise Exception(f'Unknown mode {mode}')

    for (file_type, file_ext) in itertools.product(src_files, ['hpp', 'cpp']):
        template_name = f'{file_type}.usrv.{file_ext}.jinja'
        template = jinja_env.get_template(template_name)

        file = response.file.add()
        file.name = proto_file.name.replace(
            '.proto', f'_{file_type}.usrv.pb.{file_ext}',
        )
        file.content = template.render(proto=data)


def main(
        loader: Optional[jinja2.BaseLoader] = None,
        mode: Optional[Mode] = None,
) -> None:
    data = sys.stdin.buffer.read()

    request = plugin.CodeGeneratorRequest()
    request.ParseFromString(data)

    response = plugin.CodeGeneratorResponse()

    if loader is None:
        loader = jinja2.FileSystemLoader(
            os.path.join(
                os.path.dirname(os.path.abspath(__file__)), 'templates',
            ),
        )

    jinja_env = jinja2.Environment(
        loader=loader, trim_blocks=True, lstrip_blocks=True,
    )
    jinja_env.filters['grpc_to_cpp_name'] = _grpc_to_cpp_name

    # pylint: disable=no-member
    for proto_file in request.proto_file:
        _generate_code(
            jinja_env=jinja_env,
            proto_file=proto_file,
            response=response,
            mode=mode,
        )

    output = response.SerializeToString()
    sys.stdout.buffer.write(output)


if __name__ == '__main__':
    main()
