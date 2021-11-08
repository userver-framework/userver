#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
This script is a Protobuf protoc plugin that generates userver asynchronous
wrappers for gRPC clients.

For each `path/X.proto` file that contains gRPC services, we generate
`path/X_client.usrv.pb.{hpp,cpp}` using
`client.usrv.pb.{hpp,cpp}.jinja` templates.
"""

import itertools
import os
import sys

from google.protobuf.compiler import plugin_pb2 as plugin
import jinja2


def _grpc_to_cpp_name(in_str):
    return in_str.replace('.', '::')


def _generate_code(jinja_env, proto_file, response):
    if not proto_file.service:
        return

    data = {
        'source_file': proto_file.name,
        'includes_client_hpp': [
            '<userver/clients/grpc/impl/client_data.hpp>',
            '<userver/clients/grpc/rpc.hpp>',
        ],
        'includes_client_cpp': [
            '"{}"'.format(
                proto_file.name.replace('.proto', '_client.usrv.pb.hpp'),
            ),
        ],
        'includes_handler_hpp': [
            '<userver/server/grpc/reactor.hpp>',
            '<userver/server/grpc/rpc.hpp>',
        ],
        'includes_handler_cpp': [
            '"{}"'.format(
                proto_file.name.replace('.proto', '_handler.usrv.pb.hpp'),
            ),
            '<userver/server/grpc/impl/listen_async.hpp>',
            '<userver/server/grpc/impl/reactor_data.hpp>',
        ],
        'generated_include': '"{}"'.format(
            proto_file.name.replace('.proto', '.grpc.pb.h'),
        ),
        'namespace': _grpc_to_cpp_name(proto_file.package),
        'services': [],
    }

    data['services'].extend(proto_file.service)

    for (file_type, file_ext) in itertools.product(
            ['client', 'handler'], ['hpp', 'cpp'],
    ):
        file = response.file.add()
        file.name = proto_file.name.replace(
            '.proto', f'_{file_type}.usrv.pb.{file_ext}',
        )
        file.content = jinja_env.get_template(
            f'{file_type}.usrv.{file_ext}.jinja',
        ).render(proto=data)


def main():
    data = sys.stdin.buffer.read()

    request = plugin.CodeGeneratorRequest()
    request.ParseFromString(data)

    response = plugin.CodeGeneratorResponse()

    jinja_env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(
            os.path.dirname(os.path.abspath(__file__)),
        ),
        trim_blocks=True,
        lstrip_blocks=True,
    )
    jinja_env.filters['grpc_to_cpp_name'] = _grpc_to_cpp_name

    # pylint: disable=no-member
    for proto_file in request.proto_file:
        _generate_code(jinja_env, proto_file, response)

    output = response.SerializeToString()
    sys.stdout.buffer.write(output)


if __name__ == '__main__':
    main()
