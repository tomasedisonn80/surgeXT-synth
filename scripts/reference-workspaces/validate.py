#!/usr/bin/env python3
"""Validate native reference workspace packages with Python's standard library.

Geometry/data validation is intentionally separate from native ParameterBridge
resolution and screenshot/interaction testing. Passing this script is not proof
of visual fidelity, accessibility, or a working processor parameter binding.
"""

from __future__ import annotations

import argparse
import copy
import json
import math
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path, PurePosixPath

MAX_BYTES = 1024 * 1024
STYLES = set("precision ivory spectral bauhaus space tokyo aurora clear flow live".split())
FONTS = {"sans", "serif", "mono"}
PALETTE = set("background panel border text muted accent secondary display knob red yellow blue".split())
PANELS = set("oscillator filter mixer envelope lfo macros matrix effects analyzer waveform browser routing scene output xy keyboard inspector amp tabs".split())
VARIANTS = set("compact horizontal vertical large combined terrain flow faders knobs".split())
CONTROLS = {"knob", "fader", "horizontal", "menu", "toggle"}
IDENTIFIER = re.compile(r"[a-zA-Z0-9_.-]{1,80}\Z")
COLOUR = re.compile(r"#[0-9a-fA-F]{6}(?:[0-9a-fA-F]{2})?\Z")


class InvalidManifest(ValueError):
    """A package is unsafe, malformed, or inconsistent with schema one."""


def require(condition: bool, message: str) -> None:
    if not condition:
        raise InvalidManifest(message)


def numeric(value: object) -> bool:
    if not isinstance(value, (int, float)) or isinstance(value, bool):
        return False
    try:
        return math.isfinite(value)
    except OverflowError:
        return False


def text_field(obj: dict, key: str, limit: int, where: str, empty: bool = False) -> None:
    value = obj.get(key)
    require(isinstance(value, str) and len(value) <= limit and (empty or bool(value.strip())),
            f"{where}.{key} must be {'a' if empty else 'a nonempty'} string <= {limit} characters")


def number_field(obj: dict, key: str, low: float, high: float, where: str) -> None:
    value = obj.get(key)
    require(numeric(value) and low <= value <= high,
            f"{where}.{key} must be a finite number in [{low}, {high}]")


def bounded_tree(value: object, depth: int = 0) -> None:
    require(depth <= 12, "manifest nesting exceeds 12 levels")
    if isinstance(value, dict):
        require(len(value) <= 128, "object exceeds 128 properties")
        for key, item in value.items():
            require(isinstance(key, str) and len(key) <= 80, "invalid property name")
            bounded_tree(item, depth + 1)
    elif isinstance(value, list):
        require(len(value) <= 512, "array exceeds 512 items")
        for item in value:
            bounded_tree(item, depth + 1)
    elif isinstance(value, str):
        require(len(value) <= 4096, "string exceeds 4096 characters")
    elif isinstance(value, bool) or value is None:
        pass
    else:
        require(numeric(value), "unsupported value or nonfinite number")


def rectangle(obj: dict, where: str) -> None:
    for key, low, high in (("x", 0, 1586), ("y", 0, 992), ("w", 1, 1586), ("h", 1, 992)):
        number_field(obj, key, low, high, where)
    require(obj["x"] + obj["w"] <= 1586.001 and obj["y"] + obj["h"] <= 992.001,
            f"{where} extends outside 1586 x 992 canvas")


