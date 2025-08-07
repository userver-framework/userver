#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""The core of the proto structs generation."""

import pathlib
import sys
import typing
from typing import Any
from typing import Dict
from typing import Optional

from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf.compiler import plugin_pb2  # pyright: ignore
import jinja2

from proto_structs.descriptors import node_parsers
from proto_structs.models import gen_node
from proto_structs.models import includes


def _strip_ext(path: str) -> str:
    return path.removesuffix('.proto')


class _CodeGenerator:
    def __init__(
        self,
        file_descriptor: descriptor.FileDescriptor,
        jinja_env: jinja2.Environment,
        response: plugin_pb2.CodeGeneratorResponse,  # pyright: ignore
    ) -> None:
        self.file_descriptor = file_descriptor
        self.jinja_env = jinja_env
        self.response = response  # pyright: ignore

    def run(self) -> None:
        file_node = node_parsers.parse_file(self.file_descriptor)
        data = self._make_jinja_data(file_node)

        for file_ext in ['cpp', 'hpp']:
            template_name = f'structs.usrv.{file_ext}.jinja'
            template = self.jinja_env.get_template(template_name)

            file_name = str(file_node.gen_path(ext=file_ext))
            file_content = template.render(**data)

            file = self.response.file.add()  # pyright: ignore
            file.name = file_name  # pyright: ignore
            file.content = file_content  # pyright: ignore

    def _make_jinja_data(self, file_node: gen_node.File) -> Dict[str, Any]:
        includes_list = includes.sorted_includes(file_node, current_hpp=str(file_node.gen_path(ext='hpp')))
        proto_file_name = typing.cast(str, self.file_descriptor.name)

        return {
            'file_name_wo_ext': _strip_ext(proto_file_name),
            'gen_nodes': file_node.children,
            'includes': includes_list,
        }


def generate(loader: jinja2.BaseLoader) -> None:
    data = sys.stdin.buffer.read()

    request = plugin_pb2.CodeGeneratorRequest()  # pyright: ignore
    request.ParseFromString(data)  # pyright: ignore

    response = plugin_pb2.CodeGeneratorResponse()  # pyright: ignore
    if hasattr(response, 'FEATURE_PROTO3_OPTIONAL'):  # pyright: ignore
        setattr(
            response,  # pyright: ignore
            'supported_features',
            getattr(response, 'FEATURE_PROTO3_OPTIONAL'),  # pyright: ignore
        )

    jinja_env = jinja2.Environment(
        loader=loader,
        trim_blocks=True,
        lstrip_blocks=True,
        # Do not escape C++ sources for HTML.
        autoescape=False,
    )

    pool = descriptor_pool.DescriptorPool()

    # pylint: disable=no-member
    for proto_file in request.proto_file:  # pyright: ignore
        pool.Add(proto_file)  # pyright: ignore

    # pylint: disable=no-member
    for file_to_generate in request.file_to_generate:  # pyright: ignore
        _CodeGenerator(
            file_descriptor=pool.FindFileByName(file_to_generate),  # pyright: ignore
            jinja_env=jinja_env,
            response=response,  # pyright: ignore
        ).run()

    sys.stdout.buffer.write(response.SerializeToString())  # pyright: ignore


def main(loader: Optional[jinja2.BaseLoader] = None) -> None:
    if loader is None:
        loader = jinja2.FileSystemLoader(pathlib.Path(__file__).resolve().parent / 'templates')

    generate(
        loader=loader,
    )


if __name__ == '__main__':
    main()
