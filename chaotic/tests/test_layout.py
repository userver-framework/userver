import os
import subprocess
import sys

import pytest
import yaml

from chaotic import error
from chaotic import layout

CHAOTIC_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

ERRORS = """\
x-usrv-cpp-namespace: test::errors
components:
  schemas:
    Error:
      type: object
      additionalProperties: false
      properties:
        code: {type: string}
"""

VAULT = """\
x-usrv-cpp-namespace: test::vault::dto
components:
  schemas:
    Credential:
      type: object
      additionalProperties: false
      properties:
        error:
          $ref: '../common/errors.yaml#/components/schemas/Error'
"""


def _write(path, content: str) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)
    return str(path)


def _yaml(content: str) -> dict:
    return yaml.safe_load(content)


def _chaotic_gen(*args: str) -> subprocess.CompletedProcess:
    return subprocess.run(
        [sys.executable, '-m', 'chaotic.main', *args],
        cwd=CHAOTIC_ROOT,
        capture_output=True,
        text=True,
    )


def test_untagged_document():
    assert layout.read_layout('a/b.yaml', {'components': {}}) is None


def test_path_derived_from_namespace():
    assert layout.read_layout('contracts/errors.yaml', {'x-usrv-cpp-namespace': 'whq::errors::dto'}) == (
        layout.DocumentLayout(namespace='whq::errors::dto', path='whq/errors/dto/errors')
    )


def test_path_keeps_dots_of_stem():
    assert layout.read_layout('a/passport.internal.yaml', {'x-usrv-cpp-namespace': 'ns'}).path == 'ns/passport.internal'


def test_explicit_path():
    document_layout = layout.read_layout('a/b.yaml', {'x-usrv-cpp-namespace': 'ns', 'x-usrv-cpp-path': 'x/y/z'})
    assert document_layout == layout.DocumentLayout(namespace='ns', path='x/y/z')
    assert document_layout.directory == 'x/y'


@pytest.mark.parametrize(
    'data, message',
    [
        ({'x-usrv-cpp-path': 'x/y'}, 'requires "x-usrv-cpp-namespace"'),
        ({'x-usrv-cpp-namespace': 'a::1b'}, 'not a valid C\\+\\+ namespace'),
        ({'x-usrv-cpp-namespace': 'a::class'}, 'not a valid C\\+\\+ namespace'),
        ({'x-usrv-cpp-namespace': 'a::'}, 'not a valid C\\+\\+ namespace'),
        ({'x-usrv-cpp-namespace': ''}, 'must be a non-empty string'),
        ({'x-usrv-cpp-namespace': 'a', 'x-usrv-cpp-path': '/abs'}, 'must be a relative path'),
        ({'x-usrv-cpp-namespace': 'a', 'x-usrv-cpp-path': 'x/../y'}, 'must be a relative path'),
    ],
)
def test_invalid_tags(data, message):
    with pytest.raises(error.BaseError, match=message):
        layout.read_layout('a/b.yaml', data)


def test_type_name():
    assert layout.type_name('/components/schemas/Error') == 'Error'
    assert layout.type_name('/definitions/class') == 'class_'
    assert layout.type_name('/components/schemas/Error/properties/code') is None
    assert layout.type_name('/paths/[/x]/get/requestBody') is None


def test_closure_loads_dependencies_from_disk(tmp_path):
    _write(tmp_path / 'common/errors.yaml', ERRORS)
    vault = _write(tmp_path / 'schemas/vault.yaml', VAULT)

    closure = layout.load_closure({vault: _yaml(VAULT)})

    errors = str(tmp_path / 'common/errors.yaml')
    assert closure.inputs == [vault]
    assert closure.dependencies == [errors]
    assert closure.layouts[errors] == layout.DocumentLayout(namespace='test::errors', path='test/errors/errors')


def test_closure_of_untagged_documents():
    assert layout.load_closure({'a.yaml': {'components': {}}, 'b.yaml': {}}) is None


def test_closure_refuses_untagged_input():
    with pytest.raises(error.BaseError, match='Untagged document'):
        layout.load_closure({'a.yaml': {'x-usrv-cpp-namespace': 'a'}, 'b.yaml': {}})


def test_closure_refuses_untagged_dependency(tmp_path):
    _write(tmp_path / 'common/errors.yaml', 'components: {}\n')
    vault = _write(tmp_path / 'schemas/vault.yaml', VAULT)
    with pytest.raises(error.BaseError, match='has no "x-usrv-cpp-namespace"'):
        layout.load_closure({vault: _yaml(VAULT)})


def test_closure_refuses_missing_dependency(tmp_path):
    vault = _write(tmp_path / 'schemas/vault.yaml', VAULT)
    with pytest.raises(error.BaseError, match='\\$ref to missing file'):
        layout.load_closure({vault: _yaml(VAULT)})


