"""
Saves Foodmark's weigh-ins (this week's days and the averages of the weeks
before) for the weight sketch (tools/sketch/weight-bars.html) as
tools/out/weight-data.js, which is gitignored: the repo is public and the
numbers are personal.

Usage: python3 tools/weight_data.py
"""
import json
import re
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "tools/out/weight-data.js"

token = re.search(r'FOODMARK_API_TOKEN "(.*?)"', (ROOT / "src/secrets.h").read_text()).group(1)
request = urllib.request.Request("https://next.foodmark.app/external/weights", headers={"Authorization": f"Bearer {token}", "User-Agent": "focusmate-hw"})
weights = json.load(urllib.request.urlopen(request))

OUT.parent.mkdir(exist_ok=True)
OUT.write_text(f"// Made by tools/weight_data.py: Foodmark's /external/weights as it came\nconst WEIGHTS = {json.dumps(weights)};\n")
print(f"{OUT.relative_to(ROOT)}: {len(weights.get('past_weeks', []))} past weeks")
