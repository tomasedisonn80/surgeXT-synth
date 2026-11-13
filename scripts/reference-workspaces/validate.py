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
import tempfile
import xml.etree.ElementTree as ET
from pathlib import Path, PurePosixPath

MAX_BYTES = 1024 * 1024
MAX_EXACT_INTEGER = 9007199254740991
STYLES = set("precision ivory spectral bauhaus space tokyo aurora clear flow live".split())
FONTS = {"sans", "serif", "mono"}
PALETTE = set("background panel border text muted accent secondary display knob red yellow blue".split())
PANELS = set("oscillator filter mixer envelope lfo macros matrix effects analyzer waveform browser routing scene output xy keyboard inspector amp tabs".split())
VARIANTS = set("compact horizontal vertical large combined terrain flow faders knobs scope spectrum spectrogram".split())
CONTROLS = {"knob", "fader", "horizontal", "menu", "toggle", "value"}
FLOW_NODES = set("osc1 osc2 osc3 mixer filter1 filter2 amp fx output modsources modbus".split())
SELECTION_SOURCES = {"envelope": 2, "lfo": 12, "lfo-envelope": 12,
                     "oscillator": 3, "waveform": 3, "fx": 16}
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
    if isinstance(value, int) and abs(value) > MAX_EXACT_INTEGER:
        return False
    try:
        return math.isfinite(value)
    except OverflowError:
        return False


def unicode_text(value: str) -> bool:
    return all(char != "\x00" and not 0xD800 <= ord(char) <= 0xDFFF for char in value)


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
            require(isinstance(key, str) and 0 < len(key) <= 80 and unicode_text(key),
                    "invalid property name")
            bounded_tree(item, depth + 1)
    elif isinstance(value, list):
        require(len(value) <= 512, "array exceeds 512 items")
        for item in value:
            bounded_tree(item, depth + 1)
    elif isinstance(value, str):
        require(len(value) <= 4096 and unicode_text(value),
                "string must be valid non-NUL Unicode <= 4096 characters")
    elif isinstance(value, bool) or value is None:
        pass
    else:
        require(numeric(value), "unsupported value or nonfinite number")


def rectangle(obj: dict, where: str) -> None:
    for key, low, high in (("x", 0, 1586), ("y", 0, 992), ("w", 1, 1586), ("h", 1, 992)):
        number_field(obj, key, low, high, where)
    require(obj["x"] + obj["w"] <= 1586.001 and obj["y"] + obj["h"] <= 992.001,
            f"{where} extends outside 1586 x 992 canvas")


def array_rectangle(values: object, where: str, panel: dict | None = None) -> None:
    require(isinstance(values, list) and len(values) == 4 and all(numeric(v) for v in values),
            f"{where} must be [x, y, width, height] with finite numbers")
    rectangle(dict(zip(("x", "y", "w", "h"), values)), where)
    if panel is not None:
        x, y, width, height = values
        require(x >= panel["x"] - .001 and y >= panel["y"] - .001 and
                x + width <= panel["x"] + panel["w"] + .001 and
                y + height <= panel["y"] + panel["h"] + .001,
                f"{where} must be inside its panel")


def selection_grid(selection: dict) -> tuple[int, int, float, float]:
    """Match Workspace::buildViewTabs after schema validation, including 4px gaps."""
    count = len(selection["choices"])
    columns = min(count, int(selection.get("columns", count)))
    rows = (count + columns - 1) // columns
    width, height = selection["tabsRect"][2:]
    return columns, rows, (width - 4 * (columns - 1)) / columns, (height - 4 * (rows - 1)) / rows


