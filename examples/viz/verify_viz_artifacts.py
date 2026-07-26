"""Validate visualization PNG and metadata artifacts for publication requirements.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
import argparse
import os
from pathlib import Path

import matplotlib.image as mpimg

_MAX_WIDTH_PIXELS = 8.0 * 150
_MAX_HEIGHT_PIXELS = 6.1 * 150
_METADATA_FIELDS = ("title", "caption", "alt_text")


def _parse_metadata(path):
    lines = path.read_text(encoding="utf-8").splitlines()
    if len(lines) != len(_METADATA_FIELDS):
        raise ValueError("metadata must contain exactly title, caption, and alt_text")

    values = {}
    for expected_field, line in zip(_METADATA_FIELDS, lines):
        prefix = f"{expected_field}: "
        if not line.startswith(prefix):
            raise ValueError(f"metadata field must be {expected_field!r} in order")
        value = line[len(prefix) :].strip()
        if not value:
            raise ValueError(f"metadata field {expected_field!r} must be non-empty")
        values[expected_field] = value

    if len(values["caption"].split()) > 75:
        raise ValueError("metadata caption must not exceed 75 words")


def _validate_png(path):
    image = mpimg.imread(path)
    if image.ndim < 2:
        raise ValueError("PNG must have height and width")
    height, width = image.shape[:2]
    if width > _MAX_WIDTH_PIXELS or height > _MAX_HEIGHT_PIXELS:
        raise ValueError(
            f"PNG dimensions {width}x{height} exceed the 8.0x6.1 inch limit at 150 DPI"
        )


def validate_artifacts(directory, basenames):
    for basename in basenames:
        png_path = directory / f"{basename}.png"
        metadata_path = directory / f"{basename}_metadata.txt"
        if not png_path.is_file():
            raise RuntimeError(f"{basename}: missing PNG artifact: {png_path}")
        if not metadata_path.is_file():
            raise RuntimeError(f"{basename}: missing metadata artifact: {metadata_path}")
        try:
            _validate_png(png_path)
            _parse_metadata(metadata_path)
        except ValueError as exc:
            raise RuntimeError(f"{basename}: {exc}") from exc


def main():
    parser = argparse.ArgumentParser(
        description="Validate visualization PNG and metadata artifacts."
    )
    parser.add_argument(
        "directory",
        nargs="?",
        default=os.environ.get("NEP_VIZ_ARTIFACT_DIR", "."),
        help="directory containing visualization artifacts",
    )
    parser.add_argument(
        "basenames",
        nargs="*",
        default=os.environ.get("NEP_VIZ_ARTIFACTS", "").split(),
        help="artifact basenames without .png or _metadata.txt",
    )
    args = parser.parse_args()
    directory = Path(args.directory)
    if not directory.is_dir():
        parser.error(f"artifact directory does not exist: {directory}")
    if not args.basenames:
        parser.error("at least one artifact basename is required")

    validate_artifacts(directory, args.basenames)


if __name__ == "__main__":
    main()
