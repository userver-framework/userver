#!/usr/bin/python
# Copyright 2024 Braden Ganetsky and Niall Douglas
# Distributed under the Boost Software License, Version 1.0.
# https://www.boost.org/LICENSE_1_0.txt

import os, sys
import datetime
import json
import marshal
import zlib
import base64

if len(sys.argv) != 3:
    print(f"{sys.argv[0]} <output.h> <input.py>")
    sys.exit(1)

printers_header = sys.argv[1]
printers_script = sys.argv[2]
protection_macro = (
    printers_header.lstrip("/").replace("/", "_").replace(".", "_").upper()
)

timestamp = datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%S")

# Grab the entire script
with open(printers_script, "r") as script:
    bytecode = compile(script.read(), printers_script, "exec")
marshalized = base64.encodebytes(zlib.compress(marshal.dumps(bytecode)))
string_len = 85
marshalized = "\n" + "\n".join(
    str(marshalized[i : i + string_len]) for i in range(0, len(marshalized), string_len)
)
print(marshalized)
new_script = f"import marshal, zlib, base64\nexec(marshal.loads(zlib.decompress(base64.decodebytes({marshalized}))))".split(
    "\n"
)

top_matter = f'''
// Auto-generated on {timestamp}. DO NOT EDIT.
#pragma once

// NOLINTBEGIN
#ifndef NDEBUG
#ifdef __ELF__

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverlength-strings"
#endif

__asm__(".pushsection \\".debug_gdb_scripts\\", \\"MS\\",@progbits,1\\n"
        ".ascii \\"\\\\4gdb.inlined-script.{protection_macro}\\\\n\\"\\n"'''

bottom_matter = f"""
        ".byte 0\\n"
        ".popsection\\n");

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#endif // __ELF__
#endif // NDEBUG
// NOLINTEND"""

# Write the inline asm header
with open(printers_header, "wt") as header:
    print(
        top_matter,
        *(
            f'        ".ascii \\"{json.dumps(json.dumps(line)[1:-1])[1:-1]}\\\\n\\"\\n"'
            for line in new_script
        ),
        bottom_matter,
        sep="\n",
        file=header,
    )