def validate_selection(selection: object, panel: dict, where: str,
                       selections: dict[str, dict]) -> None:
    require(isinstance(selection, dict), f"{where} must be an object")
    text_field(selection, "id", 80, where)
    require(bool(IDENTIFIER.fullmatch(selection["id"])), f"{where}.id is invalid")
    require(selection["id"] not in selections, f"{where}.id duplicates another selection group")
    require(isinstance(selection.get("kind"), str) and selection["kind"] in ("source", "fx"),
            f"{where}.kind must be source or fx")
    array_rectangle(selection.get("tabsRect"), f"{where}.tabsRect", panel)
    choices = selection.get("choices")
    require(isinstance(choices, list) and 1 <= len(choices) <= 16,
            f"{where}.choices must contain 1..16 objects")
    choice_ids: set[str] = set()
    for index, choice in enumerate(choices):
        choice_where = f"{where}.choices[{index}]"
        require(isinstance(choice, dict), f"{choice_where} must be an object")
        text_field(choice, "id", 80, choice_where)
        require(bool(IDENTIFIER.fullmatch(choice["id"])), f"{choice_where}.id is invalid")
        require(choice["id"] not in choice_ids, f"{choice_where}.id duplicates another choice")
        choice_ids.add(choice["id"])
        text_field(choice, "label", 160, choice_where)
        text_field(choice, "source", 40, choice_where)
        maximum = SELECTION_SOURCES.get(choice["source"], 0)
        value = choice.get("index")
        require(maximum > 0 and numeric(value) and 1 <= value <= maximum and int(value) == value,
                f"{choice_where}.source/index is not a supported synth source")
        require((selection["kind"] == "fx") == (choice["source"] == "fx"),
                f"{choice_where}.source does not match the selection kind")
    text_field(selection, "default", 80, where)
    require(selection["default"] in choice_ids, f"{where}.default must name a choice")
    if "columns" in selection:
        value = selection["columns"]
        require(numeric(value) and 1 <= value <= 8 and int(value) == value,
                f"{where}.columns must be an integer in 1..8")
    _, _, width, height = selection_grid(selection)
    require(width >= 1 and height >= 1,
            f"{where}.tabsRect is too small for its tab buttons and gaps")
    if "fontSize" in selection:
        number_field(selection, "fontSize", 8, 24, where)
    selections[selection["id"]] = selection


