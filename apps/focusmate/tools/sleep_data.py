"""
Saves Moonlite's daily sleep (the last twelve weeks) for the sleep color
sketch (tools/sketch/sleep-colors.html) as tools/out/sleep-data.js, which is
gitignored: the repo is public and the numbers are personal.

Usage: python3 tools/sleep_data.py
"""
import json
import re
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "tools/out/sleep-data.js"

token = re.search(r'MOONLITE_API_TOKEN "(.*?)"', (ROOT / "src/secrets.h").read_text()).group(1)
request = urllib.request.Request("https://moonlite.im/api/v1/stats", headers={"Authorization": f"Bearer {token}"})
stats = json.load(urllib.request.urlopen(request))
nights = [[day["date"], day["sleep_seconds"]] for day in stats["sleep"]]

OUT.parent.mkdir(exist_ok=True)
OUT.write_text(f"// Made by tools/sleep_data.py: [date, sleep seconds or null], oldest first\nconst SLEEP_NIGHTS = {json.dumps(nights)};\n")
print(f"{OUT.relative_to(ROOT)}: {sum(seconds is not None for _, seconds in nights)} nights")