def test_generates_input_and_includes_dependency(tmp_path):
    _write(tmp_path / 'common/errors.yaml', ERRORS)
    vault = _write(tmp_path / 'schemas/vault.yaml', VAULT)
    out = tmp_path / 'out'

    result = _chaotic_gen('-o', str(out), '--clang-format=', vault)
    assert result.returncode == 0, result.stderr

    generated = sorted(str(path.relative_to(out)) for path in out.rglob('*') if path.is_file())
    assert generated == sorted(
        layout.output_files(layout.DocumentLayout(namespace='test::vault::dto', path='test/vault/dto/vault'))
    )
    header = (out / 'include/test/vault/dto/vault.hpp').read_text()
    assert '#include <test/errors/errors.hpp>' in header
    assert '::test::errors::Error' in header
    assert 'struct Error' not in header


def test_list_outputs_and_dependencies(tmp_path):
    errors = _write(tmp_path / 'common/errors.yaml', ERRORS)
    vault = _write(tmp_path / 'schemas/vault.yaml', VAULT)
    out = str(tmp_path / 'out')

    outputs = _chaotic_gen('-o', out, '--list-outputs', vault)
    assert outputs.returncode == 0, outputs.stderr
    assert outputs.stdout.strip().split(';') == [
        os.path.join(out, file)
        for file in layout.output_files(
            layout.DocumentLayout(namespace='test::vault::dto', path='test/vault/dto/vault')
        )
    ]
    assert not (tmp_path / 'out').exists()

    dependencies = _chaotic_gen('-o', out, '--list-dependencies', vault)
    assert dependencies.returncode == 0, dependencies.stderr
    assert dependencies.stdout.strip() == errors


def test_document_without_types_generates_its_files(tmp_path):
    empty = _write(tmp_path / 'empty.yaml', 'x-usrv-cpp-namespace: test::empty\ncomponents: {}\n')
    out = tmp_path / 'out'

    result = _chaotic_gen('-o', str(out), '--clang-format=', empty)
    assert result.returncode == 0, result.stderr
    assert (out / 'include/test/empty/empty.hpp').read_text() == '#pragma once\n\n#include <test/empty/empty_fwd.hpp>\n'
    assert (out / 'src/test/empty/empty.cpp').exists()


@pytest.mark.parametrize(
    'args',
    [
        ['-n', '/components/schemas/([^/]*)/=ns::{0}'],
        ['--relative-to', '/'],
        ['-e', '/x'],
        ['--generate-serializers'],
        ['--no-sax-parse'],
        ['--no-stream-writer'],
        ['--parse-extra-formats'],
    ],
)
def test_mapping_options_are_forbidden(tmp_path, args):
    errors = _write(tmp_path / 'errors.yaml', ERRORS)
    result = _chaotic_gen('-o', str(tmp_path / 'out'), *args, errors)
    assert result.returncode == 1
    assert 'forbidden for documents with x-usrv-cpp-namespace' in result.stderr


def test_openapi_only_document_is_refused(tmp_path):
    api = _write(tmp_path / 'api.yaml', 'x-usrv-cpp-namespace: test::api\npaths:\n  /x: {}\n')
    result = _chaotic_gen('-o', str(tmp_path / 'out'), api)
    assert result.returncode == 1
    assert 'paths can only be generated by chaotic-openapi-gen --gen types' in result.stderr


def test_untagged_documents_require_mapping(tmp_path):
    untagged = _write(tmp_path / 'untagged.yaml', 'definitions: {}\n')
    result = _chaotic_gen('-o', str(tmp_path / 'out'), untagged)
    assert result.returncode == 1
    assert '--name-map and --relative-to are required' in result.stderr


def test_untagged_list_outputs_match_generation(tmp_path):
    schemas = tmp_path / 'schemas'
    one = _write(schemas / 'one.yaml', 'definitions:\n  A: {type: string}\n')
    two = _write(schemas / 'sub/two.yaml', 'definitions:\n  B: {type: integer}\n')
    out = tmp_path / 'out'
    mapping = ['-n', '/definitions/([^/]*)/=ns::{0}', '--relative-to', str(schemas)]

    listed = _chaotic_gen('-o', str(out), *mapping, '--list-outputs', one, two)
    assert listed.returncode == 0, listed.stderr
    assert not out.exists()

    generated = _chaotic_gen('-o', str(out), *mapping, '--clang-format=', one, two)
    assert generated.returncode == 0, generated.stderr
    assert sorted(listed.stdout.strip().split(';')) == sorted(str(path) for path in out.rglob('*') if path.is_file())

    dependencies = _chaotic_gen('-o', str(out), *mapping, '--list-dependencies', one, two)
    assert dependencies.returncode == 0, dependencies.stderr
    assert dependencies.stdout.strip() == ''
