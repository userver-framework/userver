import argparse
import os
import shutil
import subprocess
import sys
import tempfile


def check_binary_available(binary_name):
    try: # Pass version arg to expect any "wait for input" situations
        subprocess.run([binary_name, '--version'], capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False


def main():
    parser = argparse.ArgumentParser(description='Compare formatted source files.')
    parser.add_argument('--golden-dir', required=True, help='Golden directory (e.g., ${CMAKE_CURRENT_SOURCE_DIR}/output)')
    parser.add_argument('--generated-dir', required=True, help='Generated directory (e.g., ${CMAKE_CURRENT_BINARY_DIR}/src)')
    args = parser.parse_args()

    if not check_binary_available('clang-format'):
        print("Error: clang-format is not available in PATH", file=sys.stderr)
        sys.exit(1)

    if not check_binary_available('diff'):
        print("Error: diff is not available in PATH", file=sys.stderr)
        sys.exit(1)

    # Create temporary directory in /tmp/
    with tempfile.TemporaryDirectory() as tmpdir:
        golden_copy = os.path.join(tmpdir, 'golden')
        generated_copy = os.path.join(tmpdir, 'generated')
        shutil.copytree(args.golden_dir, golden_copy)
        shutil.copytree(args.generated_dir, generated_copy)

        extensions = ('.hpp', '.cpp', '.ipp')
        for root, _, files in os.walk(tmpdir):
            for file in files:
                if file.lower().endswith(extensions):
                    file_path = os.path.join(root, file)
                    subprocess.run(['clang-format', '-i', file_path], check=True)

        result = subprocess.run([
            'diff', '-uNrpB',
            golden_copy,
            generated_copy
        ], capture_output=True, text=True)

        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr, file=sys.stderr)

        sys.exit(result.returncode)  # Diff returns 0 if files are the same, 1 if they differ


if __name__ == '__main__':
    main()
