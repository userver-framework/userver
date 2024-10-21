import os
import os.path
import sys

if len(sys.argv) < 2:
    raise Exception("usage: update_gdbinit.py gdbinit_symlink [safe_path='~']")
gdbinit_symlink = os.path.realpath(sys.argv[1])
with open(gdbinit_symlink, 'x') as f:
    pass

HOME = os.getenv('HOME')
if HOME is None:
    print('HOME is not set, skip updating of ~/.gdbinit', file=sys.stderr)
    sys.exit(0)

safe_path = os.path.realpath(sys.argv[2] if len(sys.argv) >= 3 else HOME)

gdbinit_record = f'add-auto-load-safe-path {safe_path}'
gdbinit_path = os.path.join(HOME, '.gdbinit')

with open(gdbinit_path, 'a+') as f:
    if f.read().find(gdbinit_record) == -1:
        print(gdbinit_record, file=f)

if (
    not os.path.exists(gdbinit_symlink)
    or not os.path.islink(gdbinit_symlink)
    or os.readlink(gdbinit_symlink) != gdbinit_path
):
    if os.path.exists(gdbinit_symlink):
        os.unlink(gdbinit_symlink)
    os.symlink(gdbinit_path, gdbinit_symlink)
