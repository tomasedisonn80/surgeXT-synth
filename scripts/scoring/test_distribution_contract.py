#!/usr/bin/env python3
"""Check emitted packaging commands and required Inno file selections.

This configures tiny CMake harnesses with the real packaging functions. It does
not compile plugins, copy factory resources, or claim to inspect built installers.
The Inno check resolves every bank/skin input against the checked-in file rules,
including exclusions and selected installation components.
"""

from __future__ import annotations

import argparse
import fnmatch
import json
from pathlib import Path, PureWindowsPath
import re
import shutil
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


def inno_sections(text: str) -> dict[str, list[dict[str, str]]]:
    sections: dict[str, list[dict[str, str]]] = {}
    current = ""
    for raw in text.splitlines():
        line = raw.strip()
        if not line or line.startswith((";", "#")):
            continue
        if line.startswith("[") and line.endswith("]"):
            current = line[1:-1]
            sections.setdefault(current, [])
        elif current in ("Components", "Files"):
            fields = re.split(r";\s*(?=[A-Za-z][A-Za-z0-9]*\s*:)", line)
            entry = {}
            for field in fields:
                key, value = field.split(":", 1)
                entry[key.strip()] = value.strip().strip('"')
            sections[current].append(entry)
    return sections


def matches_inno_rule(path: Path, rule: dict[str, str], root: Path) -> str | None:
    source = rule["Source"]
    source_marker = "{#SURGE_SRC}\\"
    if not source.startswith(source_marker):
        return None
    pattern = PureWindowsPath(source[len(source_marker):])
    parent = root.joinpath(*pattern.parent.parts)
    try:
        relative = path.relative_to(parent)
    except ValueError:
        return None
    if len(relative.parts) > 1 and "recursesubdirs" not in rule.get("Flags", "").split():
        return None
    if not fnmatch.fnmatchcase(relative.name.lower(), pattern.name.lower()):
        return None
    relative_windows = str(PureWindowsPath(*relative.parts))
    for exclusion in rule.get("Excludes", "").split(","):
        exclusion = exclusion.strip().lower()
        if not exclusion:
            continue
        # Inno's leading backslash anchors at the source-tree root. Otherwise
        # a pattern can match a suffix starting at any path component.
        candidates = ["\\" + relative_windows] if exclusion.startswith("\\") else [
            str(PureWindowsPath(*relative.parts[index:]))
            for index in range(len(relative.parts))
        ]
        if any(fnmatch.fnmatchcase(candidate.lower(), exclusion) for candidate in candidates):
            return None
    return rule["DestDir"].rstrip("\\") + "\\" + relative_windows


def check_windows_components(root: Path) -> dict[str, object]:
    sections = inno_sections((root / "scripts/installer_win/surge64.iss").read_text())
    components = {entry["Name"]: entry for entry in sections["Components"]}
    data = components["Data"]
    require("fixed" in data["Flags"].split(), "Data must remain a fixed component")
    data_root = root / "resources/data"
    bank = sorted((data_root / "patches_factory/Scoring 1000").rglob("*.fxp"))
    skins = sorted((data_root / "skins").glob("reference-*.surge-skin"))
    require(len(bank) == 1000, f"Expected 1000 scoring presets, found {len(bank)}")
    require(len(skins) == 10, f"Expected 10 reference skins, found {len(skins)}")
    required = bank + sorted(path for skin in skins for path in skin.rglob("*") if path.is_file())
    types = ("full", "compact", "custom", "minimal")
    for kind in types:
        selected = {name for name, entry in components.items() if kind in entry["Types"].split()}
        require("Data" in selected, f"{kind} does not select Data")
        for path in required:
            destinations = []
            for rule in sections["Files"]:
                component = rule.get("Components", "")
                # These source rules use single component names. Reject a new
                # expression until the checker can interpret it accurately.
                require(not component or component in components,
                        f"Unsupported component expression: {component}")
                if component and component not in selected:
                    continue
                destination = matches_inno_rule(path, rule, root)
                if destination is not None:
                    destinations.append(destination)
            expected = "{commonappdata}\\{#MyAppName}\\" + str(
                PureWindowsPath(*path.relative_to(data_root).parts))
            require(destinations == [expected],
                    f"{kind}: {path.relative_to(root)} has destinations {destinations}, expected {expected}")
    return {"install_types": list(types), "presets": len(bank), "skin_packages": len(skins),
            "required_files_per_type": len(required), "duplicate_required_files": 0}


