# userver GDB scripts

The generated static library contains a retained `.debug_gdb_scripts` ELF section. It is linked transitively through
`userver-universal`. The namespace is stored as retained per-objfile metadata, so the scripts also work with matching
split debug symbols.

To link the GDB scripts section to another CMake target explicitly:
`userver_link_gdb_scripts(${PROJECT_NAME})`

## Development

The expected folder structure is as follows:

- `pretty_printers/<component-namespace>/printers.py` or
  `pretty_printers/<component-namespace>/<header-only>_printers.py`
  A self-contained GDB pretty-printer script.
- `pretty_printers/cmd/<command-name>/cmd.py`
  A self-contained GDB command script.

Each embedded Python script runs with isolated globals and must be self-contained.

The library must be linked as a whole archive so that its object files are extracted. The generated
ELF sections use `SHF_GNU_RETAIN`, so `-Wl,--gc-sections` does not discard them afterward.

## Preparing an environment for script development

Run `python3 ./update_gdbinit.py --local-install` to register the GDB scripts on your system.

To prepare the environment manually:

```bash
USERVER_DIR=$PWD

# Make the GDB scripts available locally.
mkdir -p ${HOME}/.gdb/python/ && (
    cd ${HOME}/.gdb/python/
    ln -s ${USERVER_DIR}/scripts/gdb/install/pretty_printers ./userver_printers
    ln -s ${USERVER_DIR}/scripts/gdb/install/register_userver_printers.py ./
)

# Register the scripts during GDB initialization.
touch ${HOME}/.gdbinit
grep -q "register_userver_printers.py" ${HOME}/.gdbinit || (
    echo "source ${HOME}/.gdb/python/register_userver_printers.py" >> ${HOME}/.gdbinit
)
```

## References

- [GDB Initialization Files](https://www.sourceware.org/gdb/download/onlinedocs/gdb.html/Initialization-Files.html)
- [GDB Auto-load safe-path](https://www.sourceware.org/gdb/download/onlinedocs/gdb.html/Auto_002dloading-safe-path.html)
