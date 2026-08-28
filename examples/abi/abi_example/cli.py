"""Command-line interface: plot GOES-R ABI CMIP imagery and footprint.

Usage::

    python -m abi_example FILE [--output-dir output] [--show]
    python -m abi_example --date YYYY-MM-DDTHH:MM --satellite GOES-16 --channel 09 --region M1 [--output-dir output] [--show]

``FILE`` is a local ABI CMIP ``.nc`` product (not bundled with this
repository — the files are large).  Alternatively, provide
``--date YYYY-MM-DDTHH:MM`` together with ``--satellite``, ``--channel``,
and ``--region`` (mutually exclusive with ``FILE``) to download the
closest matching GOES-R ABI CMIP granule from the NOAA AWS Open Data
registry into ``data/`` (skipped if already cached; requires outbound
HTTPS access to AWS, but no Earthdata Login).  Two PNGs are written to
the output directory: ``cmi_map.png`` and ``footprint.png``.
"""

import argparse
from pathlib import Path

from . import fetch, plots, reader


def main(argv: list[str] | None = None) -> int:
    """Produce the CMI map and footprint figures.

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
        prog="plot-abi-cmi",
        description="Plot ABI Cloud and Moisture Imagery and lat/lon bounds from a CMIP file.",
    )
    parser.add_argument(
        "file",
        nargs="?",
        type=Path,
        default=None,
        help="ABI CMIP .nc file (not bundled with this repo; provide a local path)",
    )
    parser.add_argument(
        "--date",
        metavar="YYYY-MM-DDTHH:MM",
        help="target UTC date/time for fetching the closest ABI CMIP granule",
    )
    parser.add_argument(
        "--satellite",
        default="GOES-16",
        help="GOES satellite: GOES-16/17/18/19 or east/west (default: GOES-16)",
    )
    parser.add_argument(
        "--channel",
        default="09",
        help="ABI channel 01-16 (default: 09)",
    )
    parser.add_argument(
        "--region",
        default="F",
        help="sector: F (full disk), C (CONUS), M1/M2 (mesoscale) (default: F)",
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

    if args.date is not None and args.file is not None:
        parser.error("--date and FILE are mutually exclusive; provide one or the other")

    if args.date is not None:
        try:
            path = fetch.fetch_for_datetime(
                args.satellite, args.channel, args.region, args.date
            )
        except (ValueError, FileNotFoundError, RuntimeError) as exc:
            parser.exit(1, f"{parser.prog}: error: {exc}\n")
    elif args.file is not None:
        path = args.file
    else:
        parser.error(
            "a FILE path or --date/--satellite/--channel/--region is required "
            "(no sample file is bundled)"
        )

    cmi = reader.load_cmi(path)
    bounds = reader.lonlat_bounds(path)

    cmi_png = plots.plot_cmi_map(
        cmi, out_path=args.output_dir / "cmi_map.png", show=args.show
    )
    fp_png = plots.plot_footprint(
        bounds, out_path=args.output_dir / "footprint.png", show=args.show
    )
    print(f"Wrote {cmi_png}")
    print(f"Wrote {fp_png}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
