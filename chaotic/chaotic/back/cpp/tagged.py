"""Rendering of documents tagged with x-usrv-cpp-namespace, shared by all generators."""

from chaotic import layout
from chaotic.back.cpp import renderer
from chaotic.back.cpp import types as cpp_types


def vfilepath_to_relfilepath(closure: layout.Closure) -> dict[str, str]:
    # OneToOneFileRenderer strips one extension, the path itself may contain dots
    return {path: document_layout.path + '.yaml' for path, document_layout in closure.layouts.items()}


def render(
    types: dict[str, cpp_types.CppType],
    closure: layout.Closure,
    clang_format_bin: str,
) -> list[renderer.CppOutput]:
    """
    Renders the types declared by the closure's inputs. Types of the other
    documents are only included.
    """
    inputs = set(closure.inputs)
    input_types = {}
    for name, type_ in types.items():
        assert type_.json_schema
        if type_.json_schema.source_location().filepath in inputs:
            input_types[name] = type_

    outputs = renderer.OneToOneFileRenderer(
        relative_to='',
        vfilepath_to_relfilepath=vfilepath_to_relfilepath(closure),
        clang_format_bin=clang_format_bin,
        generate_serializer=layout.GENERATE_SERIALIZER,
        generate_sax_parser=layout.GENERATE_SAX_PARSER,
        generate_stream_writer=layout.GENERATE_STREAM_WRITER,
    ).render(input_types, local_pair_header=False)

    produced = {output.filepath_wo_ext for output in outputs}
    for path in closure.inputs:
        document_layout = closure.layouts[path]
        if document_layout.path not in produced:
            outputs.append(_empty_output(document_layout))
    return outputs


def _empty_output(document_layout: layout.DocumentLayout) -> renderer.CppOutput:
    # A document without types still has its files, so that the build can declare them unconditionally
    path = document_layout.path
    return renderer.CppOutput(
        filepath_wo_ext=path,
        files=[
            renderer.CppOutputFile(content='#pragma once\n', ext='_fwd.hpp', subdir='include/'),
            renderer.CppOutputFile(
                content=f'#pragma once\n\n#include <{path}_fwd.hpp>\n', ext='.hpp', subdir='include/'
            ),
            renderer.CppOutputFile(content='', ext='_parsers.ipp', subdir='include/'),
            renderer.CppOutputFile(content='#pragma once\n', ext='_sax_parsers.hpp', subdir='include/'),
            renderer.CppOutputFile(content=f'#include <{path}.hpp>\n', ext='.cpp', subdir='src/'),
        ],
    )
