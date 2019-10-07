#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# import codecs
import os
import sys

from google.protobuf.compiler import plugin_pb2 as plugin
import jinja2

# sys.stdin = codecs.getreader('utf-8')(sys.stdin)
# sys.stdout = codecs.getreader('utf-8')(sys.stdout)


def grpc_to_cpp_name(in_str):
    return in_str.replace('.', '::')


def generate_code(jinja_env, request, response):
    template = jinja_env.get_template('userver_grpc_wrapper.jinja')
    if request.proto_file:
        # The list of files is topologically sorted, so the last in chain
        # is the file that is needed to be generated
        proto_file = request.proto_file[-1]
        data = {
            'source_file': proto_file.name,
            'includes': ['"{}"'.format(proto_file.name.replace(
                    '.proto', '.grpc.pb.h')),
                    '<clients/grpc/service.hpp>'],
            'namespace': grpc_to_cpp_name(proto_file.package),
            'services': [],
        }

        for item in proto_file.service:
            data['services'].append(item)

        file = response.file.add()
        file.name = proto_file.name.replace('.proto', '.usrv.pb.hpp')
        file.content = template.render(proto=data)


def main():
    data = sys.stdin.buffer.read()

    request = plugin.CodeGeneratorRequest()
    request.ParseFromString(data)

    response = plugin.CodeGeneratorResponse()

    jinja_env = jinja2.Environment(
        loader=jinja2.FileSystemLoader(
            os.path.dirname(os.path.abspath(__file__))),
        trim_blocks=True,
        lstrip_blocks=True,
    )
    jinja_env.filters['grpc_to_cpp_name'] = grpc_to_cpp_name

    generate_code(jinja_env, request, response)

    output = response.SerializeToString()
    sys.stdout.buffer.write(output)


if __name__ == '__main__':
    main()
