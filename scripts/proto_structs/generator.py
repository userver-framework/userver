#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
What am I? Bro, you have to add description.
"""

import os
import sys
from typing import Any
from typing import Dict
from typing import Optional

from google.protobuf.compiler import plugin_pb2  # pyright: ignore
import google.protobuf.descriptor_pb2 as descriptor_pb2  # pyright: ignore
import jinja2


def strip_ext(path: str) -> str:
    return path.removesuffix('.proto')


class _CodeGenerator:
    def __init__(
        self,
        proto_file: descriptor_pb2.FileDescriptorProto,
        response: plugin_pb2.CodeGeneratorResponse,
        jinja_env: jinja2.Environment,
    ) -> None:
        self.proto_file = proto_file
        self.response = response
        self.jinja_env = jinja_env

    def run(self) -> None:
        self._generate_code()

    @property
    def _proto_file_stem(self) -> str:
        return self.proto_file.name.replace('.proto', '')

    def _generate_code(self) -> None:
        data: Dict[str, Any] = {
            'name_wo_ext': strip_ext(self.proto_file.name),
            'dependency': list(map(strip_ext, self.proto_file.dependency)),
        }

        for file_ext in ['cpp', 'hpp']:
            template_name = f'structs.usrv.{file_ext}.jinja'
            template = self.jinja_env.get_template(template_name)

            file = self.response.file.add()
            file.name = f'{self._proto_file_stem}.structs.usrv.pb.{file_ext}'
            file.content = template.render(**data)


def generate(
    loader: jinja2.BaseLoader,
) -> None:
    data = sys.stdin.buffer.read()

    request = plugin_pb2.CodeGeneratorRequest()
    request.ParseFromString(data)

    response = plugin_pb2.CodeGeneratorResponse()
    if hasattr(response, 'FEATURE_PROTO3_OPTIONAL'):
        setattr(
            response,
            'supported_features',
            getattr(response, 'FEATURE_PROTO3_OPTIONAL'),
        )

    jinja_env = jinja2.Environment(
        loader=loader,
        trim_blocks=True,
        lstrip_blocks=True,
        # We do not render HTML pages with Jinja. However the gRPC data could
        # be shown on web. Assuming that the generation should not deal with
        # HTML special characters, it is safer to turn on autoescaping.
        autoescape=True,
    )

    # pylint: disable=no-member
    for proto_file in request.proto_file:
        _CodeGenerator(
            jinja_env=jinja_env,
            proto_file=proto_file,
            response=response,
        ).run()

    output = response.SerializeToString()
    sys.stdout.buffer.write(output)


def main(
    loader: Optional[jinja2.BaseLoader] = None,
) -> None:
    if loader is None:
        loader = jinja2.FileSystemLoader(
            os.path.join(
                os.path.dirname(os.path.abspath(__file__)),
                'templates',
            ),
        )

    generate(
        loader=loader,
    )


if __name__ == '__main__':
    main()
