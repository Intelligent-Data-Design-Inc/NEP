from pathlib import Path

import matplotlib

matplotlib.use("Agg")
matplotlib.rcParams["axes.prop_cycle"] = matplotlib.cycler(color=["black"])
matplotlib.rcParams["image.cmap"] = "gray"

_MAX_WIDTH_INCHES = 8.0
_MAX_HEIGHT_INCHES = 6.1
_DPI = 150


def save_with_metadata(fig, basename, title, caption, alt_text):
    metadata = {
        "title": title,
        "caption": caption,
        "alt_text": alt_text,
    }
    for field, value in metadata.items():
        if not isinstance(value, str) or not value.strip():
            raise ValueError(f"{field} must be a non-empty string")
        if "\n" in value or "\r" in value:
            raise ValueError(f"{field} must be a single line")

    if len(caption.split()) > 75:
        raise ValueError("caption must not exceed 75 words")

    width, height = fig.get_size_inches()
    if width <= 0 or height <= 0:
        raise ValueError("figure dimensions must be positive")
    if width > _MAX_WIDTH_INCHES or height > _MAX_HEIGHT_INCHES:
        raise ValueError(
            f"figure size {width:g}x{height:g} inches exceeds "
            f"the {_MAX_WIDTH_INCHES:g}x{_MAX_HEIGHT_INCHES:g} inch limit"
        )

    base = Path(basename)
    if not base.name or base.suffix:
        raise ValueError("basename must be a filename without an extension")
    base.parent.mkdir(parents=True, exist_ok=True)

    png_path = base.with_suffix(".png")
    metadata_path = base.parent / f"{base.name}_metadata.txt"
    fig.savefig(png_path, dpi=_DPI, bbox_inches="tight")
    metadata_path.write_text(
        "".join(f"{field}: {value.strip()}\n" for field, value in metadata.items()),
        encoding="utf-8",
    )
    return png_path, metadata_path
