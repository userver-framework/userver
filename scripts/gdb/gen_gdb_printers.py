import argparse
import base64
import json
import marshal
import os
import sys
import zlib


def parse_args(args):
    parser = argparse.ArgumentParser(description='Embed a Python GDB script into a C++ source file')
    parser.add_argument('--userver-namespace', help='Emit the fully qualified userver namespace as debug metadata')
    parser.add_argument('output', help='Generated C++ source file')
    parser.add_argument('input', help='Python GDB script')
    return parser.parse_args(args)


def generate(printers_header: str, printers_script: str, userver_namespace: str | None) -> None:
    if userver_namespace is not None and userver_namespace and not userver_namespace.endswith('::'):
        raise ValueError('--userver-namespace must be empty or end with ::')

    rel_path = os.path.relpath(printers_script, os.getcwd())
    protection_macro = rel_path.lstrip('/').replace('/', '_').replace('.', '_').upper()

    with open(printers_script, encoding='utf-8') as script:
        bytecode = compile(script.read(), printers_script, 'exec')
    marshalized = base64.encodebytes(zlib.compress(marshal.dumps(bytecode)))
    string_len = 76 + 1  # chunk length 76 by RFC-2045 and extra \n by encodebytes()
    marshalized_split = '\n' + '\n'.join(
        str(marshalized[i : i + string_len]) for i in range(0, len(marshalized), string_len)
    )
    gdb_script_body = (
        'import gdb, gdb.printing, marshal, zlib, base64\n'
        f"exec(marshal.loads(zlib.decompress(base64.decodebytes({marshalized_split}))), {{'__name__': '__main__'}})"
    ).split('\n')

    namespace_metadata = ''
    if userver_namespace is not None:
        namespace_metadata = f"""\
namespace userver_gdb {{

const char kNamespace[] __attribute__((used, retain, section(".rodata.userver_gdb_namespace"))) =
    {json.dumps(userver_namespace)};

}}  // namespace userver_gdb

"""

    gdb_script_prelude = f'''\
// Auto-generated. DO NOT EDIT.

// NOLINTBEGIN
// clang-format off
#ifdef __ELF__

{namespace_metadata}\
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverlength-strings"
#endif

__asm__(
    ".pushsection \\".debug_gdb_scripts\\", \\"MSR\\",@progbits,1\\n"
    ".ascii \\"\\\\4gdb.inlined-script.{protection_macro}\\\\n\\"\\n"'''

    gdb_script_postlude = """
    ".byte 0\\n"
    ".popsection\\n");

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif  // __ELF__
// NOLINTEND
// clang-format on"""

    with open(printers_header, 'w', encoding='utf-8') as header:
        print(
            gdb_script_prelude,
            *(f'    ".ascii \\"{json.dumps(json.dumps(line)[1:-1])[1:-1]}\\\\n\\"\\n"' for line in gdb_script_body),
            gdb_script_postlude,
            sep='\n',
            file=header,
        )


def main() -> None:
    args = parse_args(sys.argv[1:])
    generate(args.output, args.input, args.userver_namespace)


if __name__ == '__main__':
    main()
