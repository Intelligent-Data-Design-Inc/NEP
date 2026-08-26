"""Command-line interface: plot NISAR SME2 soil moisture and footprint.

Usage::

    python -m nisar_example FILE [--output-dir output] [--show]
    python -m nisar_example --bbox W S E N [--output-dir output] [--show]

``FILE`` is a local NISAR SME2 ``.h5`` product (not bundled with this
repository — the files are too large). Alternatively, ``--bbox W S E N``
(mutually exclusive with ``FILE``) searches NASA Earthdata for the most
recent SME2 granule intersecting the lat/lon box and downloads it into
``data/`` (skipped if already cached; requires an Earthdata Login
account). Two PNGs are written to the output directory:
``soil_moisture.png`` and ``footprint.png``.
"""

import argparse
from pathlib import Path

from . import fetch, plots, sme2


def main(argv: list[str] | None = None) -> int:
    """Produce the soil moisture and footprint figures.

    Parameters
    ----------
    argv : list of str or None, optional
        Command-line arguments; defaults to ``sys.argv[1:]``.

    Returns
    -------
    int
        Process exit code (0 on success).
    """
    parser = argparse.ArgumentParser(
        prog="plot-nisar-sme2",
        description="Plot soil moisture and lat/lon bounds from a NISAR SME2 file.",
    )
    source = parser.add_mutually_exclusive_group()
    source.add_argument(
        "file",
        nargs="?",
        type=Path,
        default=None,
        help="SME2 .h5 file (not bundled with this repo; provide a local path)",
    )
    source.add_argument(
        "--bbox",
        nargs=4,
        type=float,
        metavar=("W", "S", "E", "N"),
        help="lat/lon box (west south east north, degrees); fetches the "
        "most recent SME2 granule intersecting it into data/",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("output"),
        help="directory for output PNGs (default: output/)",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="also display the figures interactively",
    )
    args = parser.parse_args(argv)

    if args.bbox is not None:
        try:
            path = fetch.fetch_for_bbox(tuple(args.bbox))
        except (ValueError, FileNotFoundError, RuntimeError) as exc:
            parser.exit(1, f"{parser.prog}: error: {exc}\n")
    elif args.file is not None:
        path = args.file
    else:
        parser.error("a FILE path or --bbox is required (no sample file is bundled)")

    sm = sme2.load_soil_moisture(path)
    bounds = sme2.lonlat_bounds(path)

    sm_png = plots.plot_soil_moisture(
        sm, out_path=args.output_dir / "soil_moisture.png", show=args.show
    )
    fp_png = plots.plot_footprint(
        bounds, out_path=args.output_dir / "footprint.png", show=args.show
    )
    print(f"Wrote {sm_png}")
    print(f"Wrote {fp_png}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