def validate_selection_bindings(control: dict, where: str, selections: dict[str, dict]) -> None:
    text_field(control, "selectionGroup", 80, where)
    group = control["selectionGroup"]
    require(bool(IDENTIFIER.fullmatch(group)), f"{where}.selectionGroup is invalid")
    require(group in selections, f"{where}.selectionGroup names an unknown selection")
    selection = selections[group]
    bindings = control.get("bindings")
    require(isinstance(bindings, dict) and
            set(bindings) == {choice["id"] for choice in selection["choices"]},
            f"{where}.bindings must contain exactly the selection choice ids")
    for choice, binding in bindings.items():
        require(binding is None or isinstance(binding, str) and bool(IDENTIFIER.fullmatch(binding)),
                f"{where}.bindings.{choice} must be a parameter identifier or null")
    require(bindings[selection["default"]] == control["binding"],
            f"{where}.bindings default must equal its base binding and not be null")


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
    selections: dict[str, dict] = {}
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
                        array_rectangle(item[key], f"{where}.{key}", item)
                if "wavetableRect" in item:
                    require(item["kind"] == "oscillator" and item.get("index", 1) <= 3,
                            f"{where}.wavetableRect requires oscillator index 1..3")
                    array_rectangle(item["wavetableRect"], f"{where}.wavetableRect", item)
                    require(item["wavetableRect"][2] >= 60 and item["wavetableRect"][3] >= 18,
                            f"{where}.wavetableRect must be at least 60 x 18")
                if "flowNodes" in item:
                    nodes = item["flowNodes"]
                    require(isinstance(nodes, dict) and 1 <= len(nodes) <= 11 and
                            item["kind"] == "routing",
                            f"{where}.flowNodes requires 1..11 routing node rectangles")
                    for key, values in nodes.items():
                        require(key in FLOW_NODES, f"{where}.flowNodes.{key} is not a supported flow node")
                        array_rectangle(values, f"{where}.flowNodes.{key}", item)
                if "titleHeight" in item:
                    number_field(item, "titleHeight", 0, min(120, item["h"]), where)
                if "titleInset" in item:
                    number_field(item, "titleInset", 0, item["w"] - 12, where)
                if "envelope" in item:
                    require(isinstance(item["envelope"], bool) and item["kind"] == "lfo",
                            f"{where}.envelope must be a boolean on an lfo panel")
                if "selection" in item:
                    validate_selection(item["selection"], item, f"{where}.selection", selections)
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
                if "labelPosition" in item:
                    require(isinstance(item["labelPosition"], str) and
                            item["labelPosition"] in ("left", "right", "top") and
                            item.get("kind", "knob") == "knob",
                            f"{where}.labelPosition must be left/right/top on a knob")
                kind = item.get("kind", "knob")
                if "faceDiameter" in item:
                    require(kind == "knob", f"{where}.faceDiameter is only supported on a knob")
                    number_field(item, "faceDiameter", 8, min(160, item["w"], item["h"]), where)
                if "valuePosition" in item:
                    require(kind == "knob" and isinstance(item["valuePosition"], str) and
                            item["valuePosition"] in ("bottom", "right", "hidden", "top"),
                            f"{where}.valuePosition must be bottom/right/hidden/top on a knob")
                    require(item["valuePosition"] != "top" or item.get("labelPosition") == "top",
                            f"{where}.valuePosition top requires labelPosition top")
                if "labelWidth" in item:
                    require(kind == "value" or (kind == "horizontal" and item.get("compact") is True),
                            f"{where}.labelWidth requires a value or compact slider")
                    number_field(item, "labelWidth", 0,
                                 item["w"] - (52 if item.get("compact") is True else 12), where)
                if "compact" in item:
                    require(kind == "horizontal" and isinstance(item["compact"], bool),
                            f"{where}.compact must be boolean on a horizontal slider")
                if "glyph" in item:
                    require(kind == "toggle" and item["glyph"] == "power",
                            f"{where}.glyph must be power on a toggle")
                if "selectionGroup" in item or "bindings" in item:
                    validate_selection_bindings(item, where, selections)
                minimum = {"fader": (24, 46), "horizontal": (24, 40)}.get(item.get("kind"))
                if kind == "horizontal" and item.get("compact") is True:
                    minimum = (100, 20)
                if minimum:
                    require(item["w"] >= minimum[0] and item["h"] >= minimum[1],
                            f"{where} is too small for its labelled slider and track")
            if "ink" in item:
                require(isinstance(item["ink"], str) and bool(COLOUR.fullmatch(item["ink"])),
                        f"{where}.ink must be #RRGGBB or #RRGGBBAA")
            if collection == "panels" and "fontSize" in item:
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


def bounded_json(text: str) -> None:
    """Cap container depth before Python's recursive JSON parser is called."""
    require(text.lstrip(" \t\r\n").startswith("{"), "manifest must be a JSON object")
    stack: list[str] = []
    quoted = escaped = ended = False
    for char in text:
        if quoted:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == '"':
                quoted = False
            continue
        if char in " \t\r\n":
            continue
        require(not ended, "unexpected text after manifest JSON object")
        if char == '"':
            quoted = True
        elif char in "{[":
            stack.append(char)
            # Root is depth zero. Scalar depth is checked by bounded_tree.
            require(len(stack) <= 13, "manifest nesting exceeds 12 levels")
        elif char in "}]":
            require(bool(stack) and stack[-1] == {"}": "{", "]": "["}[char],
                    "unbalanced manifest JSON delimiters")
            stack.pop()
            ended = not stack
    require(ended and not stack and not quoted, "incomplete manifest JSON object")


def number_token(token: str, integral: bool) -> int | float:
    require(len(token) <= 128, "JSON number exceeds its 128-character limit")
    parts = re.split("[eE]", token.lstrip("-"))
    if len(parts) == 2:
        require(len(parts[1].lstrip("+-")) <= 4, "JSON exponent exceeds its four-digit limit")
    whole = parts[0].split(".", 1)[0]
    require(len(whole) < 19 or len(whole) == 19 and whole <= "9223372036854775807",
            "JSON number's integer part exceeds the parser's safe range")
    value = int(token) if integral else float(token)
    require(numeric(value), "JSON number is nonfinite or outside the exact integer range")
    return value


