# report

LaTeX source for the master's thesis paper.

## Files

| File | Description |
|------|-------------|
| `paper.tex` | Main LaTeX source |
| `paper.pdf` | Compiled PDF (git-tracked; regenerate after edits) |
| `bibliography/references.bib` | BibTeX bibliography entries |
| `bibliography/unresolvedReferences.md` | Notes on citations still to be resolved |

All other files (`*.aux`, `*.log`, `*.bbl`, etc.) are build artifacts and are git-ignored.

## Building

```bash
cd docs/report
latexmk -pdf paper.tex
```

Requires a LaTeX distribution with `latexmk`, `biblatex`, and `biber`.
