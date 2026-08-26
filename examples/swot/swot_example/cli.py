"""Command-line interface: plot SWOT L2/L3 SSH SSHA map and footprint.

Usage::

    python -m swot_example FILE [--output-dir output] [--show]
    python -m swot_example --bbox W S E N [--output-dir output] [--show]

``FILE`` is a local SWOT L2/L3 SSH ``.nc`` product (not bundled with this
repository — the files are large).  Alternatively, ``--bbox W S E N``
(mutually exclusive with ``FILE``) searches NASA Earthdata/PO.DAAC for the
most recent SWOT L2 LR SSH granule intersecting the lat/lon box and
downloads it into ``data/`` (skipped if already cached; requires an
Earthdata Login account).  Two PNGs are written to the output directory:
``ssha_map.png`` and ``swath_footprint.png``.
"""

import argparse
from pathlib import Path

from . import fetch, plots, l3_ssh


def main(argv: list[str] | None = None) -> int:
    """Produce the SSHA map and footprint figures.

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
        prog="plot-swot-ssh",
        description="Plot sea surface height anomaly and lat/lon bounds from a SWOT SSH file.",
    )
    source = parser.add_mutually_exclusive_group()
    source.add_argument(
        "file",
        nargs="?",
        type=Path,
        default=None,
        help="SWOT L2/L3 SSH .nc file (not bundled with this repo; provide a local path)",
    )
    source.add_argument(
        "--bbox",
        nargs=4,
        type=float,
        metavar=("W", "S", "E", "N"),
        help="lat/lon box (west south east north, degrees); fetches the "
        "most recent SWOT L2 LR SSH granule intersecting it into data/",
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

    ssha = l3_ssh.load_ssha(path)
    bounds = l3_ssh.lonlat_bounds(path)

    ssha_png = plots.plot_ssha_map(
        ssha, out_path=args.output_dir / "ssha_map.png", show=args.show
    )
    fp_png = plots.plot_footprint(
        bounds, out_path=args.output_dir / "swath_footprint.png", show=args.show
    )
    print(f"Wrote {ssha_png}")
    print(f"Wrote {fp_png}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
