import argparse
import os
import shutil
import subprocess
import sys
import tempfile

SIMPLE_STYLE = '{BasedOnStyle: Google, ColumnLimit: 120}'


def is_binary_available(binary_name: str) -> bool:
    try:
        subprocess.check_call(['which', binary_name], stdout=subprocess.DEVNULL)
        return True
    except subprocess.CalledProcessError:
        return False


def parse_args():
    parser = argparse.ArgumentParser(description='Compare formatted source files.')
    parser.add_argument(
        '--golden-dir', required=True, help='Golden directory (e.g., ${CMAKE_CURRENT_SOURCE_DIR}/output)'
    )
    parser.add_argument(
        '--generated-dir', required=True, help='Generated directory (e.g., ${CMAKE_CURRENT_BINARY_DIR}/src)'
    )
    return parser.parse_args()


def walk_xpp(path: str) -> list[str]:
    result = []

    extensions = ('.hpp', '.cpp', '.ipp')
    for root, _, files in os.walk(path):
        for file in files:
            if file.lower().endswith(extensions):
                file_path = os.path.join(root, file)
                result.append(file_path)
    return result


def main():
    args = parse_args()

    if not is_binary_available('clang-format'):
        print('Error: clang-format is not available in PATH', file=sys.stderr)
        sys.exit(1)

    if not is_binary_available('diff'):
        print('Error: diff is not available in PATH', file=sys.stderr)
        sys.exit(1)

    with tempfile.TemporaryDirectory() as tmpdir:
        golden_copy = os.path.join(tmpdir, 'golden')
        generated_copy = os.path.join(tmpdir, 'generated')
        shutil.copytree(args.golden_dir, golden_copy)
        shutil.copytree(args.generated_dir, generated_copy)

        subprocess.check_call(['clang-format', '--style', SIMPLE_STYLE, '-i'] + walk_xpp(tmpdir))

        result = subprocess.run(['diff', '-uNrpB', golden_copy, generated_copy], capture_output=True, text=True)

        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)

        # Diff returns 0 if files are the same, 1 if they differ
        sys.exit(result.returncode)


if __name__ == '__main__':
    main()
