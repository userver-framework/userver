# /// [pytest]
import subprocess


_EXPECTED_OUTPUT = """key:
  nested-key:
    - 1
    - 2
    - hello!
    - key-again: 42
"""


def test_basic(path_to_json2yaml):
    pipe = subprocess.Popen(
        [path_to_json2yaml],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        stdin=subprocess.PIPE,
    )
    stdout, stderr = pipe.communicate(
        input=b'{"key":{"nested-key": [1, 2.0, "hello!", {"key-again": 42}]}}',
    )
    assert stdout.decode('utf-8') == _EXPECTED_OUTPUT, stderr
    # /// [pytest]
