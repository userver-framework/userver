#!/usr/bin/env python3
"""Guard: verify consistency between debian/*.cpack-components and cpack-metadata.json.

Two checks are performed:

  Forward check: every component declared in a *.cpack-components file must
  actually exist in cpack-metadata.json (i.e. was built).  A packaged-but-not-
  built component causes dh_cpack_substvars to fail with "Invalid CPack
  components in <pkg>".

  Reverse check: every component in cpack-metadata.json must be claimed by at
  least one *.cpack-components file.  An unclaimed built component leaves files
  in debian/tmp that no package owns, causing dh_missing to abort (compat 13).

Usage:
    check-cpack-components <cpack-metadata.json> <debian-dir>

Forward fix: add the component name to DISABLED_MODULES in
scripts/generate-debian-directory.sh (or disable its cmake feature flag in
scripts/debian-rules) and re-run generate-debian-directory.sh.

Reverse fix: add a deps/<DISTRO>/<component> file (may be empty) so the
component is picked up by ALL_MODULES in generate-debian-directory.sh, then
re-run generate-debian-directory.sh.  Special-cased components with non-dev
package names (like 'examples') need explicit handling in the generator.
"""

import json
import pathlib
import sys


def main() -> int:
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} <cpack-metadata.json> <debian-dir>', file=sys.stderr)
        return 2

    metadata_path = pathlib.Path(sys.argv[1])
    debian_dir = pathlib.Path(sys.argv[2])

    meta = json.loads(metadata_path.read_text())
    built_components = set(meta['components'].keys())

    # Collect all components claimed by any package.
    claimed_components: set[str] = set()
    errors: list[str] = []

    for comp_file in sorted(debian_dir.glob('*.cpack-components')):
        package = comp_file.name.replace('.cpack-components', '')
        for line in comp_file.read_text().splitlines():
            component = line.strip()
            if not component or component.startswith('#'):
                continue
            claimed_components.add(component)
            # Forward check: packaged component must have been built.
            if component not in built_components:
                errors.append(
                    f"Package '{package}' declares CPack component '{component}'"
                    f' but it was not built.\n'
                    f'  Built components: {sorted(built_components)}\n'
                    f"  Fix: add '{component}' to DISABLED_MODULES in"
                    f' scripts/generate-debian-directory.sh and re-run'
                    f' generate-debian-directory.sh.'
                )

    # Reverse check: every built component must be claimed by some package.
    orphaned = sorted(built_components - claimed_components)
    for component in orphaned:
        errors.append(
            f"CPack component '{component}' was built but is not claimed by any"
            f' debian package (no *.cpack-components file lists it).\n'
            f'  This will cause dh_missing to abort (compat 13).\n'
            f'  Fix: add a deps/<DISTRO>/{component} file (may be empty) so'
            f' generate-debian-directory.sh packages it, then re-run'
            f' generate-debian-directory.sh.'
        )

    if errors:
        for error in errors:
            print(f'ERROR: {error}', file=sys.stderr)
        return 1

    n_files = len(list(debian_dir.glob('*.cpack-components')))
    print(
        f'check-cpack-components: all {n_files} package(s) valid;'
        f' all {len(built_components)} built component(s) claimed.'
    )
    return 0


if __name__ == '__main__':
    sys.exit(main())