def load_bytes(raw: bytes) -> dict:
    require(0 < len(raw) <= MAX_BYTES, "manifest must be nonempty and <= one MiB")
    require(b"\x00" not in raw, "manifest contains a NUL byte")
    text = raw.decode("utf-8")
    bounded_json(text)
    data = json.loads(text, object_pairs_hook=unique_object,
                      parse_int=lambda token: number_token(token, True),
                      parse_float=lambda token: number_token(token, False))
    validate_manifest(data)
    return data


def load_manifest(path: Path) -> dict:
    # Read one extra byte rather than trusting a pre-read stat against file growth.
    with path.open("rb") as stream:
        return load_bytes(stream.read(MAX_BYTES + 1))


def resolve_layout(package: Path, relative: str) -> Path:
    relative_path = PurePosixPath(relative)
    require(bool(relative) and not relative_path.is_absolute() and
            ".." not in relative_path.parts and "\\" not in relative and
            ":" not in relative and "\x00" not in relative,
            "manifest layout must be a confined relative path")
    manifest_path = (package / relative).resolve(strict=True)
    require(manifest_path.is_relative_to(package.resolve()), "manifest symlink escapes skin package")
    require(manifest_path.is_file(), "manifest layout must name a file")
    return manifest_path


def intersection(a: dict, b: dict) -> tuple[float, float]:
    return (min(a["x"] + a["w"], b["x"] + b["w"]) - max(a["x"], b["x"]),
            min(a["y"] + a["h"], b["y"] + b["h"]) - max(a["y"], b["y"]))


def overlaps(controls: list[dict]) -> list[str]:
    result = []
    for index, a in enumerate(controls):
        for b in controls[index + 1:]:
            width, height = intersection(a, b)
            if width > 0.5 and height > 0.5:
                result.append(f"control bounds overlap: {a['id']} / {b['id']} ({width:g} x {height:g})")
    return result


