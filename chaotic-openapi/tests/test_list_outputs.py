import pathlib
import sys

import pytest

from chaotic_openapi import main

UNTAGGED_API = """\
openapi: 3.0.0
info: {title: api, version: '1.0'}
paths:
  /items/{item-id}:
    get:
      parameters:
        - {in: path, name: item-id, required: true, schema: {type: string}}
      responses:
        '200':
          description: OK
          content:
            application/json:
              schema: {$ref: 'schemas.yaml#/components/schemas/Item'}
    put:
      operationId: putItem
      requestBody:
        required: true
        content:
          application/json:
            schema: {type: object, additionalProperties: false, properties: {title: {type: string}}}
      responses:
        '200': {description: OK}
  /hidden:
    post:
      x-usrv-handler-codegen: false
      responses:
        '200': {description: OK}
"""

UNTAGGED_SCHEMAS = """\
openapi: 3.0.0
info: {title: schemas, version: '1.0'}
paths: {}
components:
  schemas:
    Item: {type: object, additionalProperties: false, properties: {id: {type: string}}}
"""

# A document without types still has its files
UNTAGGED_EMPTY = """\
openapi: 3.0.0
info: {title: empty, version: '1.0'}
paths: {}
"""

TAGGED_API = """\
openapi: 3.0.0
info: {title: api, version: '1.0'}
x-usrv-cpp-namespace: test::api
paths:
  /items/{id}:
    get:
      operationId: getItem
      parameters:
        - {in: path, name: id, required: true, schema: {type: string}}
      responses:
        '200':
          description: OK
          content:
            application/json:
              schema: {$ref: '../schemas/items.yaml#/components/schemas/Item'}
"""

TAGGED_SCHEMAS = """\
x-usrv-cpp-namespace: test::dto
x-usrv-cpp-path: test/dto/shop.items
components:
  schemas:
    Item: {type: object, additionalProperties: false, properties: {id: {type: string}}}
"""


def _write(path: pathlib.Path, content: str) -> str:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content)
    return str(path)


def _run(monkeypatch, capsys, *args: str) -> str:
    monkeypatch.setattr(sys, 'argv', ['chaotic-openapi', '--clang-format', '', '--no-check-includes', *args])
    capsys.readouterr()
    main.do_main()
    return capsys.readouterr().out


def _files(*roots: pathlib.Path) -> list[str]:
    return sorted(str(path) for root in roots if root.exists() for path in root.rglob('*') if path.is_file())


@pytest.fixture
def untagged(tmp_path) -> list[str]:
    return [
        _write(tmp_path / 'api/api.yaml', UNTAGGED_API),
        _write(tmp_path / 'api/schemas.yaml', UNTAGGED_SCHEMAS),
        _write(tmp_path / 'api/empty.yaml', UNTAGGED_EMPTY),
    ]


@pytest.fixture
def tagged(tmp_path) -> list[str]:
    _write(tmp_path / 'schemas/items.yaml', TAGGED_SCHEMAS)
    return [_write(tmp_path / 'api/api.yaml', TAGGED_API)]


@pytest.mark.parametrize(
    'documents,gen_args',
    [
        ('tagged', ['--gen', 'types']),
        *(
            (documents, gen_args)
            for documents in ('untagged', 'tagged')
            for gen_args in (
                ['--gen', 'client'],
                ['--gen', 'client', '--dynamic-config', 'API_QOS'],
                ['--gen', 'handlers'],
                ['--gen', 'handlers+views'],
            )
        ),
    ],
)
def test_list_outputs_match_generation(monkeypatch, capsys, request, tmp_path, documents, gen_args):
    files = request.getfixturevalue(documents)
    out = tmp_path / 'out'
    views = tmp_path / 'views'
    args = ['--name', 'api', '-o', str(out), '--src-dir', str(views), *gen_args, *files]

    listed = _run(monkeypatch, capsys, '--list-outputs', *args).strip().split(';')
    assert not out.exists()
    assert not views.exists()

    _run(monkeypatch, capsys, *args)
    assert sorted(listed) == _files(out, views)
