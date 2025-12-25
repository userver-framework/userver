#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""The core of the proto structs generation."""

import dataclasses
import pathlib
import sys
import typing
from typing import Any

from google.protobuf import descriptor
from google.protobuf import descriptor_pool
from google.protobuf.compiler import plugin_pb2  # pyright: ignore
import jinja2

from proto_structs.descriptors import node_parsers
from proto_structs.models import gen_node
from proto_structs.models import includes
from proto_structs.models import options
from proto_structs.models import type_ref_consts


def _strip_ext(path: str) -> str:
    return path.removesuffix('.proto')


@dataclasses.dataclass(frozen=True)
class Params(options.ModelBase):
    """
    protoc allows to pass a single string option to a plugin (`--uproto-structs_opt`).
    The option must contain JSON described by this model.
    """

    #: Absolute path to the file with the JSON containing detailed options, see `models/options.py`.
    opts_file: pathlib.Path | None = None


class _CodeGenerator:
    def __init__(
        self,
        file_descriptor: descriptor.FileDescriptor,
        jinja_env: jinja2.Environment,
        plugin_options: options.PluginOptions,
        response: plugin_pb2.CodeGeneratorResponse,  # pyright: ignore
    ) -> None:
        self.file_descriptor = file_descriptor
        self.jinja_env = jinja_env
        self.plugin_options = plugin_options
        self.response = response  # pyright: ignore

    def run(self) -> None:
        try:
            file_node = node_parsers.parse_file(self.file_descriptor, plugin_options=self.plugin_options)
            data = self._make_jinja_data(file_node)

            for file_ext in ['cpp', 'hpp']:
                template_name = f'structs.usrv.{file_ext}.jinja'
                template = self.jinja_env.get_template(template_name)

                file_name = str(file_node.gen_path(ext=file_ext))
                file_content = template.render(**data)

                file = self.response.file.add()  # pyright: ignore
                file.name = file_name  # pyright: ignore
                file.content = file_content  # pyright: ignore
        except Exception:
            raise Exception(
                f'userver proto structs codegen failed for file: {self.file_descriptor.name} '
                '(see details in the exception above)',
            )

    @staticmethod
    def _make_jinja_data(file_node: gen_node.File) -> dict[str, Any]:
        includes_dict = includes.sorted_includes(file_node, current_hpp=str(file_node.gen_path(ext='hpp')))

        return {
            'file_name_wo_ext': _strip_ext(str(file_node.proto_relative_path)),
            'file': file_node,
            'includes_hpp': includes_dict[includes.IncludeKind.FOR_HPP],
            'includes_cpp': includes_dict[includes.IncludeKind.FOR_CPP],
            'type_ref_consts': type_ref_consts,
        }


def generate(loader: jinja2.BaseLoader) -> None:
    data = sys.stdin.buffer.read()

    request = plugin_pb2.CodeGeneratorRequest()  # pyright: ignore
    request.ParseFromString(data)  # pyright: ignore

    params = Params.from_json(request.parameter or '{}')  # pyright: ignore

    response = plugin_pb2.CodeGeneratorResponse()  # pyright: ignore
    if hasattr(response, 'FEATURE_PROTO3_OPTIONAL'):  # pyright: ignore
        setattr(  # noqa: B010
            response,  # pyright: ignore
            'supported_features',
            getattr(response, 'FEATURE_PROTO3_OPTIONAL'),  # pyright: ignore  # noqa: B009
        )

    jinja_env = jinja2.Environment(
        loader=loader,
        trim_blocks=True,
        lstrip_blocks=True,
        # Do not escape C++ sources for HTML.
        autoescape=False,
    )

    pool = descriptor_pool.DescriptorPool()

    files: list[str] = []
    # pylint: disable=no-member
    for proto_file in request.proto_file:  # pyright: ignore
        pool.Add(proto_file)  # pyright: ignore
        name: str = typing.cast(str, proto_file.name)
        files.append(name)

    try:
        plugin_options = options.load_plugin_options(params.opts_file)
    except Exception:
        raise Exception(
            f'userver proto structs codegen failed to parse options for files: {", ".join(files)} '
            '(see details in the exception above)',
        )

    # pylint: disable=no-member
    for file_to_generate in request.file_to_generate:  # pyright: ignore
        _CodeGenerator(
            file_descriptor=pool.FindFileByName(file_to_generate),  # pyright: ignore
            jinja_env=jinja_env,
            plugin_options=plugin_options,
            response=response,  # pyright: ignore
        ).run()

    sys.stdout.buffer.write(response.SerializeToString())  # pyright: ignore


def main(loader: jinja2.BaseLoader | None = None) -> None:
    if loader is None:
        loader = jinja2.FileSystemLoader(pathlib.Path(__file__).resolve().parent / 'templates')

    generate(
        loader=loader,
    )


if __name__ == '__main__':
    main()