def interactive_bounds(data: dict) -> list[dict]:
    """Declared controls, wavetable selectors and individual generated tab cells."""
    result = list(data["controls"])
    for panel in data["panels"]:
        if "wavetableRect" in panel:
            result.append({"id": f"workspace.wavetable.{panel['id']}",
                           **dict(zip(("x", "y", "w", "h"), panel["wavetableRect"]))})
        selection = panel.get("selection")
        if selection is None:
            continue
        columns, _, width, height = selection_grid(selection)
        x, y = selection["tabsRect"][:2]
        for index, choice in enumerate(selection["choices"]):
            result.append({"id": f"workspace.selection.{selection['id']}.{choice['id']}",
                           "x": x + (index % columns) * (width + 4),
                           "y": y + (index // columns) * (height + 4),
                           "w": width, "h": height})
    return result


def visual_overlaps(data: dict) -> tuple[list[str], list[str]]:
    """Diagnostic only: a route graph intentionally contains live node controls.

    A panel is a container, so panel/control intersections are not collisions.
    Unexpected plot/control or plot/plot intersections are reported separately
    from the fatal control hit-target test; labels may need visual inspection.
    """
    warnings: list[str] = []
    intentional: list[str] = []
    plots = []
    for panel in data["panels"]:
        route = panel["kind"] == "routing" and ("flowNodes" in panel or panel.get("variant") == "flow")
        route_count = 0
        for key in ("plot", "plot2"):
            if key not in panel:
                continue
            plot = dict(zip(("x", "y", "w", "h"), panel[key]))
            plot["id"] = f"{panel['id']}.{key}"
            plots.append(plot)
            for control in data["controls"]:
                width, height = intersection(plot, control)
                if width <= .5 or height <= .5:
                    continue
                if route:
                    route_count += 1
                else:
                    warnings.append(f"plot/control bounds overlap: {plot['id']} / {control['id']} "
                                    f"({width:g} x {height:g}); inspect rendered labels")
        if route_count:
            intentional.append(f"{panel['id']}: {route_count} live controls intentionally overlay routing plot")
    for index, a in enumerate(plots):
        for b in plots[index + 1:]:
            width, height = intersection(a, b)
            if width > .5 and height > .5:
                warnings.append(f"plot bounds overlap: {a['id']} / {b['id']} ({width:g} x {height:g})")
    return warnings, intentional


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
            manifest_path = resolve_layout(xml_path.parent, relative)
            data = load_manifest(manifest_path)
            require(data["id"] not in workspace_ids, "duplicate workspace id")
            require(data["style"] not in styles, "duplicate concept style")
            workspace_ids.add(data["id"])
            styles.add(data["style"])
            warnings = overlaps(interactive_bounds(data))
            require(not strict_overlap or not warnings, "; ".join(warnings))
            visual_warnings, intentional = visual_overlaps(data)
            warnings.extend(visual_warnings)
            results.append({"id": data["id"], "name": name, "style": data["style"],
                            "manifest": str(manifest_path.relative_to(root.resolve())),
                            "panels": len(data["panels"]), "controls": len(data["controls"]),
                            "warnings": warnings, "intentional_overlays": intentional})
        except (InvalidManifest, OSError, ValueError, ET.ParseError, RecursionError) as error:
            errors.append(f"{xml_path.relative_to(root)}: {error}")
    if not allow_partial and len(results) != 10:
        errors.append(f"Expected all 10 reference workspaces; validated {len(results)}")
    return {"ok": not errors, "count": len(results), "workspaces": results, "errors": errors}


def shared_schema_self_test() -> tuple[int, int]:
    """The native Catch test reads this same data, rather than duplicating cases."""
    path = Path(__file__).resolve().parent / "fixtures" / "selection-schema.json"
    fixtures = json.loads(path.read_text(encoding="utf-8"))
    accepted = rejected = 0
    for fixture in fixtures["cases"]:
        case = copy.deepcopy(fixtures["base"])
        for change in fixture["changes"]:
            parent = case
            for key in change["path"][:-1]:
                parent = parent[key]
            key = change["path"][-1]
            if change.get("remove", False):
                del parent[key]
            else:
                parent[key] = copy.deepcopy(change["value"])
        try:
            validate_manifest(case)
        except InvalidManifest as error:
            assert not fixture["valid"], f"valid shared fixture {fixture['name']!r} failed: {error}"
            rejected += 1
        else:
            assert fixture["valid"], f"invalid shared fixture {fixture['name']!r} was accepted"
            accepted += 1

    base = fixtures["base"]
    bounds = interactive_bounds(base)
    assert len(bounds) == len(base["controls"]) + 2
    assert bounds[-2] == {"id": "workspace.selection.source.env1", "x": 10, "y": 85,
                          "w": 188, "h": 30}
    assert bounds[-1]["x"] == 202
    assert not overlaps(bounds)
    collision = copy.deepcopy(base)
    collision["controls"].append({"id": "collision", "binding": "osc1.pitch", "x": 20,
                                  "y": 90, "w": 16, "h": 16})
    validate_manifest(collision)
    assert any("workspace.selection.source.env1" in warning
               for warning in overlaps(interactive_bounds(collision)))

    # Three buttons in two columns leave the bottom-right cell free. The parent
    # tabsRect would incorrectly reject this otherwise valid live control.
    grid = copy.deepcopy(base)
    selection = grid["panels"][0]["selection"]
    selection["tabsRect"] = [10, 85, 200, 100]
    selection["choices"].append({"id": "env2", "label": "ENV 2", "source": "envelope", "index": 2})
    grid["controls"] = [{"id": "free", "binding": "osc1.pitch", "x": 125, "y": 150,
                         "w": 20, "h": 20}]
    validate_manifest(grid)
    assert not overlaps(interactive_bounds(grid))
    grid["controls"][0].update(x=108, y=90, w=4, h=20)  # The 4px gap is also free.
    validate_manifest(grid)
    assert not overlaps(interactive_bounds(grid))

    wavetable = copy.deepcopy(base)
    wavetable["panels"][0].update(kind="oscillator", wavetableRect=[10, 210, 180, 24])
    del wavetable["panels"][0]["envelope"]
    validate_manifest(wavetable)
    assert not overlaps(interactive_bounds(wavetable))
    wavetable["controls"][1].update(x=10, y=210)
    assert any("workspace.wavetable.sources" in warning
               for warning in overlaps(interactive_bounds(wavetable)))
    wavetable["panels"][0]["wavetableRect"] = [10, 85, 180, 24]
    assert any("workspace.wavetable.sources" in warning and "workspace.selection.source.env1" in warning
               for warning in overlaps(interactive_bounds(wavetable)))
    return accepted, rejected


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
                        (('controls', 0, 'x'), 10**1000), (('palette', 'extra'), 3),
                        (('controls', 0, 'labelPosition'), "bottom"),
                        (('controls', 0, 'labelPosition'), None),
                        (('controls', 0, 'fontSize'), float('inf')),
                        (('controls', 0, 'ink'), "text"),
                        (('panels', 0, 'ink'), "#12345Z"),
                        (('panels', 0, 'fontSize'), 25),
                        (('panels', 0, 'plot'), [0, 0, 100, 20]),
                        (('panels', 0, 'plot2'), [0, 90, 100, -20]),
                        (('panels', 0, 'flowNodes'), {}),
                        (('name',), "unpaired\ud800"), (('name',), "NUL\x00text")]:
        case = copy.deepcopy(base)
        parent = case
        for key in path[:-1]:
            parent = parent[key]
        parent[path[-1]] = value
        bad_cases.append(case)
    bad_cases.append({**base, "controls": []})
    for kind, width, height in (("fader", 23, 80), ("fader", 24, 45),
                                ("horizontal", 24, 39), ("horizontal", 23, 60)):
        case = copy.deepcopy(base)
        case["controls"][0].update(kind=kind, w=width, h=height)
        bad_cases.append(case)
    route = copy.deepcopy(base)
    route["panels"][0].update(kind="routing", flowNodes={"osc1": [10, 90, 100, 60]},
                              plot=[5, 85, 390, 190])
    validate_manifest(route)
    for nodes in ([], {}, {"remote": [10, 90, 100, 60]}, {"osc1": [10, 90, 0, 60]},
                  {"osc1": [10, 90, 100, float("nan")]}, {"osc1": [10, 90, 100]},
                  {"osc1": [500, 90, 100, 60]}, {"osc1": "https://invalid.example/image"}):
        case = copy.deepcopy(route)
        case["panels"][0]["flowNodes"] = nodes
        bad_cases.append(case)
    case = copy.deepcopy(base)
    case["controls"][0].update(kind="fader", labelPosition="left")
    bad_cases.append(case)
    case = copy.deepcopy(base)
    nested = case
    for _ in range(14):
        nested["extension"] = {}
        nested = nested["extension"]
    bad_cases.append(case)
    for index, case in enumerate(bad_cases):
        try:
            validate_manifest(case)
        except InvalidManifest:
            continue
        raise AssertionError(f"invalid case {index} was accepted")
    duplicates = copy.deepcopy(base)
    duplicates["controls"].append({**duplicates["controls"][0], "id": "second", "x": 400})
    validate_manifest(duplicates)  # A parameter may intentionally have two views.
    unicode_case = copy.deepcopy(base)
    unicode_case["name"] = "Café 日本語 فارسی 🟩"
    for ascii_only in (True, False):
        raw = json.dumps(unicode_case, ensure_ascii=ascii_only).encode("utf-8")
        assert load_bytes(raw)["name"] == unicode_case["name"]
    raw = json.dumps(base).encode("utf-8")
    malformed = [b"", b"x" * (MAX_BYTES + 1), raw + b"{}", raw + b"\x00", b"\xef\xbb\xbf" + raw,
                 raw.replace(b'"Test"', b'"bad\\q"'), raw.replace(b'"Test"', b'"bad\\a"'),
                 raw.replace(b'"Test"', b'"bad\\u0000"'), raw.replace(b'"Test"', b'"bad\\ud800"'),
                 raw.replace(b'"Test"', b'"bad\\udc00"'), raw.replace(b'"Test"', b'"bad\xff"'),
                 raw.replace(b'"Test"', b'"bad\ntext"'), raw[:-1] + b",}",
                 raw.replace(b'"schema": 1', b'"schema": 1, "\\u0073chema": 1'),
                 raw.replace(b'"schema": 1', b'"schema": 18446744073709551617'),
                 raw[:-1] + b', "extra": 999999999999999999999.0}',
                 raw[:-1] + b', "extra": 1e999999999999}',
                 raw[:-1] + b', "extra": 1e309}', raw[:-1] + b', "extra": 01}',
                 raw[:-1] + b', "extra": 1.}', raw[:-1] + b', "extra": +1}',
                 raw[:-1] + b', "extra": 9007199254740992}',
                 raw[:-1] + b', "extra": "unterminated}',
                 raw[:-1] + b', "extra": ' + b'[' * 20 + b'0' + b']' * 20 + b'}']
    for index, payload in enumerate(malformed):
        try:
            load_bytes(payload)
        except (InvalidManifest, ValueError, UnicodeError):
            continue
        raise AssertionError(f"malformed JSON fixture {index} was accepted")
    paths_checked = 0
    with tempfile.TemporaryDirectory(prefix="surge-manifest-selftest-") as directory:
        root = Path(directory)
        package = root / "test.surge-skin"
        package.mkdir()
        (package / "workspace.json").write_bytes(raw)
        (root / "outside.json").write_bytes(raw)
        assert load_manifest(resolve_layout(package, "workspace.json"))["id"] == "test"
        for relative in ("", "../outside.json", "/outside.json", "C:/outside.json",
                         "https://invalid.example/layout", "..\\outside.json", ".", "\x00"):
            try:
                resolve_layout(package, relative)
            except (InvalidManifest, OSError, ValueError):
                paths_checked += 1
                continue
            raise AssertionError(f"unconfined layout path {relative!r} was accepted")
        (package / "escape.json").symlink_to(root / "outside.json")
        try:
            resolve_layout(package, "escape.json")
        except InvalidManifest:
            paths_checked += 1
        else:
            raise AssertionError("escaping manifest symlink was accepted")
    assert overlaps(base["controls"]) == []
    assert len(overlaps(base["controls"] * 2)) == 1
    assert len(visual_overlaps(route)[1]) == 1
    assert not visual_overlaps(route)[0]
    unexpected = copy.deepcopy(route)
    unexpected["panels"][0]["kind"] = "waveform"
    assert len(visual_overlaps(unexpected)[0]) == 1
    accepted, rejected = shared_schema_self_test()
    print(f"PASS: valid/Unicode/duplicate-binding fixtures; {len(bad_cases)} schema rejections; "
          f"{len(malformed)} malformed JSON rejections; {paths_checked} confined-path checks; "
          f"{accepted} accepted/{rejected} rejected shared selection/widget schema cases; "
          "declared-control, individual-tab, gap/unused-cell and wavetable overlap checks")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--allow-partial", action="store_true", help="allow fewer than 10 packages during development")
    parser.add_argument("--strict-overlap", action="store_true",
                        help="treat control, wavetable-selector and generated-tab overlaps as errors")
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
            for info in item["intentional_overlays"]:
                print(f"  NOTE {info}")
        for error in report["errors"]:
            print(f"FAIL {error}", file=sys.stderr)
        print(f"{'PASS' if report['ok'] else 'FAIL'}: {report['count']} reference workspaces validated")
    return 0 if report["ok"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
