import argparse
import os
import os.path
import sys

HOMEDIR = os.getenv('HOME')
if HOMEDIR is None:
    print('HOME is not set, skip updating of ~/.gdbinit', file=sys.stderr)
    sys.exit(0)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--local-install',
        action='store_true',
        help='Install userver_printers locally into `~/.gdb/python/` folder and update `~/.gdbinit`. Conflicts with `gdbinit_symlink`',
    )
    parser.add_argument(
        '--gdbinit-path', type=str, help='Path to `.gdbinit` file', default=os.path.join(HOMEDIR, '.gdbinit')
    )
    parser.add_argument(
        'gdbinit_symlink', nargs='?', help='Symlink to `--gdbinit-path`. Conflicts with `--local-install`'
    )
    parser.add_argument('safe_path', nargs='?', help='Default: $HOME')

    args = parser.parse_args()
    if args.local_install and args.gdbinit_symlink:
        print('ERROR: `--local-install` and `gdbinit_symlink` is conflicted to each other.', file=sys.stderr)
        parser.help()
        os.exit(1)
    elif not args.local_install and not args.gdbinit_symlink:
        print('ERROR: One of `--local-install` or `gdbinit_symlink` argument is missing.', file=sys.stderr)
        parser.help()
        os.exit(1)

    if args.gdbinit_symlink:
        args.gdbinit_symlink = os.path.realpath(args.gdbinit_symlink)
        args.safe_path = os.path.realpath(args.safe_path if args.safe_path else HOMEDIR)
    return args


def update_gdbinit_with_record(gdbinit_path, record: str):
    with open(gdbinit_path, 'a+') as f:
        f.seek(0)
        if f.read().find(record) == -1:
            print(record, file=f)
            print(f"Updated `{gdbinit_path}` with '{record}'.")
        else:
            print(f"Config file `{gdbinit_path}` already contains '{record}'. Do nothing.")


def install_gdb_sectioned_printers(args):
    with open(args.gdbinit_symlink, 'x') as _:
        pass

    update_gdbinit_with_record(
        args.gdbinit_path,
        record=f'add-auto-load-safe-path {args.safe_path}',
    )

    if (
        not os.path.exists(args.gdbinit_symlink)
        or not os.path.islink(args.gdbinit_symlink)
        or os.readlink(args.gdbinit_symlink) != args.gdbinit_path
    ):
        if os.path.exists(args.gdbinit_symlink):
            os.unlink(args.gdbinit_symlink)
        os.symlink(args.gdbinit_path, args.gdbinit_symlink)


def install_local_printers(args):
    gdb_py_path = os.path.join(HOMEDIR, '.gdb', 'python')
    if not os.path.exists(gdb_py_path):
        os.makedirs(gdb_py_path)
        print(f'Created GDB python scripts path: {gdb_py_path}')
    elif not os.path.isdir(os.path.realpath(gdb_py_path)):
        print(f'GDB python scripts path is not a folder: {gdb_py_path}', file=sys.stderr)
        os.exit(1)
    else:
        print(f'GDB python scripts path `{gdb_py_path}` is exists. Do nothing.')

    curdir = os.path.dirname(__file__)

    registration_script_name = 'register_userver_printers.py'
    registration_script = os.path.join(curdir, 'install', 'register_userver_printers.py')
    registration_script_symlink = os.path.join(gdb_py_path, registration_script_name)
    if not os.path.exists(registration_script_symlink):
        os.symlink(registration_script, registration_script_symlink)
        print(f'Created symlink to registration script: {registration_script_symlink}')
    else:
        print(f'Symlink to registration script is already exists: {registration_script_symlink}')

    printers = os.path.join(curdir, 'pretty_printers')
    printers_symlink = os.path.join(gdb_py_path, 'userver_printers')
    if not os.path.exists(printers_symlink):
        os.symlink(printers, printers_symlink)
        print(f'Created symlink to pretty_printers: {printers_symlink}')
    else:
        print(f'Symlink to registration script is already exists: {printers_symlink}')

    update_gdbinit_with_record(
        args.gdbinit_path,
        record=f'source {registration_script_symlink}',
    )


if __name__ == '__main__':
    args = parse_args()
    if not args.local_install:
        install_gdb_sectioned_printers(args)
    else:
        install_local_printers(args)
