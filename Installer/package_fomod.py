#!/usr/bin/env python3
"""Create the Per Limb Healing FOMOD directly from the repository tree."""

from __future__ import annotations

import argparse
from pathlib import Path
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
FOMOD = ROOT / "Installer" / "fomod"
FIXED_TIME = (1980, 1, 1, 0, 0, 0)


def package_entries() -> list[tuple[Path, str]]:
    entries = [
        (ROOT / "Plugin" / "Built" / "PerLimbHealing.dll", "Core/F4SE/Plugins/PerLimbHealing.dll"),
        (ROOT / "MCM" / "Config" / "PerLimbHealing" / "config.json", "Core/MCM/Config/PerLimbHealing/config.json"),
        (ROOT / "MCM" / "Config" / "PerLimbHealing" / "settings.ini", "Core/MCM/Config/PerLimbHealing/settings.ini"),
        (ROOT / "SWF - Fallout 4" / "Built" / "Pipboy_StatsPage.swf", "Layouts/Fallout4/Interface/Pipboy_StatsPage.swf"),
        (ROOT / "SWF - Fallout 3-NV" / "Built" / "Pipboy_StatsPage.swf", "Layouts/Fallout3NV/Interface/Pipboy_StatsPage.swf"),
    ]
    for path in sorted(FOMOD.rglob("*")):
        if path.is_file():
            entries.append((path, "fomod/" + path.relative_to(FOMOD).as_posix()))
    return entries


def default_output() -> Path:
    info = ET.parse(FOMOD / "info.xml").getroot()
    name = (info.findtext("Name") or "Per Limb Healing").strip()
    version = (info.findtext("Version") or "").strip()
    suffix = f" {version}" if version else ""
    return ROOT / f"{name}{suffix} FOMOD.zip"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, help="Output ZIP path")
    args = parser.parse_args()
    output = (args.output or default_output()).resolve()

    entries = package_entries()
    missing = [str(path.relative_to(ROOT)) for path, _ in entries if not path.is_file()]
    if missing:
        raise SystemExit("Missing installer input(s):\n  " + "\n  ".join(missing))

    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, "w", compression=zipfile.ZIP_DEFLATED, compresslevel=9) as zf:
        for path, archive_name in sorted(entries, key=lambda item: item[1].lower()):
            info = zipfile.ZipInfo(archive_name, FIXED_TIME)
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            zf.writestr(info, path.read_bytes())

    with zipfile.ZipFile(output, "r") as zf:
        bad = zf.testzip()
        if bad:
            raise SystemExit(f"ZIP integrity check failed at: {bad}")
        names = set(zf.namelist())
        required = {
            "Core/F4SE/Plugins/PerLimbHealing.dll",
            "Core/MCM/Config/PerLimbHealing/config.json",
            "Core/MCM/Config/PerLimbHealing/settings.ini",
            "Layouts/Fallout4/Interface/Pipboy_StatsPage.swf",
            "Layouts/Fallout3NV/Interface/Pipboy_StatsPage.swf",
            "fomod/ModuleConfig.xml",
            "fomod/info.xml",
            "fomod/images/Fallout 4.jpg",
            "fomod/images/Fallout 3-NV.jpg",
        }
        if names != required:
            missing_names = sorted(required - names)
            extra_names = sorted(names - required)
            raise SystemExit(
                "Unexpected installer contents."
                + ("\nMissing: " + ", ".join(missing_names) if missing_names else "")
                + ("\nExtra: " + ", ".join(extra_names) if extra_names else "")
            )

    print(output)


if __name__ == "__main__":
    main()
