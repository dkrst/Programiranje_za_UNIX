#!/usr/bin/env python3
"""
build_pdf.py — generira PDF cijele skripte "Programiranje za UNIX"
iz README.md datoteka pojedinačnih poglavlja, uključujući glavni README
kao Predgovor.
"""

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

# -----------------------------------------------------------------------------
# Konfiguracija
# -----------------------------------------------------------------------------

CHAPTERS = [
    ("P01-Osnove_UNIXa",                  "P01_slike"),
    ("P02-Osnove_programiranja",          "P02_slike"),
    ("P03-Ulazno_izlazne_operacije",      "P03_slike"),
    ("P04-Upravljanje_datotekama",        None),
    ("P05-Okruzenje_procesa",             "P05_slike"),
    ("P06-Signali",                       None),
    ("P07-Komunikacija_izmedju_procesa",  "P07_slike"),
    ("P08-Visenitno_programiranje",       None),
    ("P09-Socketi",                       None),
]

BUILD_DIR = Path("./skripta_pdf_build")
OUTPUT_PDF = Path("./programiranje_za_unix.pdf")


YAML_HEADER = r"""---
title: "Programiranje za UNIX"
author: "Damir Krstinić, Maja Braović"
date: "Rujan 2026."
documentclass: book
papersize: a4
fontsize: 11pt
geometry:
  - top=2.5cm
  - bottom=2.5cm
  - left=2.5cm
  - right=2.5cm
toc: true
toc-depth: 2
numbersections: true
colorlinks: true
linkcolor: blue
urlcolor: blue
header-includes:
  - \usepackage{xcolor}
  - \usepackage{fancyvrb}
  - \usepackage{float}
  - \floatplacement{figure}{H}
  - \usepackage{booktabs}
  - \usepackage{longtable}
  - \usepackage{array}
  - \usepackage{tabularx}
  - \setlength{\emergencystretch}{6em}
  - \tolerance=2000
  - \sloppy
  - \usepackage{enumitem}
  - \setlist[itemize]{leftmargin=*}
  - \usepackage{microtype}
  - \AtBeginEnvironment{longtable}{\footnotesize}
  - \renewcommand{\arraystretch}{1.1}
  - \usepackage{graphicx}
  - \setkeys{Gin}{width=0.85\linewidth,keepaspectratio}
  - \usepackage{xurl}
  - \makeatletter
  - \def\verbatim@font{\scriptsize\ttfamily}
  - \makeatother
  - \usepackage{fvextra}
  - \DefineVerbatimEnvironment{Highlighting}{Verbatim}{commandchars=\\\{\},fontsize=\scriptsize,breaklines,breakanywhere,breaksymbol={\textcolor{gray}{\tiny$\hookrightarrow$}},breakanywheresymbolpre={\textcolor{gray}{$\hookleftarrow$}}}
  - \fvset{breaklines=true,breakanywhere=true,fontsize=\scriptsize}
lang: hr
---

"""


# -----------------------------------------------------------------------------
# Pretvorbe iz Markdown/HTML u LaTeX-prijateljski oblik
# -----------------------------------------------------------------------------

def escape_latex_special(text):
    """Escape LaTeX special characters within plain text."""
    text = text.replace("\\", r"\textbackslash{}")
    text = text.replace("&", r"\&")
    text = text.replace("%", r"\%")
    text = text.replace("$", r"\$")
    text = text.replace("#", r"\#")
    text = text.replace("_", r"\_")
    text = text.replace("{", r"\{")
    text = text.replace("}", r"\}")
    text = text.replace("~", r"\textasciitilde{}")
    text = text.replace("^", r"\textasciicircum{}")
    return text


def convert_html_centered_image(match):
    r"""HTML centrirani blok sa slikom -> LaTeX figure sa kurziv caption-om."""
    block = match.group(0)

    src_m = re.search(r'<img\s+[^>]*src="([^"]+)"', block, re.IGNORECASE)
    if not src_m:
        return block
    src = src_m.group(1)

    width_m = re.search(r'<img\s+[^>]*width="(\d+)%"', block, re.IGNORECASE)
    if width_m:
        width_frac = int(width_m.group(1)) / 100.0
    else:
        width_frac = 0.85

    caption_m = re.search(r"<em>(.*?)</em>", block, re.IGNORECASE | re.DOTALL)
    caption = caption_m.group(1).strip() if caption_m else ""
    # `code` i <code> u kurziv captionu -> \texttt s escape-iranim _
    def _texttt(m):
        inner = m.group(1)
        inner = inner.replace("\\", r"\textbackslash{}")
        inner = inner.replace("_", r"\_")
        inner = inner.replace("&", r"\&")
        inner = inner.replace("#", r"\#")
        inner = inner.replace("%", r"\%")
        inner = inner.replace("$", r"\$")
        return r"\texttt{" + inner + "}"
    caption = re.sub(r"`([^`]+)`", _texttt, caption)
    caption = re.sub(r"<code>(.*?)</code>", _texttt, caption, flags=re.IGNORECASE | re.DOTALL)
    caption = re.sub(r"<[^>]+>", "", caption)

    latex = (
        r"\begin{figure}[H]" "\n"
        r"\centering" "\n"
        rf"\includegraphics[width={width_frac}\linewidth,keepaspectratio]{{{src}}}" "\n"
        r"\\" "\n"
        rf"{{\small\itshape {caption}}}" "\n"
        r"\end{figure}"
    )
    return latex