def validate_manifest(data: object) -> None:
    """Raise InvalidManifest on the first invalid field; do not mutate data."""
    bounded_tree(data)
    require(isinstance(data, dict), "manifest must be an object")
    require(numeric(data.get("schema")) and data["schema"] == 1, "schema must be 1")
    for key in ("id", "name", "reference", "style", "font"):
        text_field(data, key, 256, "manifest")
    require(bool(IDENTIFIER.fullmatch(data["id"])), "invalid manifest id")
    require(data["style"] in STYLES, "unknown workspace style")
    require(data["font"] in FONTS, "unknown workspace font")
    number_field(data, "width", 1586, 1586, "manifest")
    number_field(data, "height", 992, 992, "manifest")
    for key, low, high in (("headerHeight", 32, 120), ("radius", 0, 48), ("glow", 0, 32)):
        if key in data and not (key == "glow" and isinstance(data[key], bool)):
            number_field(data, key, low, high, "manifest")
    palette = data.get("palette")
    require(isinstance(palette, dict), "palette must be an object")
    for key in sorted(PALETTE):
        require(isinstance(palette.get(key), str) and bool(COLOUR.fullmatch(palette[key])),
                f"palette.{key} must be #RRGGBB or #RRGGBBAA")
    require(all(isinstance(value, str) and COLOUR.fullmatch(value) for value in palette.values()),
            "every palette entry must be a hex colour")

    ids: set[str] = set()
    for collection, maximum in (("panels", 80), ("controls", 256)):
        items = data.get(collection)
        require(isinstance(items, list) and 1 <= len(items) <= maximum,
                f"{collection} must contain 1 to {maximum} objects")
        for index, item in enumerate(items):
            where = f"{collection}[{index}]"
            require(isinstance(item, dict), f"{where} must be an object")
            text_field(item, "id", 80, where)
            require(bool(IDENTIFIER.fullmatch(item["id"])), f"{where}.id is invalid")
            require(item["id"] not in ids, f"{where}.id duplicates another panel/control id")
            ids.add(item["id"])
            rectangle(item, where)
            if collection == "panels":
                text_field(item, "title", 160, where, empty=True)
                text_field(item, "kind", 40, where)
                require(item["kind"] in PANELS, f"{where}.kind is unknown")
                if "variant" in item:
                    require(isinstance(item["variant"], str) and item["variant"] in VARIANTS,
                            f"{where}.variant is unknown")
                if "index" in item:
                    value = item["index"]
                    require(numeric(value) and 1 <= value <= 16 and int(value) == value,
                            f"{where}.index must be integer 1..16")
                for key in ("plot", "plot2"):
                    if key in item:
                        values = item[key]
                        require(isinstance(values, list) and len(values) == 4 and
                                all(numeric(value) for value in values),
                                f"{where}.{key} must be [x, y, width, height] with finite numbers")
                        rectangle(dict(zip(("x", "y", "w", "h"), values)), f"{where}.{key}")
                if "titleHeight" in item:
                    number_field(item, "titleHeight", 0, 120, where)
            else:
                text_field(item, "binding", 80, where)
                require(bool(IDENTIFIER.fullmatch(item["binding"])), f"{where}.binding is invalid")
                if "kind" in item:
                    require(isinstance(item["kind"], str) and item["kind"] in CONTROLS,
                            f"{where}.kind is unknown")
                if "label" in item:
                    text_field(item, "label", 160, where, empty=True)
                if "fontSize" in item:
                    number_field(item, "fontSize", 8, 24, where)
            colour_key = "color" if collection == "panels" else "accent"
            if colour_key in item:
                value = item[colour_key]
                require(isinstance(value, str) and (bool(COLOUR.fullmatch(value)) or value in palette),
                        f"{where}.{colour_key} must be a hex colour or palette key")
    if "decorations" in data:
        require(isinstance(data["decorations"], list) and len(data["decorations"]) <= 128 and
                all(isinstance(item, dict) for item in data["decorations"]),
                "decorations must be an array of at most 128 objects")
    adaptations = data.get("adaptations")
    require(isinstance(adaptations, list) and len(adaptations) <= 32 and
            all(isinstance(item, str) and len(item) <= 1024 for item in adaptations),
            "adaptations must contain at most 32 strings of <= 1024 characters")


