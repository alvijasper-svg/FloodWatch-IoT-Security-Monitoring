#!/usr/bin/env python3
"""Small pre-publish scanner for obvious credential patterns in this repository."""
from pathlib import Path
import re
import sys

ROOT = Path(sys.argv[1] if len(sys.argv) > 1 else ".").resolve()
SKIP_DIRS = {".git", ".pio", "build", "__pycache__"}
TEXT_EXTS = {".ino", ".h", ".hpp", ".c", ".cpp", ".md", ".txt", ".json", ".yaml", ".yml", ".env"}

patterns = [
    ("possible Blynk token", re.compile(r'BLYNK_(?:AUTH_TOKEN|DEVICE_TOKEN)\s+"(?!YOUR_|REPLACE_)[^"\n]{10,}"')),
    ("possible Wi-Fi password", re.compile(r'(?:WIFI_PASSWORD|\bpass\s*\[.*?\])\s*=*\s*"(?!YOUR_|REPLACE_)[^"\n]{4,}"')),
    ("possible generic secret", re.compile(r'(?i)(?:api[_-]?key|secret[_-]?key|password)\s*[:=]\s*["\'][^"\']{8,}["\']')),
]

findings = []
for path in ROOT.rglob("*"):
    if not path.is_file() or any(part in SKIP_DIRS for part in path.parts):
        continue
    if path.name == "secrets.example.h":
        continue
    if path.suffix.lower() not in TEXT_EXTS:
        continue
    try:
        text = path.read_text(encoding="utf-8", errors="ignore")
    except OSError:
        continue
    for label, pattern in patterns:
        for m in pattern.finditer(text):
            line = text.count("\n", 0, m.start()) + 1
            findings.append((path.relative_to(ROOT), line, label))

if findings:
    print("Potential secrets found:")
    for path, line, label in findings:
        print(f"  {path}:{line}: {label}")
    print("Review before publishing.")
    sys.exit(1)

print("No obvious embedded credential patterns detected.")