def convert_html_table_to_latex(match):
    r"""HTML tablica (bash/csh side-by-side) -> LaTeX tabular s fiksnim širinama."""
    block = match.group(0)

    col_widths = re.findall(r'<col\s+width="(\d+)%"', block, re.IGNORECASE)
    if not col_widths or len(col_widths) < 2:
        col_widths = ["40", "40"]
    w1 = int(col_widths[0]) / 100.0
    w2 = int(col_widths[1]) / 100.0

    th_matches = re.findall(r"<th[^>]*>(.*?)</th>", block, re.IGNORECASE | re.DOTALL)
    headers = [t.strip() for t in th_matches[:2]] if th_matches else ["", ""]
    headers = [escape_latex_special(h) for h in headers]
    while len(headers) < 2:
        headers.append("")

    td_matches = re.findall(r"<td[^>]*>(.*?)</td>", block, re.IGNORECASE | re.DOTALL)
    cells = []
    for td in td_matches[:2]:
        code_m = re.search(r"```\w*\n(.*?)\n```", td, re.DOTALL)
        code = code_m.group(1) if code_m else td.strip()

        lines = code.split("\n")
        latex_lines = []
        for line in lines:
            stripped = line.lstrip(" ")
            leading = len(line) - len(stripped)
            escaped = escape_latex_special(stripped)
            if leading > 0:
                escaped = r"\hphantom{" + "x" * leading + "}" + escaped
            latex_lines.append(r"\mbox{\texttt{\footnotesize " + escaped + "}}")
        body = r" \\ ".join(latex_lines)
        cells.append(body)

    while len(cells) < 2:
        cells.append("")

    latex = (
        r"\begin{center}" "\n"
        rf"\begin{{tabular}}{{|p{{{w1}\linewidth}}|p{{{w2}\linewidth}}|}}" "\n"
        r"\hline" "\n"
        rf"\textbf{{{headers[0]}}} & \textbf{{{headers[1]}}} \\" "\n"
        r"\hline" "\n"
        rf"\shortstack[l]{{{cells[0]}}} & \shortstack[l]{{{cells[1]}}} \\" "\n"
        r"\hline" "\n"
        r"\end{tabular}" "\n"
        r"\end{center}"
    )
    return latex


def convert_pre_br_table(match):
    """Stari format <pre>X<br>Y</pre> -> \shortstack[l]{...}."""
    inner = match.group(1)
    lines = inner.split("<br>")
    latex_lines = []
    for line in lines:
        stripped = line.lstrip(" ")
        leading = len(line) - len(stripped)
        escaped = escape_latex_special(stripped)
        if leading > 0:
            escaped = r"\hphantom{" + "x" * leading + "}" + escaped
        latex_lines.append(r"\mbox{\texttt{\footnotesize " + escaped + "}}")
    body = r" \\ ".join(latex_lines)
    return r"\shortstack[l]{" + body + "}"


def process_chapter(repo_root, chapter_dir, slike_prefix):
    """Procesira README.md poglavlja."""
    path = repo_root / chapter_dir / "README.md"
    text = path.read_text(encoding="utf-8")

    if slike_prefix:
        text = re.sub(r'(<img\s+[^>]*src=")slike/', rf'\1{slike_prefix}/', text)
        text = re.sub(r"(\]\()slike/", rf"\1{slike_prefix}/", text)

    text = re.sub(r"\[\*\*`([^`]+)`\*\*\]\([^)]+\)", r"**`\1`**", text)
    text = re.sub(r"\[`([^`]+)`\]\([^)]+\)", r"`\1`", text)
    text = re.sub(r"\[([^\]]+)\]\(\.\./[^)]+\)", r"\1", text)

    text = re.sub(
        r'<p\s+align="center">.*?</p>',
        convert_html_centered_image,
        text,
        flags=re.DOTALL | re.IGNORECASE,
    )

    text = re.sub(
        r"<table\s+width=\"\d+%\">.*?</table>",
        convert_html_table_to_latex,
        text,
        flags=re.DOTALL | re.IGNORECASE,
    )

    text = re.sub(
        r"<pre>(.*?)</pre>",
        convert_pre_br_table,
        text,
        flags=re.DOTALL | re.IGNORECASE,
    )

    return text


