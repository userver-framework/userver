import collections
import dataclasses
import os
import pathlib
import subprocess
from typing import Any
from typing import Dict
from typing import List
from typing import NoReturn
from typing import Optional

import jinja2

from chaotic.back.cpp import types as cpp_types
from chaotic.front import types


@dataclasses.dataclass
class CppOutputFile:
    content: str
    ext: str
    subdir: str


@dataclasses.dataclass
class CppOutput:
    filepath_wo_ext: str
    files: List[CppOutputFile]


current_namespace = ''  # pylint: disable=invalid-name


def get_current_namespace() -> str:
    return current_namespace


def close_namespace() -> str:
    if current_namespace:
        data = ''
        for name in reversed(current_namespace.split('::')):
            data += '} //' + name + '\n'
        return data
    else:
        return ''


def open_namespace(new_ns: str) -> str:
    # pylint: disable=global-statement
    global current_namespace
    current_namespace = new_ns

    if new_ns:
        res = ''
        for namespace in new_ns.split('::'):
            res = res + f'namespace {namespace} {{'
        return res
    else:
        return ''


def cpp_namespace(name: str) -> str:
    if '::' not in name:
        return ''
    ns_part, _name_part = name.rsplit('::', 1)
    return ns_part


def cpp_type(name: str) -> str:
    if '::' not in name:
        return name
    _ns_part, name_part = name.rsplit('::', 1)
    return name_part


def declaration_includes(types: List[cpp_types.CppType]) -> List[str]:
    includes = set()
    for type_ in types:
        includes.update(set(type_.declaration_includes()))
    return sorted(includes)


def definition_includes(types: List[cpp_types.CppType]) -> List[str]:
    includes = set()
    for type_ in types:
        includes.update(set(type_.definition_includes()))
    return sorted(includes)


def extra_cpp_type(type_: cpp_types.CppStruct) -> str:
    extra_type = type_.extra_type
    if extra_type is True:
        return 'USERVER_NAMESPACE::formats::json::Value'
    else:
        assert isinstance(extra_type, cpp_types.CppType)
        container = type_.extra_container()
        return f'{container}<std::string, {extra_type.cpp_user_name()}>'


def extra_cpp_parser_type(extra_type: cpp_types.CppType) -> str:
    assert isinstance(extra_type, cpp_types.CppType)
    return extra_type.parser_type(ns='TODO', name='Extra')


def cpp_struct_is_strict_parsing(struct: cpp_types.CppStruct) -> bool:
    assert isinstance(struct, cpp_types.CppStruct)
    return struct.strict_parsing and struct.extra_type is False


def not_implemented(obj: Any = None) -> NoReturn:
    raise Exception(repr(obj))


def make_arcadia_loader() -> jinja2.FunctionLoader:
    import library.python.resource as arc_resource

    def arc_resource_loader(name: str) -> jinja2.BaseLoader:
        return arc_resource.resfs_read(
            f'taxi/uservices/userver/chaotic/chaotic/back/cpp/{name}',
        ).decode('utf-8')

    loader = jinja2.FunctionLoader(arc_resource_loader)

    # try to load something and drop the result
    try:
        arc_resource_loader('templates/type.hpp.jinja')
    except Exception:
        raise ImportError('resfs is not available')

    return loader


def make_env() -> jinja2.Environment:
    loader: jinja2.BaseLoader
    try:
        loader = make_arcadia_loader()
    except ImportError:
        loader = jinja2.FileSystemLoader(PARENT_DIR)
    env = jinja2.Environment(loader=loader)

    env.globals['NOT_IMPLEMENTED'] = not_implemented
    env.globals['enumerate'] = enumerate

    env.globals['cpp_struct_is_strict_parsing'] = cpp_struct_is_strict_parsing

    env.globals['declaration_includes'] = declaration_includes
    env.globals['definition_includes'] = definition_includes

    env.globals['cpp_namespace'] = cpp_namespace
    env.globals['cpp_type'] = cpp_type

    env.globals['extra_cpp_type'] = extra_cpp_type
    env.globals['extra_cpp_parser_type'] = extra_cpp_parser_type

    env.globals['open_namespace'] = open_namespace
    env.globals['close_namespace'] = close_namespace
    env.globals['get_current_namespace'] = get_current_namespace

    return env


PARENT_DIR = os.path.dirname(__file__)
JINJA_ENV = make_env()


def format_pp(input_: str, *, binary: str) -> str:
    if not binary:
        return input_
    output = subprocess.check_output([binary], input=input_, encoding='utf-8')
    return output + '\n'


