#!/usr/bin/env python3
"""Verify the bank's exact FXP bytes in a ZIP or tar distribution.

SPDX-License-Identifier: GPL-3.0-or-later
Use repeated --member arguments to open nested archives from an Actions ZIP.
Archive paths are never extracted into the filesystem or executed. This checks
preset delivery; it does not install the synth or test a plugin host.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import shutil
import stat
import sys
import tarfile
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]
MAX_NESTED_BYTES = 4 * 1024 ** 3
MAX_PRESET_BYTES = 4 * 1024 ** 2


def digest_stream(stream):
    result = hashlib.sha256()
    while block := stream.read(1024 * 1024):
        result.update(block)
    return result.hexdigest()


class Archive:
    def __init__(self, stream):
        stream.seek(0)
        self.kind = 'zip' if zipfile.is_zipfile(stream) else 'tar'
        stream.seek(0)
        self.handle = (zipfile.ZipFile(stream) if self.kind == 'zip'
                       else tarfile.open(fileobj=stream, mode='r:*'))

    def entries(self):
        if self.kind == 'zip':
            for item in self.handle.infolist():
                mode = item.external_attr >> 16
                regular = not item.is_dir() and stat.S_IFMT(mode) in (0, stat.S_IFREG)
                yield item.filename, item.file_size, regular, item
        else:
            for item in self.handle.getmembers():
                yield item.name, item.size, item.isfile(), item

    def open(self, item):
        return (self.handle.open(item) if self.kind == 'zip'
                else self.handle.extractfile(item))

    def close(self):
        self.handle.close()


def bank_relative(name):
    # 7-Zip can supply Windows separators. Normalization here affects comparison
    # only; no archive path is ever used as a destination path.
    normalized = name.replace('\\', '/')
    parts = PurePosixPath(normalized).parts
    if 'Scoring 1000' not in parts or not normalized.lower().endswith('.fxp'):
        return None
    if PurePosixPath(normalized).is_absolute() or '..' in parts or ':' in normalized:
        raise ValueError('Unsafe bank member path: ' + name)
    if parts.count('Scoring 1000') != 1:
        raise ValueError('Ambiguous bank member path: ' + name)
    return '/'.join(parts[parts.index('Scoring 1000'):])


def inspect(archive_path, manifest_path, members, expected_presence):
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    presets = manifest['presets']
    expected = {str(PurePosixPath(p['path']).relative_to('resources/data/patches_factory')):
                p['sha256'] for p in presets}
    if len(presets) != 1000 or len(expected) != 1000:
        raise ValueError('A complete 1,000-preset manifest is required')
    streams, opened = [], []
    report = {'schema': 1, 'archive': archive_path.name, 'selected_members': members,
              'manifest_sha256': hashlib.sha256(manifest_bytes).hexdigest(),
              'script_sha256': hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
              'expected_presence': expected_presence, 'status': 'fail',
              'preset_count': 0, 'matching_hashes': 0, 'errors': [],
              'scope': 'Exact archive payload only; no installation, host execution or musical audition.'}
    try:
        stream = archive_path.open('rb')
        streams.append(stream)
        report['archive_sha256'] = digest_stream(stream)
        selected = Archive(stream)
        opened.append(selected)
        chain = []
        for member in members:
            matches = [entry for entry in selected.entries() if entry[0] == member]
            if len(matches) != 1:
                raise ValueError('Nested member must exist exactly once: ' + member)
            name, size, regular, item = matches[0]
            if not regular or size > MAX_NESTED_BYTES:
                raise ValueError('Nested archive must be a regular file at most 4 GiB: ' + name)
            nested = tempfile.SpooledTemporaryFile(max_size=32 * 1024 ** 2)
            streams.append(nested)
            with selected.open(item) as source:
                shutil.copyfileobj(source, nested, length=1024 * 1024)
            if nested.tell() != size:
                raise ValueError('Nested member size mismatch: ' + name)
            nested.seek(0)
            chain.append({'member': name, 'bytes': size, 'sha256': digest_stream(nested)})
            selected = Archive(nested)
            opened.append(selected)
        report['nested_archives'] = chain
        found = {}
        for name, size, regular, item in selected.entries():
            relative = bank_relative(name)
            if relative is None:
                continue
            if relative in found:
                report['errors'].append('Duplicate bank path: ' + relative)
                continue
            if not regular or size > MAX_PRESET_BYTES:
                report['errors'].append('Bank member is not a bounded regular file: ' + relative)
                found[relative] = None
                continue
            with selected.open(item) as source:
                found[relative] = digest_stream(source)
        report['preset_count'] = len(found)
        report['matching_hashes'] = sum(name in expected and expected[name] == sha
                                       for name, sha in found.items())
        if expected_presence == 'absent':
            if found:
                report['errors'].append('Expected no Scoring 1000 FXPs in this plugin-only archive')
        else:
            for name in sorted(set(expected) - set(found)):
                report['errors'].append('Missing: ' + name)
            for name in sorted(set(found) - set(expected)):
                report['errors'].append('Unexpected: ' + name)
            for name in sorted(set(found) & set(expected)):
                if found[name] != expected[name]:
                    report['errors'].append('SHA-256 mismatch: ' + name)
        report['status'] = 'fail' if report['errors'] else 'pass'
    except Exception as exc:
        report['errors'].append(f'{type(exc).__name__}: {exc}')
    finally:
        for handle in reversed(opened):
            handle.close()
        for stream in reversed(streams):
            stream.close()
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('archive', type=Path)
    parser.add_argument('--manifest', type=Path, default=ROOT / 'doc/scoring-1000/manifest.json')
    parser.add_argument('--member', action='append', default=[], help='Exact nested archive path; repeat for nesting')
    parser.add_argument('--expect', choices=('present', 'absent'), default='present')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    try:
        report = inspect(args.archive.resolve(), args.manifest.resolve(), args.member, args.expect)
    except Exception as exc:
        parser.error(str(exc))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes((json.dumps(report, indent=2) + '\n').encode())
    print(json.dumps({k: report[k] for k in ('status', 'preset_count', 'matching_hashes', 'expected_presence')}, indent=2))
    return 0 if report['status'] == 'pass' else 1


if __name__ == '__main__':
    sys.exit(main())
