import subprocess


def test_basic(service_binary):
    result = subprocess.run([service_binary, 'localhost'], check=True, capture_output=True)
    stderr = str(result.stderr)
    assert '- 127.0.0.1' in stderr or '- [::1]' in stderr