def unique_object(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        require(key not in result, f"duplicate JSON property: {key}")
        result[key] = value
    return result


def load_manifest(path: Path) -> dict:
    # Read one extra byte rather than trusting a pre-read stat against file growth.
    with path.open("rb") as stream:
        raw = stream.read(MAX_BYTES + 1)
    require(0 < len(raw) <= MAX_BYTES, "manifest must be nonempty and <= one MiB")
    require(b"\x00" not in raw, "manifest contains a NUL byte")
    data = json.loads(raw.decode("utf-8"), object_pairs_hook=unique_object)
    validate_manifest(data)
    return data


def overlaps(controls: list[dict]) -> list[str]:
    result = []
    for index, a in enumerate(controls):
        for b in controls[index + 1:]:
            width = min(a["x"] + a["w"], b["x"] + b["w"]) - max(a["x"], b["x"])
            height = min(a["y"] + a["h"], b["y"] + b["h"]) - max(a["y"], b["y"])
            if width > 0.5 and height > 0.5:
                result.append(f"control bounds overlap: {a['id']} / {b['id']} ({width:g} x {height:g})")
    return result


def validate_packages(root: Path, allow_partial: bool, strict_overlap: bool) -> dict:
    results = []
    errors = []
    workspace_ids: set[str] = set()
    skin_names: set[str] = set()
    styles: set[str] = set()
    skins_root = root / "resources" / "data" / "skins"
    require(skins_root.is_dir(), f"skin directory does not exist: {skins_root}")
    for xml_path in sorted(skins_root.rglob("skin.xml")):
        try:
            with xml_path.open("rb") as stream:
                raw = stream.read(MAX_BYTES + 1)
            require(len(raw) <= MAX_BYTES, f"{xml_path}: skin XML exceeds one MiB")
            xml = ET.fromstring(raw)
            tags = xml.findall("./globals/reference-workspace")
            if not tags:
                continue
            require(len(tags) == 1, "skin must contain exactly one reference-workspace global")
            require(xml.tag == "surge-skin" and xml.get("version") == "2", "skin must use version 2")
            require(xml_path.parent.name.endswith(".surge-skin"), "package requires .surge-skin suffix")
            for section in ("globals", "component-classes", "controls"):
                require(xml.find(section) is not None, f"missing skin section: {section}")
            name = xml.get("name", "").strip()
            require(bool(name) and name not in skin_names, "skin name is empty or duplicated")
            skin_names.add(name)
            relative = tags[0].get("layout", "")
            relative_path = PurePosixPath(relative)
            require(bool(relative) and not relative_path.is_absolute() and
                    ".." not in relative_path.parts and "\\" not in relative and ":" not in relative,
                    "manifest layout must be a confined relative path")
            manifest_path = (xml_path.parent / relative).resolve(strict=True)
            require(manifest_path.is_relative_to(xml_path.parent.resolve()),
                    "manifest symlink escapes skin package")
            data = load_manifest(manifest_path)
            require(data["id"] not in workspace_ids, "duplicate workspace id")
            require(data["style"] not in styles, "duplicate concept style")
            workspace_ids.add(data["id"])
            styles.add(data["style"])
            warnings = overlaps(data["controls"])
            require(not strict_overlap or not warnings, "; ".join(warnings))
            results.append({"id": data["id"], "name": name, "style": data["style"],
                            "manifest": str(manifest_path.relative_to(root.resolve())),
                            "panels": len(data["panels"]), "controls": len(data["controls"]),
                            "warnings": warnings})
        except (InvalidManifest, OSError, ValueError, ET.ParseError, RecursionError) as error:
            errors.append(f"{xml_path.relative_to(root)}: {error}")
    if not allow_partial and len(results) != 10:
        errors.append(f"Expected all 10 reference workspaces; validated {len(results)}")
    return {"ok": not errors, "count": len(results), "workspaces": results, "errors": errors}


def self_test() -> None:
    base = {"schema": 1, "id": "test", "name": "Test", "reference": "01.png",
            "style": "precision", "font": "sans", "width": 1586, "height": 992,
            "palette": {key: "#112233" for key in PALETTE}, "adaptations": [],
            "panels": [{"id": "p", "kind": "oscillator", "title": "Oscillator", "x": 0,
                        "y": 80, "w": 400, "h": 200, "index": 1}],
            "controls": [{"id": "c", "binding": "osc1.pitch", "x": 300, "y": 120,
                          "w": 64, "h": 80}]}
    validate_manifest(base)
    bad_cases = []
    for path, value in [(('schema',), True), (('width',), "1586"), (('style',), "unknown"),
                        (('palette', 'accent'), "red"), (('controls', 0, 'w'), 0),
                        (('controls', 0, 'x'), float('nan')), (('controls', 0, 'y'), 999),
                        (('controls', 0, 'id'), "p"), (('panels', 0, 'index'), 1.5),
                        (('panels', 0, 'kind'), "webview"), (('adaptations',), [None]),
                        (('controls', 0, 'binding'), "../../secret"),
                        (('controls', 0, 'fontSize'), 7), (('panels', 0, 'plot'), [0, 0, 5000, 40]),
                        (('controls', 0, 'x'), 10**1000), (('palette', 'extra'), 3)]:
        case = copy.deepcopy(base)
        parent = case
        for key in path[:-1]:
            parent = parent[key]
        parent[path[-1]] = value
        bad_cases.append(case)
    bad_cases.append({**base, "controls": []})
    for index, case in enumerate(bad_cases):
        try:
            validate_manifest(case)
        except InvalidManifest:
            continue
        raise AssertionError(f"invalid case {index} was accepted")
    assert overlaps(base["controls"]) == []
    assert len(overlaps(base["controls"] * 2)) == 1
    print(f"PASS: valid fixture, {len(bad_cases)} rejected fixtures, overlap checks")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--allow-partial", action="store_true", help="allow fewer than 10 packages during development")
    parser.add_argument("--strict-overlap", action="store_true", help="treat control bounding-box overlaps as errors")
    parser.add_argument("--json", action="store_true", help="print machine-readable report")
    parser.add_argument("--self-test", action="store_true", help="run built-in schema rejection tests")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return 0
    try:
        report = validate_packages(args.root.resolve(), args.allow_partial, args.strict_overlap)
    except InvalidManifest as error:
        report = {"ok": False, "count": 0, "workspaces": [], "errors": [str(error)]}
    if args.json:
        print(json.dumps(report, indent=2))
    else:
        for item in report["workspaces"]:
            print(f"PASS {item['name']}: {item['panels']} panels, {item['controls']} controls")
            for warning in item["warnings"]:
                print(f"  WARNING {warning}")
        for error in report["errors"]:
            print(f"FAIL {error}", file=sys.stderr)
        print(f"{'PASS' if report['ok'] else 'FAIL'}: {report['count']} reference workspaces validated")
    return 0 if report["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
