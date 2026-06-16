
# NEP Diagram Rules

All diagrams in this project are intended for print in a book. Follow these rules without exception.

## Layout
- **Vertical orientation only.** Never side-by-side or horizontal layouts.
- Diagrams must fit a single column of a printed page.
- **Maximum figure height: 6.1 inches.** Do not exceed this. Use `figsize=(width, height)` with height ≤ 6.1.
- Typical width: 7 inches. Do not exceed 8 inches wide.

## Color
- **Black and white only.** No color fills, colored lines, or colored text.
- Use patterns (hatching, dotting) or line weights to distinguish elements instead of color.
- Grayscale shading is acceptable only where necessary for clarity.

## Caption and Metadata
- **Captions must NOT appear inside the diagram file itself.**
- Every diagram must have a companion `metadata.txt` file in the same directory.
- **Maximum caption length: 75 words.** Be concise — state the data source, dataset, and what the reader should observe.
- The `metadata.txt` file must contain exactly these three fields:

```
title: <short diagram title>
caption: <full caption explaining the diagram, its data source, and what the reader should observe>
alt_text: <plain-language description of the diagram for accessibility>
```

## File Naming
- Diagram file: `<descriptive_name>.<ext>` (e.g., `cache_tuning_plot.png`, `opendap-architecture-diagram.svg`)
- Metadata file: `<descriptive_name>_metadata.txt` in the same directory as the diagram.

## Enforcement
These rules apply to all diagram types: PNG plots, SVG diagrams, and any other visual output generated or edited in this project.
