#!/usr/bin/env python3
import json
import sys
import os
import shutil
import subprocess
from pathlib import Path

FILE_MAP = {
    "bootloader/bootloader.bin": "bootloader.bin",
    "partition_table/partition-table.bin": "partitions.bin",
    "teslasynth.bin": "firmware.bin",
}

DEFAULT_BAUDRATE = 460800
DEFAULT_CHIP = "esp32"


def fail(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def load_json(path: Path):
    try:
        return json.loads(path.read_text())
    except Exception as e:
        fail(f"Failed to read {path}: {e}")


def generate_manifest(build_dir: Path, out_dir: Path, version: str):
    targets = {}
    if not out_dir.exists():
        os.mkdir(out_dir)
    if not out_dir.is_dir():
        fail(f"Output path is not a directory: {out_dir}")

    for directory in os.listdir(build_dir):
        current_path = build_dir / directory
        flasher_args_path = current_path / "flasher_args.json"
        if not flasher_args_path.is_file():
            continue

        flasher = load_json(flasher_args_path)
        flash_files = flasher.get("flash_files")
        if not flash_files:
            fail("flasher_args.json does not contain flash_files")

        files = []

        for offset, logical_path in flash_files.items():
            filename = FILE_MAP.get(logical_path)
            if not filename:
                fail(f"Unknown flash file mapping for '{logical_path}'")

            src_path = current_path / filename
            dest_filename = f"{directory}-{filename}"
            dest_path = out_dir / dest_filename
            shutil.copy(src_path, dest_path)

            files.append({"offset": offset, "path": dest_filename})

        targets[directory] = {
            "flash_settings": flasher["flash_settings"],
            "extra_esptool_args": flasher["extra_esptool_args"],
            "files": files,
        }

    manifest = {"name": "Teslasynth Firmware", "version": version, "targets": targets}
    out_dir.mkdir(parents=True, exist_ok=True)
    manifest_path = out_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2))

    print(f"Manifest written to {manifest_path}")


def calculate_version():
    return subprocess.check_output(["git", "describe"]).decode("ascii").strip()


if __name__ == "__main__":
    build_dir = (
        Path(sys.argv[1]) if len(sys.argv) >= 2 else Path(".pio", "build").resolve()
    )
    dist_dir = Path("dist").resolve()
    version = sys.argv[2] if len(sys.argv) == 3 else calculate_version()

    generate_manifest(build_dir, dist_dir, version)