def trace_packaging(root: Path, platform: str, arch: str = "", extra: str = "",
                    cmake_command: str = "cmake") -> list[dict]:
    cmake = shutil.which(cmake_command)
    require(cmake is not None, "CMake is required to evaluate the packaging functions")
    with tempfile.TemporaryDirectory(prefix="surge-packaging-contract-") as temporary:
        harness = Path(temporary)
        config = [
            "cmake_minimum_required(VERSION 3.22)",
            "project(PackagingContract LANGUAGES NONE)",
            f'set(CMAKE_SOURCE_DIR [=[{root.as_posix()}]=])',
            f'set(WIN32 {"TRUE" if platform == "windows" else "FALSE"})',
            f'set(APPLE {"TRUE" if platform == "macos" else "FALSE"})',
            f'set(UNIX {"FALSE" if platform == "windows" else "TRUE"})',
            f'set(CMAKE_GENERATOR_PLATFORM "{arch}")',
            f'set(SURGE_EXTRA_ZIP_NAME "{extra}")',
            "set(SURGE_BITNESS 64)",
            'set(SURGE_VERSION "CONTRACT")',
            "add_custom_target(check-scoring-bank)",
            "add_executable(innosetup::compiler IMPORTED GLOBAL)",
            f'set_property(TARGET innosetup::compiler PROPERTY IMPORTED_LOCATION [=[{Path(cmake).as_posix()}]=])',
        ]
        if platform == "linux" and arch == "arm64":
            config.append("set(SURGE_LINUX_ARM TRUE)")
        config.extend([
            f'include([=[{(root / "src/cmake/lib.cmake").as_posix()}]=])',
            "surge_make_installers()",
        ])
        (harness / "CMakeLists.txt").write_text("\n".join(config) + "\n")
        result = subprocess.run(
            [cmake, "-S", str(harness), "-B", str(harness / "build"),
             "--trace-expand", "--trace-format=json-v1",
             f"--trace-source={root / 'src/cmake/lib.cmake'}"],
            capture_output=True, text=True, check=False,
        )
        require(result.returncode == 0, f"{platform}/{arch}/{extra}: {result.stderr[-5000:]}")
        return [json.loads(line) for line in result.stderr.splitlines() if line.startswith('{"')]


def check_packaging_routes(root: Path, cmake_command: str = "cmake") -> list[dict[str, object]]:
    results = []
    cases = [("windows", arch, extra) for arch, extra in
             (("x64", ""), ("arm64", ""), ("arm64ec", ""), ("ARM64", ""),
              ("ARM64EC", ""), ("x64", "-juce7"))]
    cases += [("linux", "x64", ""), ("linux", "arm64", ""), ("macos", "universal", "")]
    for platform, arch, extra in cases:
        trace = trace_packaging(root, platform, arch, extra, cmake_command)
        args = [event["args"] for event in trace if event.get("cmd") == "add_custom_command"]
        flat = [argument for command in args for argument in command]
        copies = [command[index + 1:index + 3] for command in args
                  for index, value in enumerate(command) if value == "copy_directory"]
        if platform in ("windows", "linux"):
            require(any(pair[0] == (root / "resources/data").as_posix() and pair[1].endswith("/SurgeXTData")
                        for pair in copies), f"{platform}/{arch}/{extra}: full portable data is absent")
            suffix = f"-{arch.lower()}" if arch.lower() in ("arm64", "arm64ec") else ""
            expected = (f"surge-xt-win64{extra}{suffix}-CONTRACT-portable-install.zip"
                        if platform == "windows" else
                        f"surge-xt-portable-content{suffix}-CONTRACT.tar.gz")
            require(sum(Path(argument).name == expected for argument in flat) == 1,
                    f"{platform}/{arch}/{extra}: expected one {expected}")
        else:
            require(any(event.get("cmd") == "run_installer_script" and
                        event["args"] == ["installer_mac/make_installer.sh"] for event in trace),
                    "macOS complete resource-bearing installer route is missing")
        if platform == "windows":
            native_installer = "innosetup::compiler" in flat
            require(native_installer == (arch.lower() == "x64" and not extra),
                    f"{arch}/{extra}: unexpected Windows native installer selection")
        results.append({"platform": platform, "architecture": arch, "variant": extra or "default",
                        "packaging_commands_valid": True})
    return results


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--cmake", default="cmake", help="CMake executable path or command")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    root = args.root.resolve()
    report = {
        "scope": "CMake command generation and Inno input selection; no native installer execution",
        "windows_required_content": check_windows_components(root),
        "packaging_routes": check_packaging_routes(root, args.cmake),
    }
    rendered = json.dumps(report, indent=2) + "\n"
    if args.output:
        args.output.write_text(rendered)
    print(rendered, end="")


if __name__ == "__main__":
    main()