def process_preface(repo_root):
    """Procesira glavni README kao Predgovor."""
    path = repo_root / "README.md"
    if not path.exists():
        return None
    text = path.read_text(encoding="utf-8")

    # Ukloni H1 naslov knjige
    text = re.sub(r"^# Programiranje za UNIX\s*\n+", "", text, count=1)

    # Sve naslove označi kao nenumerirane i neuvrštene u sadržaj —
    # u Sadržaju će se pojaviti samo "Predgovor"
    text = re.sub(
        r"^(#{2,4})\s+(.+?)\s*$",
        r"\1 \2 {.unnumbered .unlisted}",
        text,
        flags=re.MULTILINE,
    )

    # Ukloni tablicu direktorija
    text = re.sub(
        r"\| Direktorij \| Poglavlje \|.*?(?=\n\n)",
        "",
        text,
        flags=re.DOTALL,
    )

    # Ukloni linkove na lokalne direktorije i LICENSE
    text = re.sub(r"\[`([^`]+/)`\]\([^)]+\)", r"`\1`", text)
    text = re.sub(r"\[`LICENSE`\]\([^)]+\)", r"`LICENSE`", text)

    preface_latex = (
        r"\frontmatter" "\n"
        r"\chapter*{Predgovor}" "\n"
        r"\addcontentsline{toc}{chapter}{Predgovor}" "\n"
        r"\markboth{Predgovor}{Predgovor}" "\n\n"
    )
    return preface_latex + text + "\n\n" + r"\mainmatter" + "\n"


def build(repo_root):
    repo_root = Path(repo_root).resolve()
    if not repo_root.is_dir():
        sys.exit(f"GREŠKA: Repo direktorij '{repo_root}' ne postoji.")

    print(f"Repo: {repo_root}")

    BUILD_DIR.mkdir(exist_ok=True)

    # Kopiraj slike
    for chapter_dir, slike_prefix in CHAPTERS:
        if slike_prefix is None:
            continue
        src_slike = repo_root / chapter_dir / "slike"
        if src_slike.is_dir():
            dst_slike = BUILD_DIR / slike_prefix
            if dst_slike.exists():
                shutil.rmtree(dst_slike)
            shutil.copytree(src_slike, dst_slike)
            print(f"  Kopirao {chapter_dir}/slike -> {slike_prefix}/")

    parts = [YAML_HEADER]

    preface = process_preface(repo_root)
    if preface:
        print("  Procesiram glavni README kao Predgovor...")
        parts.append(preface)
        parts.append("\n\n\\newpage\n\n")

    for chapter_dir, slike_prefix in CHAPTERS:
        print(f"  Procesiram {chapter_dir}...")
        chapter_text = process_chapter(repo_root, chapter_dir, slike_prefix)
        parts.append("\n\n\\newpage\n\n")
        parts.append(chapter_text)

    combined_md = BUILD_DIR / "combined.md"
    combined_md.write_text("\n".join(parts), encoding="utf-8")
    print(f"  Sastavljen combined.md ({combined_md.stat().st_size:,} bajtova)")

    pandoc_cmd = [
        "pandoc",
        "combined.md",
        "-o", str(OUTPUT_PDF.resolve()),
        "--pdf-engine=xelatex",
        "-V", "mainfont=DejaVu Serif",
        "-V", "monofont=DejaVu Sans Mono",
        "-V", "sansfont=DejaVu Sans",
        "-V", "lang=hr",
        "--highlight-style=tango",
    ]
    print(f"\nPokrećem pandoc: pandoc combined.md -o {OUTPUT_PDF}")
    result = subprocess.run(pandoc_cmd, cwd=BUILD_DIR, capture_output=True, text=True)

    if result.returncode != 0:
        print("GREŠKA u pandoc-u:")
        print(result.stderr[-2000:])
        sys.exit(1)

    print(f"\n✓ PDF kreiran: {OUTPUT_PDF.resolve()}")
    print(f"  Veličina: {OUTPUT_PDF.stat().st_size:,} bajtova")


if __name__ == "__main__":
    repo = sys.argv[1] if len(sys.argv) > 1 else "./Programiranje_za_UNIX"
    build(repo)
