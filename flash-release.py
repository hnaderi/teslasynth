#!/usr/bin/env python3
# Copyright Hossein Naderi 2025, 2026
# SPDX-License-Identifier: GPL-3.0-only

import argparse
import json
import re
import subprocess
import sys
import urllib.request
from pathlib import Path

CATALOG_BASE = "https://cdn.jsdelivr.net/gh/hnaderi/teslasynth@firmware"
CACHE_DIR = Path.home() / ".cache" / "teslasynth"
FIRST_SUPPORTED = "v0.4.1"
DEFAULT_BAUD = 460800


def fail(msg):
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(1)


def fetch_json(url):
    try:
        with urllib.request.urlopen(url) as resp:
            return json.loads(resp.read())
    except Exception as e:
        fail(f"Failed to fetch {url}: {e}")


def download_file(url, dest: Path):
    if dest.exists():
        return
    dest.parent.mkdir(parents=True, exist_ok=True)
    print(f"  Downloading {dest.name}...")
    try:
        urllib.request.urlretrieve(url, dest)
    except Exception as e:
        fail(f"Failed to download {url}: {e}")


def get_catalog():
    data = fetch_json(f"{CATALOG_BASE}/catalog.json")
    idx = data.index(FIRST_SUPPORTED) if FIRST_SUPPORTED in data else len(data) - 1
    return data[: idx + 1]


def normalize_chip(chip_str):
    s = chip_str.upper().replace("-", "").replace("_", "")
    if "ESP32S3" in s:
        return "esp32s3"
    if "ESP32S2" in s:
        return "esp32s2"
    if "ESP32" in s:
        return "esp32"
    return s.lower()


def find_port():
    try:
        from serial.tools import list_ports
    except ImportError:
        fail("pyserial not found — run this inside the ESP-IDF dev shell")

    # Espressif native USB + common USB-serial chips (CP210x, CH340, FTDI)
    ESP_VIDS = {0x303A, 0x10C4, 0x1A86, 0x0403}
    candidates = [p for p in list_ports.comports() if p.vid in ESP_VIDS]

    if not candidates:
        candidates = [
            p for p in list_ports.comports()
            if "USB" in p.device.upper() or "ACM" in p.device.upper()
        ]

    if not candidates:
        fail("No serial port found. Specify one with --port.")
    if len(candidates) > 1:
        ports = [p.device for p in candidates]
        fail(f"Multiple ports found: {ports}. Specify one with --port.")

    port = candidates[0].device
    print(f"Auto-detected port: {port}")
    return port


def detect_chip(port, baud):
    print(f"Detecting chip on {port}...")
    try:
        result = subprocess.run(
            [sys.executable, "-m", "esptool", "--port", port, "--baud", str(baud),
             "--after", "no-reset", "chip-id"],
            capture_output=True,
            text=True,
        )
        output = result.stdout + result.stderr
        # matches "Chip is ESP32-S2", "Chip type: ESP32-S2FNR2", "Connected to ESP32-S2 on"
        m = re.search(r"(?:Chip\s+(?:is|type:)\s*|Connected to\s+)(ESP32[\w-]*)", output, re.IGNORECASE)
        if not m:
            fail(f"Could not detect chip from esptool output:\n{output}")
        return normalize_chip(m.group(1))
    except ImportError:
        fail("esptool module not found — run this inside the ESP-IDF dev shell")


def find_target(manifest, chip_id):
    for key, target in manifest["targets"].items():
        if target["extra_esptool_args"]["chip"] == chip_id:
            return key, target
    available = [t["extra_esptool_args"]["chip"] for t in manifest["targets"].values()]
    fail(f"No manifest target matches chip '{chip_id}'. Available: {available}")


def flash(port, baud, version, chip_override=None):
    print(f"Fetching manifest for {version}...")
    manifest = fetch_json(f"{CATALOG_BASE}/{version}/manifest.json")

    chip_id = chip_override if chip_override else detect_chip(port, baud)
    target_key, target = find_target(manifest, chip_id)
    print(f"Target: {target_key} ({chip_id})")

    cache_path = CACHE_DIR / version
    print(f"Fetching firmware files (cache: {cache_path})...")
    for f in target["files"]:
        download_file(f"{CATALOG_BASE}/{version}/{f['path']}", cache_path / f["path"])

    settings = target["flash_settings"]
    extra = target["extra_esptool_args"]

    cmd = [
        sys.executable, "-m", "esptool",
        "--port",
        port,
        "--baud",
        str(baud),
        "--chip",
        chip_id,
        "--before",
        extra["before"],
        "--after",
        extra["after"],
        "write-flash",
        "--flash-mode",
        settings["flash_mode"],
        "--flash-size",
        settings["flash_size"],
        "--flash-freq",
        settings["flash_freq"],
    ]
    for f in target["files"]:
        cmd += [f["offset"], str(cache_path / f["path"])]

    print(f"\nFlashing {version} to {port}...\n")
    subprocess.run(cmd, check=True)


def main():
    parser = argparse.ArgumentParser(
        description="Flash Teslasynth firmware from the published release."
    )
    parser.add_argument("--port", "-p", default=None, help="Serial port (auto-detected if omitted)")
    parser.add_argument(
        "--baud", "-b", type=int, default=DEFAULT_BAUD, help="Baud rate"
    )
    parser.add_argument(
        "--version", "-v", default=None, help="Firmware version (default: latest)"
    )
    parser.add_argument(
        "--chip",
        "-c",
        default=None,
        help="Skip detection, use this chip (e.g. esp32s3)",
    )
    parser.add_argument(
        "--list-versions", action="store_true", help="List available versions and exit"
    )
    args = parser.parse_args()

    catalog = get_catalog()

    if args.list_versions:
        print("Available versions:")
        for v in catalog:
            print(f"  {v}")
        return

    version = args.version or catalog[0]
    if version not in catalog:
        fail(f"Version '{version}' not found. Use --list-versions to see available.")

    port = args.port or find_port()
    flash(port, args.baud, version, args.chip)


if __name__ == "__main__":
    main()
