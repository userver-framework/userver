"""Output layout of documents tagged with x-usrv-cpp-namespace.

A tagged document states its C++ namespace and, optionally, the path of its
generated files. Every generator (chaotic-gen, chaotic-openapi-gen) derives the
output of a tagged document from these rules only, so a document generates the
same files wherever it is generated, and a document referring to it includes
those files instead of generating a copy.
"""

import dataclasses
import os
import pathlib
import re
from typing import Any

import yaml

from chaotic import cpp_keywords
from chaotic import cpp_names
from chaotic import error

NAMESPACE_TAG = 'x-usrv-cpp-namespace'
PATH_TAG = 'x-usrv-cpp-path'

# In-file paths of the types a tagged document declares, the type is named after the last component
TYPE_ROOTS_RE = re.compile(r'/(components/schemas|definitions)/([^/]+)/')

GENERATE_SERIALIZER = True
GENERATE_SAX_PARSER = True
GENERATE_STREAM_WRITER = True

_IDENTIFIER_RE = re.compile(r'[A-Za-z_][A-Za-z0-9_]*')


class LayoutError(error.BaseError):
    def __init__(self, filepath: str, msg: str) -> None:
        super().__init__(full_filepath=filepath, infile_path='', schema_type='layout', msg=msg)


@dataclasses.dataclass(frozen=True)
class DocumentLayout:
    # C++ namespace of the document's types, e.g. 'whq::errors::dto'
    namespace: str
    # Path of the generated files relative to the output root, without extension, e.g. 'whq/errors/dto/errors'
    path: str

    @property
    def directory(self) -> str:
        return os.path.dirname(self.path)


def read_layout(filepath: str, data: Any) -> DocumentLayout | None:
    """Returns the layout a document declares, None for an untagged document."""
    if not isinstance(data, dict):
        return None
    namespace = data.get(NAMESPACE_TAG)
    path = data.get(PATH_TAG)
    if namespace is None:
        if path is not None:
            raise LayoutError(filepath, f'"{PATH_TAG}" requires "{NAMESPACE_TAG}"')
        return None

    if not isinstance(namespace, str) or not namespace:
        raise LayoutError(filepath, f'"{NAMESPACE_TAG}" must be a non-empty string')
    for part in namespace.split('::'):
        if not _IDENTIFIER_RE.fullmatch(part) or cpp_keywords.is_cpp_keyword(part):
            raise LayoutError(filepath, f'"{NAMESPACE_TAG}: {namespace}" is not a valid C++ namespace')

    if path is None:
        path = '{}/{}'.format(namespace.replace('::', '/'), pathlib.Path(filepath).stem)
    elif not isinstance(path, str) or not path or path.startswith('/') or '..' in path.split('/'):
        raise LayoutError(filepath, f'"{PATH_TAG}" must be a relative path inside the output directory')

    return DocumentLayout(namespace=namespace, path=path)


def type_name(infile_path: str) -> str | None:
    """C++ name, relative to the document namespace, of a type declared at infile_path."""
    match = TYPE_ROOTS_RE.fullmatch(infile_path.rstrip('/') + '/')
    if not match:
        return None
    return cpp_names.cpp_identifier(match.group(2))


def openapi_only_sections(data: Any) -> list[str]:
    """Sections chaotic-openapi-gen generates types for and chaotic-gen does not."""
    sections = []
    if data.get('paths'):
        sections.append('paths')
    for name in ('responses', 'parameters', 'requestBodies', 'headers'):
        if (data.get('components') or {}).get(name):
            sections.append(f'components/{name}')
    for name in ('responses', 'parameters'):
        if data.get(name):
            sections.append(name)
    return sections


def _external_files(filepath: str, data: Any) -> set[str]:
    files = set()

    def visit(value: Any) -> None:
        if isinstance(value, list):
            for item in value:
                visit(item)
        elif isinstance(value, dict):
            ref_ = value.get('$ref')
            if isinstance(ref_, str) and not ref_.startswith('#'):
                file = ref_.split('#', 1)[0]
                files.add(os.path.normpath(os.path.join(os.path.dirname(filepath), file)))
            for item in value.values():
                visit(item)

    visit(data)
    files.discard(filepath)
    return files


@dataclasses.dataclass
class Closure:
    # Normalised path -> document content, inputs first
    documents: dict[str, Any]
    layouts: dict[str, DocumentLayout]
    inputs: list[str]

    @property
    def dependencies(self) -> list[str]:
        return [path for path in self.documents if path not in self.inputs]


def load_closure(inputs: dict[str, Any]) -> Closure | None:
    """
    Loads the documents tagged inputs refer to, recursively.

    Returns None when no input is tagged. Every input and every document they
    refer to must be tagged otherwise.
    """
    layouts: dict[str, DocumentLayout] = {}
    untagged = []
    for path, data in inputs.items():
        document_layout = read_layout(path, data)
        if document_layout is None:
            untagged.append(path)
        else:
            layouts[path] = document_layout
    if not layouts:
        return None
    if untagged:
        raise LayoutError(untagged[0], f'Untagged document among documents with "{NAMESPACE_TAG}"')

    documents = dict(inputs)
    queue = list(inputs)
    while queue:
        referrer = queue.pop()
        for path in sorted(_external_files(referrer, documents[referrer])):
            if path in documents:
                continue
            if not os.path.exists(path):
                raise LayoutError(referrer, f'$ref to missing file "{path}"')
            with open(path) as ifile:
                data = yaml.load(ifile, Loader=yaml.CLoader)
            layout = read_layout(path, data)
            if layout is None:
                raise LayoutError(path, f'Document referred to from "{referrer}" has no "{NAMESPACE_TAG}"')
            documents[path] = data
            layouts[path] = layout
            queue.append(path)

    return Closure(documents=documents, layouts=layouts, inputs=list(inputs))


def output_files(layout: DocumentLayout) -> list[str]:
    """Files generated for a tagged document, relative to the output root."""
    return schema_output_files(layout.path)


def schema_output_files(path: str) -> list[str]:
    """Files generated for a document's types at path (without extension), relative to the output root."""
    return [
        f'include/{path}_fwd.hpp',
        f'include/{path}.hpp',
        f'include/{path}_parsers.ipp',
        f'include/{path}_sax_parsers.hpp',
        f'src/{path}.cpp',
    ]
