import json
import os
import pathlib
import subprocess
from typing import Any
from typing import Dict
from typing import List
from typing import Tuple

import jinja2
import yaml

from chaotic.back.cpp import renderer
from chaotic.back.cpp import translator
from chaotic.back.cpp import types
from chaotic.front.types import ResolvedSchemas
from chaotic.main import generate_cpp_name_func
from chaotic.main import NameMapItem
from chaotic.main import read_schemas

INCLUDE_DIR = str(pathlib.Path(__file__).parent.parent.parent / 'include')

CLANG_FORMAT_BIN = None


def get_clang_format_bin():
    global CLANG_FORMAT_BIN  # pylint: disable=global-statement
    if CLANG_FORMAT_BIN is not None:
        return CLANG_FORMAT_BIN

    if os.environ.get('CODEGEN_FORMAT'):
        ya_bin = (
            pathlib.Path(
                __file__,
            ).parent.parent.parent.parent.parent.parent.parent
            / 'ya'
        )
        CLANG_FORMAT_BIN = subprocess.check_output(
            [str(ya_bin), 'tool', 'clang-format', '--print-path'],
            encoding='utf-8',
        ).strip()
    else:
        CLANG_FORMAT_BIN = ''
    return CLANG_FORMAT_BIN


def taxi_alias(type_name: str) -> str:
    return types.camel_case(type_name.split('::')[-1], True)


# TODO: move to compilers.common?
def write_file(filepath: str, content: str) -> None:
    os.makedirs(os.path.dirname(filepath), exist_ok=True)

    if os.path.exists(filepath):
        with open(filepath, 'r', encoding='utf8') as ifile:
            old_content = ifile.read()
            if old_content == content:
                return

    with open(filepath, 'w') as ofile:
        ofile.write(content)


def enrich_jinja_env(env: jinja2.Environment) -> None:
    env.globals['camel_case'] = types.camel_case
    env.globals['taxi_alias'] = taxi_alias


