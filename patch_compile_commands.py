#!/usr/bin/env python

import json
from pathlib import Path

home = Path.home()

with open("compile_commands.json") as f:
    db = json.load(f)

packages = home / ".platformio" / "packages"

to_add = [
    f"-I{packages / 'toolchain-xtensa-esp-elf' / 'xtensa-esp-elf' / 'include'}",
    f"-I{packages / 'toolchain-xtensa-esp32' / 'xtensa-esp32-elf' / 'sys-include'}",
    "-D__XTENSA__=1",
    "-D__XTENSA_WINDOWED_ABI__=1",
    "-D__XTENSA_SOFT_FLOAT__=1",
]
flags = " ".join(to_add)
for entry in db:
    entry["command"] += f" {flags}"

with open("compile_commands.json", "w") as f:
    json.dump(db, f, indent=2)
