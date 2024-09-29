import importlib
import marshal

import compileall

import pretty_printers

with open(pretty_printers.__file__) as f:
    code = compile(f.read(), pretty_printers.__file__, "exec", optimize=2)
print(marshal.dumps(code))
print(exec(code))