class CompilerBase:
    def __init__(self) -> None:
        self._variables_types: Dict[str, Dict[str, types.CppType]] = {}
        self._definitions: Dict[
            str, Tuple[ResolvedSchemas, Dict[str, types.CppType]],
        ] = {}
        self._defaults: Dict[str, Any] = {}

    def extract_definition_names(self, filepath: str) -> List[str]:
        with open(filepath, 'r') as ifile:
            content = yaml.load(ifile, Loader=yaml.CLoader)
        return list(set(self._extract_definition_names(content)) - set(['']))

    def _extract_definition_names(self, content: Any) -> List[str]:
        if isinstance(content, dict):
            refs = []
            ref = content.get('$ref')
            if isinstance(ref, str):
                filename = ref.split('#')[0]
                refs.append(filename)

            for value in content.values():
                refs += self._extract_definition_names(value)

            return refs

        if isinstance(content, list):
            refs = []
            for value in content:
                refs += self._extract_definition_names(value)
            return refs

        return []

    def extract_variable_type(self) -> types.CppType:
        keys = list(self._variables_types.keys())
        assert len(keys) == 1
        name = keys[0]

        name_lower = self.format_ns_name(name)
        types = self._variables_types[name]
        return types[f'taxi_config::{name_lower}::VariableTypeRaw']

    def format_ns_name(self, name: str) -> str:
        return name.lower().replace('/', '_').replace('-', '_').split('.')[0]

    def parse_definition(
        self, filepath: str, name: str, include_dirs: List[str] = [],
    ) -> None:
        name_lower = self.format_ns_name(name)
        name_map = [NameMapItem('/([^/]+)/={0}')]
        # fname = f'taxi_config/definitions/{name}'
        fname = f'{name}'

        schemas, types = self._generate_types(
            filepath,
            namespace=f'taxi_config::{name_lower}',
            name_map=name_map,
            erase_prefix='',
            fname=fname,
            include_dirs=include_dirs,
        )
        self._definitions[name] = (schemas, types)

    def definitions_includes_hpp(self) -> List[str]:
        types = self._collect_types()
        includes: List[str] = []
        for type_ in types.values():
            includes += type_.declaration_includes()
        return sorted(set(includes))

    def definitions_includes_cpp(self) -> List[str]:
        types = self._collect_types()
        includes: List[str] = []
        for type_ in types.values():
            includes += type_.definition_includes()
        return sorted(set(includes))

    def parse_variable(
        self, filepath: str, name: str, include_dirs: List[str] = [],
    ) -> None:
        name_lower = self.format_ns_name(name)
        name_map = [
            # Variable type
            NameMapItem('/schema/=VariableTypeRaw'),
            # Aux. types
            NameMapItem('/schema/definitions/([^/]+)/={0}'),
        ]
        # The virtual name is used for generated filepath identification
        # fname = f'taxi_config/variables/{name}.types.yaml'
        fname = f'{name}'

        schemas, types = self._generate_types(
            filepath,
            namespace=f'taxi_config::{name_lower}',
            erase_prefix='/schema',
            name_map=name_map,
            fname=fname,
            include_dirs=include_dirs,
        )

        self._variables_types[name] = types
        self._defaults[name] = self._read_default(filepath)

    def _collect_schemas(self) -> List[ResolvedSchemas]:
        schemas = []
        for def_schemas, _definitions in self._definitions.values():
            schemas.append(def_schemas)
        return schemas

    def _collect_types(self) -> Dict[str, types.CppType]:
        types = {}
        for _def_schemas, definitions in self._definitions.values():
            types.update(definitions)
        return types

    def _generate_types(
        self,
        filepath: str,
        namespace: str,
        erase_prefix: str,
        name_map,
        fname: str,
        include_dirs: List[str],
    ) -> Tuple[ResolvedSchemas, Dict[str, types.CppType]]:
        schemas = read_schemas(
            erase_path_prefix=erase_prefix,
            filepaths=[filepath],
            name_map=name_map,
            file_map=[NameMapItem(filepath + '=' + fname)],
            dependencies=self._collect_schemas(),
        )
        cpp_name_func = generate_cpp_name_func(
            name_map=name_map, erase_prefix=erase_prefix,
        )
        gen = translator.Generator(
            config=translator.GeneratorConfig(
                include_dirs=[INCLUDE_DIR] + include_dirs,
                namespaces={fname: namespace},
                infile_to_name_func=cpp_name_func,
                autodiscover_default_dict=True,
                strict_parsing_default=False,
            ),
        )
        types = gen.generate_types(
            schemas, external_schemas=self._collect_types(),
        )
        return schemas, types

    def variables_includes_hpp(self, name: str) -> List[str]:
        types = self._variables_types[name]
        includes: List[str] = []
        for type_ in types.values():
            includes += type_.declaration_includes()

        return sorted(set(includes))

    def variables_includes_cpp(self, name: str) -> List[str]:
        types = self._variables_types[name]
        includes: List[str] = []
        for type_ in types.values():
            includes += type_.definition_includes()
        return sorted(set(includes))

    def variables_external_includes_hpp(self, name: str) -> List[str]:
        types = self._variables_types[name]
        return self.renderer_for_variable(
            name, False,
        ).extract_external_includes(types, '')

    def _read_default(self, filepath: str) -> Any:
        with open(filepath, 'r') as ifile:
            content = yaml.load(ifile, Loader=yaml.CLoader)
        return content['default']

    def _jinja(self) -> jinja2.Environment:
        raise NotImplementedError()

    def create_aliases(
        self, types: Dict[str, types.CppType],
    ) -> List[Tuple[str, str]]:
        return []

    def renderer_for_variable(
        self, name: str, parse_extra_formats: bool,
    ) -> renderer.OneToOneFileRenderer:
        return renderer.OneToOneFileRenderer(
            relative_to='/',
            vfilepath_to_relfilepath={
                name: f'taxi_config/variables/{name}.types.hpp',
                **{
                    name: (
                        'taxi_config/definitions/'
                        f'{name.split(".")[0].replace("/", "_")}.hpp'
                    )
                    for name in self._definitions
                },
            },
            clang_format_bin=get_clang_format_bin(),
            parse_extra_formats=parse_extra_formats,
            generate_serializer=parse_extra_formats,
        )

    def variable_type(self, name: str) -> str:
        types = self._variables_types[name]
        name_lower = self.format_ns_name(name)
        var_type = types[f'taxi_config::{name_lower}::VariableTypeRaw']
        return var_type.cpp_user_name()

    # TODO: move jinja files to arcadia_compiler
    def generate_variable(
        self, name: str, output_dir: str, parse_extra_formats: bool,
    ) -> None:
        types = self._variables_types[name]
        outputs = self.renderer_for_variable(name, parse_extra_formats).render(
            types,
            local_pair_header=False,
            # pair_header=f'taxi_config/variables/{name}.types.hpp',
        )

        name_lower = self.format_ns_name(name)
        var_type = types[f'taxi_config::{name_lower}::VariableTypeRaw']

        # types_fwd.hpp, types.{hpp,cpp}
        assert len(outputs) == 1
        for file in outputs[0].files:
            write_file(
                os.path.join(
                    output_dir,
                    file.subdir
                    + f'taxi_config/variables/{name}.types'
                    + file.ext,
                ),
                file.content,
            )

        # variable.{hpp,cpp}
        env = {
            'types_hpp': f'taxi_config/variables/{name}.types.hpp',
            'variable_hpp': f'taxi_config/variables/{name}.hpp',
            'name_lower': name_lower,
            'name': name,
            'type': var_type.parser_type('', ''),
            'types': types,
            'aliases': self.create_aliases(types),
            'cpp_type': var_type,
            'cpp_user_type': var_type.cpp_user_name(),
            'default_value': json.dumps(self._defaults[name]),
            'generate_taxi_aliases': True,
        }

        tpl = self._jinja().get_template('variable.hpp.jinja')
        write_file(
            os.path.join(
                output_dir, f'include/taxi_config/variables/{name}.hpp',
            ),
            renderer.format_pp(tpl.render(env), binary=get_clang_format_bin()),
        )

        tpl = self._jinja().get_template('variable.cpp.jinja')
        write_file(
            os.path.join(output_dir, f'src/taxi_config/variables/{name}.cpp'),
            renderer.format_pp(tpl.render(env), binary=get_clang_format_bin()),
        )

    def generate_definition(
        self, name: str, output_dir: str, parse_extra_formats: bool,
    ) -> None:
        _schemas, types = self._definitions[name]
        outputs = renderer.OneToOneFileRenderer(
            relative_to='/',
            vfilepath_to_relfilepath={name: f'taxi_config/definitions/{name}'},
            clang_format_bin=get_clang_format_bin(),
            parse_extra_formats=parse_extra_formats,
            generate_serializer=parse_extra_formats,
        ).render(
            types,
            local_pair_header=False,
            pair_header=f'taxi_config/definitions/{name}',
        )

        # types.{hpp,cpp}
        assert len(outputs) == 1
        for file in outputs[0].files:
            write_file(
                os.path.join(
                    output_dir,
                    file.subdir + f'taxi_config/definitions/{name}' + file.ext,
                ),
                file.content,
            )
