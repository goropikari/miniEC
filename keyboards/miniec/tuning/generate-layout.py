"""Regenerate the browser layout bundled for servers rooted at tuning/."""

import json
from pathlib import Path

directory = Path(__file__).resolve().parent
keyboard = json.loads((directory.parent / "keyboard.json").read_text(encoding="utf-8"))
layout = keyboard["layouts"]["LAYOUT"]["layout"]
lines = [json.dumps(key, separators=(",", ":")) for key in layout]
(directory / "layout.json").write_text("[\n  " + ",\n  ".join(lines) + "\n]\n", encoding="utf-8")
