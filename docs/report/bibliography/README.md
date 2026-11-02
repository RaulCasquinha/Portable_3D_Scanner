# bibliography

BibTeX/biblatex bibliography for the thesis report.

| File | Description |
|------|-------------|
| `references.bib` | All cited references — add new entries here |
| `unresolvedReferences.md` | Works-in-progress: citations that need a proper BibTeX entry |

## Adding a reference

Append a new entry to `references.bib` using standard BibTeX format, then cite it in `paper.tex` with `\cite{key}`. Run `latexmk -pdf paper.tex` to rebuild.
