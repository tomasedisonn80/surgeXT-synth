#!/usr/bin/env python3
"""Build a deterministic, verified standalone ZIP of the Scoring 1000 bank.

SPDX-License-Identifier: GPL-3.0-or-later
This packages original preset data; it does not certify audio or musical audition.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import sys
import tempfile
import zipfile

from validate_bank import validate


ROOT = Path(__file__).resolve().parents[2]
FACTORY_PREFIX = PurePosixPath('resources/data/patches_factory')
FIXED_TIMESTAMP = (1980, 1, 1, 0, 0, 0)
LICENSE_TEXT = 'Creative Commons Legal Code\n\nCC0 1.0 Universal\n\n    CREATIVE COMMONS CORPORATION IS NOT A LAW FIRM AND DOES NOT PROVIDE\n    LEGAL SERVICES. DISTRIBUTION OF THIS DOCUMENT DOES NOT CREATE AN\n    ATTORNEY-CLIENT RELATIONSHIP. CREATIVE COMMONS PROVIDES THIS\n    INFORMATION ON AN "AS-IS" BASIS. CREATIVE COMMONS MAKES NO WARRANTIES\n    REGARDING THE USE OF THIS DOCUMENT OR THE INFORMATION OR WORKS\n    PROVIDED HEREUNDER, AND DISCLAIMS LIABILITY FOR DAMAGES RESULTING FROM\n    THE USE OF THIS DOCUMENT OR THE INFORMATION OR WORKS PROVIDED\n    HEREUNDER.\n\nStatement of Purpose\n\nThe laws of most jurisdictions throughout the world automatically confer\nexclusive Copyright and Related Rights (defined below) upon the creator\nand subsequent owner(s) (each and all, an "owner") of an original work of\nauthorship and/or a database (each, a "Work").\n\nCertain owners wish to permanently relinquish those rights to a Work for\nthe purpose of contributing to a commons of creative, cultural and\nscientific works ("Commons") that the public can reliably and without fear\nof later claims of infringement build upon, modify, incorporate in other\nworks, reuse and redistribute as freely as possible in any form whatsoever\nand for any purposes, including without limitation commercial purposes.\nThese owners may contribute to the Commons to promote the ideal of a free\nculture and the further production of creative, cultural and scientific\nworks, or to gain reputation or greater distribution for their Work in\npart through the use and efforts of others.\n\nFor these and/or other purposes and motivations, and without any\nexpectation of additional consideration or compensation, the person\nassociating CC0 with a Work (the "Affirmer"), to the extent that he or she\nis an owner of Copyright and Related Rights in the Work, voluntarily\nelects to apply CC0 to the Work and publicly distribute the Work under its\nterms, with knowledge of his or her Copyright and Related Rights in the\nWork and the meaning and intended legal effect of CC0 on those rights.\n\n1. Copyright and Related Rights. A Work made available under CC0 may be\nprotected by copyright and related or neighboring rights ("Copyright and\nRelated Rights"). Copyright and Related Rights include, but are not\nlimited to, the following:\n\n  i. the right to reproduce, adapt, distribute, perform, display,\n     communicate, and translate a Work;\n ii. moral rights retained by the original author(s) and/or performer(s);\niii. publicity and privacy rights pertaining to a person\'s image or\n     likeness depicted in a Work;\n iv. rights protecting against unfair competition in regards to a Work,\n     subject to the limitations in paragraph 4(a), below;\n  v. rights protecting the extraction, dissemination, use and reuse of data\n     in a Work;\n vi. database rights (such as those arising under Directive 96/9/EC of the\n     European Parliament and of the Council of 11 March 1996 on the legal\n     protection of databases, and under any national implementation\n     thereof, including any amended or successor version of such\n     directive); and\nvii. other similar, equivalent or corresponding rights throughout the\n     world based on applicable law or treaty, and any national\n     implementations thereof.\n\n2. Waiver. To the greatest extent permitted by, but not in contravention\nof, applicable law, Affirmer hereby overtly, fully, permanently,\nirrevocably and unconditionally waives, abandons, and surrenders all of\nAffirmer\'s Copyright and Related Rights and associated claims and causes\nof action, whether now known or unknown (including existing as well as\nfuture claims and causes of action), in the Work (i) in all territories\nworldwide, (ii) for the maximum duration provided by applicable law or\ntreaty (including future time extensions), (iii) in any current or future\nmedium and for any number of copies, and (iv) for any purpose whatsoever,\nincluding without limitation commercial, advertising or promotional\npurposes (the "Waiver"). Affirmer makes the Waiver for the benefit of each\nmember of the public at large and to the detriment of Affirmer\'s heirs and\nsuccessors, fully intending that such Waiver shall not be subject to\nrevocation, rescission, cancellation, termination, or any other legal or\nequitable action to disrupt the quiet enjoyment of the Work by the public\nas contemplated by Affirmer\'s express Statement of Purpose.\n\n3. Public License Fallback. Should any part of the Waiver for any reason\nbe judged legally invalid or ineffective under applicable law, then the\nWaiver shall be preserved to the maximum extent permitted taking into\naccount Affirmer\'s express Statement of Purpose. In addition, to the\nextent the Waiver is so judged Affirmer hereby grants to each affected\nperson a royalty-free, non transferable, non sublicensable, non exclusive,\nirrevocable and unconditional license to exercise Affirmer\'s Copyright and\nRelated Rights in the Work (i) in all territories worldwide, (ii) for the\nmaximum duration provided by applicable law or treaty (including future\ntime extensions), (iii) in any current or future medium and for any number\nof copies, and (iv) for any purpose whatsoever, including without\nlimitation commercial, advertising or promotional purposes (the\n"License"). The License shall be deemed effective as of the date CC0 was\napplied by Affirmer to the Work. Should any part of the License for any\nreason be judged legally invalid or ineffective under applicable law, such\npartial invalidity or ineffectiveness shall not invalidate the remainder\nof the License, and in such case Affirmer hereby affirms that he or she\nwill not (i) exercise any of his or her remaining Copyright and Related\nRights in the Work or (ii) assert any associated claims and causes of\naction with respect to the Work, in either case contrary to Affirmer\'s\nexpress Statement of Purpose.\n\n4. Limitations and Disclaimers.\n\n a. No trademark or patent rights held by Affirmer are waived, abandoned,\n    surrendered, licensed or otherwise affected by this document.\n b. Affirmer offers the Work as-is and makes no representations or\n    warranties of any kind concerning the Work, express, implied,\n    statutory or otherwise, including without limitation warranties of\n    title, merchantability, fitness for a particular purpose, non\n    infringement, or the absence of latent or other defects, accuracy, or\n    the present or absence of errors, whether or not discoverable, all to\n    the greatest extent permissible under applicable law.\n c. Affirmer disclaims responsibility for clearing rights of other persons\n    that may apply to the Work or any use thereof, including without\n    limitation any person\'s Copyright and Related Rights in the Work.\n    Further, Affirmer disclaims responsibility for obtaining any necessary\n    consents, permissions or other rights required for any use of the\n    Work.\n d. Affirmer understands and acknowledges that Creative Commons is not a\n    party to this document and has no duty or obligation with respect to\n    this CC0 or use of the Work.\n'
LICENSE_NOTICE = '''Scoring 1000 license and provenance

SPDX-License-Identifier: CC0-1.0

The original Scoring 1000 preset data, descriptive catalog, package metadata,
and installation notes in this archive are distributed under CC0 1.0 Universal.
The complete legal text is in LICENSE-CC0.txt.
Canonical legal code: https://creativecommons.org/publicdomain/zero/1.0/legalcode

The native FXP container is initialized from Surge's CC0 Init Saw factory patch.
The authored oscillator, envelope, filter, modulation and effect recipes are
original; the archive does not import commercial sample libraries or composers'
presets. The presets need no external samples or wavetables.

The Surge XT program and this project's packaging/compiler programs retain their
source-repository licenses (including GPL-3.0-or-later). Those programs and engine
binaries are not included in this data archive, and CC0 is not being applied to
the entire Surge XT repository or its dependencies.

No artist, film studio, manufacturer or composer endorsement is claimed.
'''
INSTALL_README = '''# Scoring 1000 — standalone preset bank

This archive contains 1,000 native Surge XT FXP presets: ten categories of 100,
with 20 original synthesis families and five purposeful variants in each category.
It contains preset data and documentation; install Surge XT separately.

## Install in your user patch collection

1. Extract the ZIP into a temporary folder.
2. In Surge XT's patch-selector context menu, choose **Open User Patches Folder...**.
   This is the **Patches** folder beneath the configured user data folder. Using
   the menu respects custom and portable locations.
3. Copy the extracted **Scoring 1000** folder into that user Patches folder.
   Preserve the ten numbered category folders beneath it. Do not replace the
   entire Patches directory or overwrite your unrelated patches.
4. Refresh the patch browser or restart the synth, then find **Scoring 1000** in
   the user collection. If this bank already exists there, keep personal edits
   in a separate user folder before updating its original files.

Full factory-data installers built from the source repository already include the
bank in the factory collection. Plugin-only archives and minimal installations
can omit factory data. This standalone archive provides an additional user-bank
installation route; it does not embed presets into a plugin binary.

## Play and adapt

The eight consistent macros are Tone, Motion, Contour, Space, Color, Width,
Attack and Release. Space starts at 40%; set it to zero for a dry comparison.
Motion and Width at zero retain authored movement/spread rather than guaranteeing
stillness or mono. Velocity shapes dynamics; the wheel adds tone/motion, and
channel pressure adds vibrato. Times on analog envelopes are nominal settings.
Set track level with your intended phrase, especially with overlapping releases.

The catalog in **manifest.json** gives each preset's stable ID, native name,
musical role, suggested MIDI range, variant intent, macros and source SHA-256.
Catalog paths describe the source repository. **PACKAGE.json** maps those paths
to the actual archive paths, and **CONTENTS.sha256** verifies archive contents.
The companion .sha256 file outside the ZIP verifies the complete archive.

These are synthetic colors, including evocative keyboard, ensemble and vocal-like
textures; they do not contain recorded acoustic instruments. Technical packaging
checks do not establish listening approval or universal genre suitability.
Human audition and composer endorsement retain the statuses stated in the catalog.

Player guide and research:
https://github.com/tomasedisonn80/surgeXT-synth/blob/main/doc/scoring-1000/USER_GUIDE.md
https://github.com/tomasedisonn80/surgeXT-synth/blob/main/doc/scoring-1000/RESEARCH.md
Those online documents follow main; this archive's catalog and checksums identify
its exact preset data. Licensing and provenance are in LICENSE-NOTICE.txt.
'''


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def encoded_json(value) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=True) + '\n').encode('utf-8')


def safe_name(name: str) -> None:
    path = PurePosixPath(name)
    if (path.is_absolute() or '..' in path.parts or '\\' in name or ':' in name
            or any(ord(c) < 32 for c in name) or str(path) != name):
        raise ValueError(f'Unsafe archive path: {name!r}')


def package_bank(root: Path, output: Path) -> dict:
    root, output = root.resolve(), output.resolve()
    if output.suffix.lower() != '.zip':
        raise ValueError('--output must end in .zip')
    if output.is_relative_to(root / 'resources/data'):
        raise ValueError('Do not write distribution archives into the factory data tree')
    manifest_path = root / 'doc/scoring-1000/manifest.json'
    manifest_bytes = manifest_path.read_bytes()
    manifest = json.loads(manifest_bytes)
    report = validate(root=root, check_generated=True)
    if not report['ok'] or not report['complete_bank']:
        raise ValueError('Bank validation failed: ' + '; '.join(report['errors'][:12]))
    if report['manifest_sha256'] != digest(manifest_bytes):
        raise ValueError('Catalog changed during validation; retry after generation finishes')
    if manifest.get('license') != 'CC0-1.0':
        raise ValueError('Archive license notice requires a CC0-1.0 bank catalog')

    payload = {
        'README_INSTALL.md': INSTALL_README.encode('utf-8'),
        'LICENSE-CC0.txt': LICENSE_TEXT.encode('utf-8'),
        'LICENSE-NOTICE.txt': LICENSE_NOTICE.encode('utf-8'),
        'manifest.json': manifest_bytes,
    }
    mapping = []
    for preset in manifest['presets']:
        source = PurePosixPath(preset['path'])
        archive_path = str(source.relative_to(FACTORY_PREFIX))
        safe_name(archive_path)
        if archive_path in payload:
            raise ValueError(f'Duplicate archive entry: {archive_path}')
        data = (root / str(source)).read_bytes()
        if digest(data) != preset['sha256']:
            raise ValueError(f'Preset changed after validation: {preset["id"]}')
        payload[archive_path] = data
        mapping.append({'id': preset['id'], 'source_path': str(source),
                        'archive_path': archive_path, 'sha256': preset['sha256']})
    metadata = {
        'schema': 1, 'name': 'Scoring 1000', 'preset_count': 1000,
        'category_counts': report['category_counts'],
        'manifest_sha256': digest(manifest_bytes),
        'generator_sha256': digest(Path(__file__).read_bytes()),
        'static_validator_sha256': digest((Path(__file__).parent / 'validate_bank.py').read_bytes()),
        'license': 'CC0-1.0', 'zip_timestamp': list(FIXED_TIMESTAMP),
        'validation_scope': 'Complete static integrity, exact regeneration and archive-byte verification; no new audio or musical-audition claim.',
        'presets': sorted(mapping, key=lambda item: item['id']),
    }
    payload['PACKAGE.json'] = encoded_json(metadata)
    payload['CONTENTS.sha256'] = ''.join(
        f'{digest(data)}  {name}\n' for name, data in sorted(payload.items())
    ).encode('utf-8')
    if manifest_path.read_bytes() != manifest_bytes:
        raise ValueError('Catalog changed during collection; retry after generation finishes')

    output.parent.mkdir(parents=True, exist_ok=True)
    fd, temporary_name = tempfile.mkstemp(prefix=output.name + '.', suffix='.tmp', dir=output.parent)
    os.close(fd)
    temporary = Path(temporary_name)
    try:
        with zipfile.ZipFile(temporary, 'w', compression=zipfile.ZIP_DEFLATED,
                             compresslevel=9, strict_timestamps=True) as archive:
            for name, data in sorted(payload.items()):
                safe_name(name)
                info = zipfile.ZipInfo(name, date_time=FIXED_TIMESTAMP)
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                info.compress_type = zipfile.ZIP_DEFLATED
                archive.writestr(info, data, compress_type=zipfile.ZIP_DEFLATED, compresslevel=9)
        with zipfile.ZipFile(temporary) as archive:
            if archive.namelist() != sorted(payload) or archive.testzip() is not None:
                raise ValueError('Archive entry/CRC verification failed')
            for name, expected in payload.items():
                if archive.read(name) != expected:
                    raise ValueError(f'Archive byte verification failed: {name}')
        archive_bytes = temporary.read_bytes()
        result = {
            'schema': 1, 'archive': output.name, 'sha256': digest(archive_bytes),
            'bytes': len(archive_bytes), 'entries': len(payload), 'preset_count': 1000,
            'manifest_sha256': digest(manifest_bytes),
            'generator_sha256': metadata['generator_sha256'],
            'static_validation': 'pass', 'regeneration_validation': 'pass',
            'archive_byte_verification': 'pass',
            'musical_audition': 'unchanged_from_catalog', 'composer_endorsement': 'not_claimed',
            'reproducibility': 'Identical input bytes, Python zip implementation and zlib version produce identical ZIP bytes; no clock or filesystem mtimes are recorded.',
        }
        temporary.replace(output)
        output.with_name(output.name + '.sha256').write_text(f'{result["sha256"]}  {output.name}\n')
        output.with_name(output.name + '.metadata.json').write_bytes(encoded_json(result))
        return result
    finally:
        if temporary.exists():
            temporary.unlink()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT, help='Source repository root')
    parser.add_argument('--output', required=True, type=Path, help='Destination .zip file')
    args = parser.parse_args()
    try:
        result = package_bank(args.root, args.output)
    except (OSError, ValueError, KeyError, TypeError, zipfile.BadZipFile) as error:
        print(f'Packaging failed: {error}', file=sys.stderr)
        return 1
    print(json.dumps(result, indent=2))
    return 0


if __name__ == '__main__':
    sys.exit(main())
