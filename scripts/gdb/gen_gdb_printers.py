import base64
import json
import marshal
import os
import sys
import zlib

if len(sys.argv) != 3:
    print(f'{sys.argv[0]} <output.h> <input.py>')
    sys.exit(1)

printers_header = sys.argv[1]
printers_script = sys.argv[2]
protection_macro = (
    os.path.relpath(printers_script, os.getcwd())
    .lstrip('/')
    .replace('/', '_')
    .replace('.', '_')
    .upper()
)

# Grab the entire script
with open(printers_script, 'r') as script:
    bytecode = compile(script.read(), printers_script, 'exec')
marshalized = base64.encodebytes(zlib.compress(marshal.dumps(bytecode)))
string_len = 80
marshalized_splitted = '\n' + '\n'.join(
    str(marshalized[i : i + string_len])
    for i in range(0, len(marshalized), string_len)
)
print(marshalized_splitted)
new_script = f'import marshal, zlib, base64\nexec(marshal.loads(zlib.decompress(base64.decodebytes({marshalized_splitted}))))'.split(
    '\n',
)

top_matter = f'''
// Auto-generated. DO NOT EDIT.
#pragma once

// NOLINTBEGIN
// clang-format off
#ifdef USERVER_EMBEDDED_GDB_PRINTERS
#ifdef __ELF__

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverlength-strings"
#endif

__asm__(
    ".pushsection \\".debug_gdb_scripts\\", \\"MS\\",@progbits,1\\n"
    ".ascii \\"\\\\4gdb.inlined-script.{protection_macro}\\\\n\\"\\n"'''

bottom_matter = f"""
    ".byte 0\\n"
    ".popsection\\n");

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif  // __ELF__
#endif  // USERVER_EMBEDDED_GDB_PRINTERS
// NOLINTEND
// clang-format on"""

# Write the inline asm header
with open(printers_header, 'wt') as header:
    print(
        top_matter,
        *(
            f'    ".ascii \\"{json.dumps(json.dumps(line)[1:-1])[1:-1]}\\\\n\\"\\n"'
            for line in new_script
        ),
        bottom_matter,
        sep='\n',
        file=header,
    )
