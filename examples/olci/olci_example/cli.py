"""Command-line interface: plot Sentinel-3 OLCI OTCI and footprint.

Usage::

    python -m olci_example SEN3_DIR [--output-dir output] [--show] [--stride N]

``SEN3_DIR`` is a local Sentinel-3 ``OL_2_LFR___`` ``.SEN3`` product
package (not bundled with this repository — the files are large).  Two PNGs
are written to the output directory: ``otci_map.png`` and ``footprint.png``.
"""

import argparse
from pathlib import Path

from . import plots, reader


def main(argv: list[str] | None = None) -> int:
    """Produce the OTCI map and footprint figures.

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
        prog="plot-olci-otci",
        description="Plot Sentinel-3 OLCI Terrestrial Chlorophyll Index and lat/lon bounds from an OL_2_LFR___ SEN3 package.",
    )
    parser.add_argument(
        "sen3_dir",
        type=Path,
        help="Path to a Sentinel-3 OL_2_LFR___ .SEN3 directory (not bundled with this repo; provide a local path)",
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
    parser.add_argument(
        "--stride",
        type=int,
        default=10,
        help="down-sampling stride for the OTCI map (default: 10)",
    )
    args = parser.parse_args(argv)

    if not args.sen3_dir.is_dir():
        parser.error(f"SEN3 directory does not exist: {args.sen3_dir}")

    otci = reader.load_otci(args.sen3_dir)
    bounds = reader.lonlat_bounds(otci)

    otci_png = plots.plot_otci_map(
        otci,
        out_path=args.output_dir / "otci_map.png",
        show=args.show,
        stride=args.stride,
    )
    fp_png = plots.plot_footprint(
        bounds,
        out_path=args.output_dir / "footprint.png",
        show=args.show,
    )
    print(f"Wrote {otci_png}")
    print(f"Wrote {fp_png}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
