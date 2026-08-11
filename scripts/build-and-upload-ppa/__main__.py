#!/usr/bin/env python3
"""Build and upload userver source package to Launchpad PPA.

Orchestrates:
  1. scripts/generate-debian-directory.sh  (generates debian/ + vendors pydantic wheels)
  2. debuild -S -sa                         (builds signed source package)
  3. dput ppa:userver-framework/userver    (uploads to Launchpad)

Usage:
  scripts/build-and-upload-ppa --distro ubuntu-24.04 --version-kind nightly
  scripts/build-and-upload-ppa --distro ubuntu-22.04 --version-kind release
"""

import argparse
import datetime
import os
import pathlib
import re
import subprocess
import sys

PPA = 'ppa:userver-framework/userver'

SUPPORTED_DISTROS = ['ubuntu-22.04', 'ubuntu-24.04']


def find_repo_root() -> pathlib.Path:
    root = pathlib.Path(__file__).resolve().parent.parent.parent
    missing = [p for p in ['version.txt', 'scripts/generate-debian-directory.sh'] if not (root / p).exists()]
    if missing:
        sys.exit(f'ERROR: repo root {root} is missing expected files: ' + ', '.join(missing))
    return root


def compute_version(root: pathlib.Path, version_kind: str) -> str:
    base = (root / 'version.txt').read_text().strip()
    if version_kind == 'release':
        return base
    timestamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%d%H%M')
    return f'{base}~{timestamp}'


def discover_signing_key() -> tuple[str, str, str]:
    """Return (keyid, debfullname, debemail) from the GPG secret keyring.

    Fails if there are 0 or >1 signing-capable secret keys.
    """
    result = subprocess.run(
        ['gpg', '--list-secret-keys', '--with-colons'],
        capture_output=True,
        text=True,
        check=True,
    )

    # Collect (keyid, uid_string) pairs for signing-capable keys.
    # Colon format: field 1=type, field 5=keyid, field 12=capabilities.
    # Capabilities field contains 's' for signing.
    # uid lines immediately following a sec block carry field 10 = "Name <email>".
    keys: list[tuple[str, str]] = []
    current_keyid: str | None = None

    for line in result.stdout.splitlines():
        fields = line.split(':')
        record_type = fields[0]

        if record_type == 'sec':
            caps = fields[11] if len(fields) > 11 else ''
            if 's' in caps:
                current_keyid = fields[4]
            else:
                current_keyid = None

        elif record_type == 'uid' and current_keyid is not None:
            uid_string = fields[9] if len(fields) > 9 else ''
            if uid_string:
                keys.append((current_keyid, uid_string))
                current_keyid = None  # take only the first uid per key

    if not keys:
        sys.exit('ERROR: no signing-capable GPG secret key found.\nGenerate one with: gpg --gen-key')

    if len(keys) > 1:
        hint = '\n'.join(f'  {keyid}  {uid}' for keyid, uid in keys)
        sys.exit(
            'ERROR: multiple signing-capable GPG secret keys found;\n'
            'key selection is not parameterized — please leave exactly one:\n' + hint
        )

    keyid, uid_string = keys[0]

    # Parse "Full Name <email@example.com>"
    match = re.match(r'^(.*?)\s*<([^>]+)>$', uid_string)
    if not match:
        sys.exit(
            f'ERROR: GPG key UID {uid_string!r} is not in "Name <email>" format.\n'
            'Update the key UID or set DEBEMAIL/DEBFULLNAME manually.'
        )

    debfullname = match.group(1).strip()
    debemail = match.group(2).strip()
    return keyid, debfullname, debemail


def build_env(
    base: dict[str, str],
    distro: str,
    version: str,
    debfullname: str,
    debemail: str,
) -> dict[str, str]:
    env = base.copy()
    env['DISTRO'] = distro
    env['VERSION'] = version
    env['DEBFULLNAME'] = debfullname
    env['DEBEMAIL'] = debemail

    # GPG_TTY is required for gpg-agent to prompt for the passphrase.
    if 'GPG_TTY' not in env:
        try:
            env['GPG_TTY'] = os.ttyname(sys.stdin.fileno())
        except OSError:
            pass  # not a tty; gpg may still work via agent/pinentry

    return env


def run(description: str, cmd: list[str], **kwargs) -> None:
    print(f'\n=== {description} ===', flush=True)
    print(' '.join(cmd), flush=True)
    try:
        subprocess.run(cmd, check=True, **kwargs)
    except subprocess.CalledProcessError as exc:
        sys.exit(f'ERROR: {description} failed with exit code {exc.returncode}')


def main() -> None:
    parser = argparse.ArgumentParser(
        description='Build and upload userver source package to Launchpad PPA.',
    )
    parser.add_argument(
        '--distro',
        required=True,
        choices=SUPPORTED_DISTROS,
        help='Target Ubuntu distro (e.g. ubuntu-24.04).',
    )
    parser.add_argument(
        '--version-kind',
        required=True,
        choices=['release', 'nightly'],
        help=('release: use version.txt as-is; nightly: append ~<UTC timestamp> to version.txt.'),
    )
    args = parser.parse_args()

    root = find_repo_root()
    os.chdir(root)

    version = compute_version(root, args.version_kind)
    keyid, debfullname, debemail = discover_signing_key()

    print(f'VERSION:     {version}')
    print(f'DISTRO:      {args.distro}')
    print(f'SIGNING KEY: {keyid}')
    print(f'DEBFULLNAME: {debfullname}')
    print(f'DEBEMAIL:    {debemail}')
    print(f'PPA:         {PPA}')

    env = build_env(
        os.environ.copy(),
        distro=args.distro,
        version=version,
        debfullname=debfullname,
        debemail=debemail,
    )

    # Step 1: generate debian/ and vendor pydantic wheels
    run(
        'Generating debian directory',
        ['scripts/generate-debian-directory.sh'],
        env=env,
    )

    # Step 2: build signed source package
    run(
        'Building source package',
        ['debuild', '-S', '-sa', f'-k{keyid}'],
        env=env,
    )

    # Step 3: upload
    changes_file = root.parent / f'userver_{version}_source.changes'
    if not changes_file.exists():
        sys.exit(f'ERROR: expected .changes file not found: {changes_file}\nCheck debuild output above.')

    run(
        'Uploading to Launchpad PPA',
        ['dput', PPA, str(changes_file)],
        env=env,
    )

    print()
    print('Upload complete.')
    print(f'  VERSION:  {version}')
    print(f'  changes:  {changes_file}')
    print()
    print(
        'NOTE: "Successfully uploaded" = FTP transfer only.\n'
        'Authoritative result is the Launchpad acceptance email\n'
        '(OpenPGP-encrypted; decrypt with: GPG_TTY=$(tty) gpg --decrypt).'
    )


if __name__ == '__main__':
    main()