class OneToOneFileRenderer:
    def __init__(
            self,
            *,
            relative_to: str,
            vfilepath_to_relfilepath: Dict[str, str],
            clang_format_bin: str,
            parse_extra_formats: bool = False,
            generate_serializer: bool = False,
    ) -> None:
        self._relative_to = relative_to
        self._vfilepath_to_relfilepath_map = vfilepath_to_relfilepath
        self._clang_format_bin = clang_format_bin
        self._parse_extra_formats = parse_extra_formats
        self._generate_serializer = generate_serializer

    @staticmethod
    def filepath_wo_ext(filepath: str) -> str:
        path = pathlib.Path(filepath)
        return str(path.parent / path.stem)

    def _vfilepath_to_relfilepath(self, vfilepath: str) -> str:
        return self._vfilepath_to_relfilepath_map[vfilepath]

    def extract_external_includes(
            self,
            types_cpp: Dict[str, cpp_types.CppType],
            ignore_filepath_wo_ext: str,
    ) -> List[str]:
        result = set()

        def visitor(
                schema: types.Schema, _parent: Optional[types.Schema],
        ) -> None:
            if not isinstance(schema, types.Ref):
                return

            filepath = self.filepath_wo_ext(
                self._vfilepath_to_relfilepath(
                    schema.schema.source_location().filepath,
                ),
            )
            if filepath != ignore_filepath_wo_ext:
                result.add(self.filepath_to_include(filepath))

        for type_ in types_cpp.values():
            assert type_.json_schema
            visitor(type_.json_schema, None)
            type_.json_schema.visit_children(visitor)

        return sorted(result)

    def filepath_to_include(self, filepath_wo_ext: str) -> str:
        if filepath_wo_ext.startswith('/'):
            return os.path.relpath(filepath_wo_ext, self._relative_to) + '.hpp'
        else:
            return filepath_wo_ext + '.hpp'

    def render(
            self,
            types: Dict[str, cpp_types.CppType],
            local_pair_header=True,
            pair_header: Optional[str] = None,
    ) -> List[CppOutput]:
        files: Dict[str, Dict[str, cpp_types.CppType]] = (
            collections.defaultdict(dict)
        )

        for name, type_ in types.items():
            assert type_.json_schema
            filepath = self.filepath_wo_ext(
                self._vfilepath_to_relfilepath(
                    type_.json_schema.source_location().filepath,
                ),
            )
            files[str(filepath)][name] = type_

        if self._parse_extra_formats:
            parse_formats = [
                '::formats::json',
                '::formats::yaml',
                '::yaml_config',
            ]
        else:
            parse_formats = ['::formats::json']

        output = []
        for filepath_wo_ext, types_cpp in files.items():
            external_includes = self.extract_external_includes(
                types_cpp, filepath_wo_ext,
            )

            if pair_header:
                p_header = pair_header
            else:
                if local_pair_header:
                    p_header = os.path.basename(filepath_wo_ext)
                else:
                    p_header = filepath_wo_ext

            env = {
                'pair_header': p_header,
                'types': types_cpp,
                'userver': 'USERVER_NAMESPACE',
                'external_includes': external_includes,
                'parse_formats': parse_formats,
                'generate_serializer': self._generate_serializer,
            }

            tpl = JINJA_ENV.get_template('templates/type_fwd.hpp.jinja')
            fwd_hpp = tpl.render(types=types_cpp)
            fwd_hpp = format_pp(fwd_hpp, binary=self._clang_format_bin)

            tpl = JINJA_ENV.get_template('templates/type.hpp.jinja')
            hpp = tpl.render(**env)
            hpp = format_pp(hpp, binary=self._clang_format_bin)

            tpl = JINJA_ENV.get_template('templates/type_parsers.ipp.jinja')
            parsers_ipp = tpl.render(**env)
            parsers_ipp = format_pp(parsers_ipp, binary=self._clang_format_bin)

            tpl = JINJA_ENV.get_template('templates/type.cpp.jinja')
            cpp = tpl.render(**env)
            cpp = format_pp(cpp, binary=self._clang_format_bin)

            output.append(
                CppOutput(
                    filepath_wo_ext=filepath_wo_ext,
                    files=[
                        CppOutputFile(
                            content=fwd_hpp, ext='_fwd.hpp', subdir='include/',
                        ),
                        CppOutputFile(
                            content=hpp, ext='.hpp', subdir='include/',
                        ),
                        CppOutputFile(
                            content=parsers_ipp,
                            ext='_parsers.ipp',
                            subdir='include/',
                        ),
                        CppOutputFile(content=cpp, ext='.cpp', subdir='src/'),
                    ],
                ),
            )

        return output
