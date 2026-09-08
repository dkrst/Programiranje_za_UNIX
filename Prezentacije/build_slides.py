#!/usr/bin/env python3
"""
Generiranje PDF prezentacija (pandoc -> beamer) za kolegij
"Programiranje za UNIX".

Uporaba:
    ./build_slides.py            # sve prezentacije
    ./build_slides.py P01        # samo poglavlja koja pocinju s "P01"

Za svaki .md u direktoriju P*/ generira se .pdf istog imena.

Preduvjeti: pandoc, xelatex, lmodern, DejaVu fontovi.
"""

import subprocess
import sys
from pathlib import Path

BASE = Path(__file__).resolve().parent

PANDOC_OPTS = [
    "-t", "beamer",
    "--pdf-engine=xelatex",
    "--slide-level=2",
    "--highlight-style=tango",
    "-V", "mainfont=DejaVu Serif",
    "-V", "sansfont=DejaVu Sans",
    "-V", "monofont=DejaVu Sans Mono",
    "-V", "fontsize=10pt",
    "-V", "lang=hr",
    "-V", "theme=default",
    "-V", "colortheme=default",
    "-H", str(BASE / "fesb.tex"),
    "-V", "aspectratio=169",
    "-V", "classoption=t",
]


def build(src: Path) -> bool:
    out = src.with_suffix(".pdf")
    cmd = ["pandoc", src.name, *PANDOC_OPTS, "-o", out.name]
    print(f"==> {src.parent.name}/{src.name}")
    res = subprocess.run(cmd, cwd=src.parent, capture_output=True, text=True)
    if res.returncode != 0:
        print(res.stdout)
        print(res.stderr)
        return False
    print(f"    OK: {out.relative_to(BASE)}")
    return True


def main() -> int:
    prefix = sys.argv[1] if len(sys.argv) > 1 else ""
    sources = sorted(
        p
        for p in BASE.glob("P*/*.md")
        if p.name != "README.md" and p.parent.name.startswith(prefix)
    )
    if not sources:
        print(f"Nema prezentacija za '{prefix}'.")
        return 1
    ok = all([build(s) for s in sources])
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
