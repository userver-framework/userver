import pathlib
import sys

import pytest

from chaotic_openapi import main

_OPENAPI_SCHEMA = """\
openapi: 3.0.0
info:
  title: Test service
  version: '1.0'
paths:
  /test:
    get:
      operationId: testGet
      responses:
        '200':
          description: OK
"""


def _run_codegen(
    monkeypatch,
    schema_path: pathlib.Path,
    output_dir: pathlib.Path,
    *,
    gen: str,
    dynamic_config: str = '',
) -> None:
    args = [
        'chaotic-openapi',
        '--name',
        'test_service',
        '--gen',
        gen,
        '--output-dir',
        str(output_dir),
        '--clang-format',
        '',
        '--no-check-includes',
    ]
    if dynamic_config:
        args.extend(['--dynamic-config', dynamic_config])
    args.append(str(schema_path))

    monkeypatch.setattr(sys, 'argv', args)
    main.do_main()


def _prepare_schema(tmp_path: pathlib.Path) -> pathlib.Path:
    schema_path = tmp_path / 'openapi.yaml'
    schema_path.write_text(_OPENAPI_SCHEMA)
    return schema_path


def _qos_header(output_dir: pathlib.Path) -> pathlib.Path:
    return output_dir / 'include' / 'clients' / 'test_service' / 'qos.hpp'


def test_client_without_qos_removes_previously_generated_header(monkeypatch, tmp_path):
    schema_path = _prepare_schema(tmp_path)
    output_dir = tmp_path / 'generated'

    _run_codegen(
        monkeypatch,
        schema_path,
        output_dir,
        gen='client',
        dynamic_config='TEST_SERVICE_QOS',
    )
    assert _qos_header(output_dir).is_file()

    _run_codegen(monkeypatch, schema_path, output_dir, gen='client')

    assert not _qos_header(output_dir).exists()


def test_client_without_qos_is_idempotent(monkeypatch, tmp_path):
    schema_path = _prepare_schema(tmp_path)
    output_dir = tmp_path / 'generated'
    client_header = output_dir / 'include' / 'clients' / 'test_service' / 'client.hpp'

    _run_codegen(monkeypatch, schema_path, output_dir, gen='client')
    first_client_header = client_header.read_text()

    _run_codegen(monkeypatch, schema_path, output_dir, gen='client')

    assert client_header.read_text() == first_client_header
    assert not _qos_header(output_dir).exists()


def test_handler_codegen_preserves_client_qos_header(monkeypatch, tmp_path):
    schema_path = _prepare_schema(tmp_path)
    output_dir = tmp_path / 'generated'

    _run_codegen(
        monkeypatch,
        schema_path,
        output_dir,
        gen='client',
        dynamic_config='TEST_SERVICE_QOS',
    )
    qos_header = _qos_header(output_dir)
    original_qos = qos_header.read_text()

    _run_codegen(monkeypatch, schema_path, output_dir, gen='handlers')

    assert qos_header.read_text() == original_qos


def _generate_handlers(monkeypatch, tmp_path: pathlib.Path, *extra: str) -> pathlib.Path:
    output_dir = tmp_path / 'generated'
    monkeypatch.setattr(
        sys,
        'argv',
        [
            'chaotic-openapi',
            '--name',
            'test_service',
            '--gen',
            'handlers',
            '--output-dir',
            str(output_dir),
            '--clang-format',
            '',
            '--no-check-includes',
            *extra,
            str(_prepare_schema(tmp_path)),
        ],
    )
    main.do_main()
    return output_dir / 'include' / 'handlers' / 'test_service' / 'testget' / 'handler.hpp'


def test_handler_policy(monkeypatch, tmp_path):
    handler = _generate_handlers(
        monkeypatch,
        tmp_path,
        '--handler-policy',
        'service::HandlerPolicy',
        '--handler-policy-include',
        'service/handler_policy.hpp',
    ).read_text()

    assert '#include <service/handler_policy.hpp>' in handler
    # No hidden query arguments: their slot is filled with void before the policy
    assert 'View,void,service::HandlerPolicy>' in ''.join(handler.split())


def test_handler_without_policy(monkeypatch, tmp_path):
    handler = _generate_handlers(monkeypatch, tmp_path).read_text()

    assert 'Policy' not in handler
    assert 'View>' in ''.join(handler.split())


def test_handler_policy_requires_include(monkeypatch, tmp_path, capsys):
    with pytest.raises(SystemExit):
        _generate_handlers(monkeypatch, tmp_path, '--handler-policy', 'service::HandlerPolicy')
    assert '--handler-policy and --handler-policy-include must be passed together' in capsys.readouterr().err
