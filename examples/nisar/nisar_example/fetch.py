"""Search and download NISAR SME2 granules by lat/lon bounding box.

Uses the `earthaccess <https://earthaccess.readthedocs.io/>`_ package to
query NASA's Common Metadata Repository (CMR) for SME2 granules whose
footprint intersects a user-specified lat/lon box, select the most
recently acquired match, and download it into a local ``data/``
directory. Downloads are skipped when the granule file is already
present (caching). Downloading requires an Earthdata Login account;
credentials are resolved by ``earthaccess.login`` (environment
variables, then ``~/.netrc``, then an interactive prompt).
"""

from datetime import datetime
from pathlib import Path

import earthaccess

from .sme2 import DATA_DIR

#: CMR collection short names to search, in order of preference. The
#: PROVISIONAL collection (fully calibrated) is preferred; the BETA
#: collection is only searched when no PROVISIONAL granule intersects
#: the box, so the two maturity levels are never mixed.
SHORT_NAMES = ("NISAR_L3_SME2_PROVISIONAL_V1", "NISAR_L3_SME2_BETA_V1")


def validate_bbox(bbox: tuple[float, float, float, float]) -> tuple[float, float, float, float]:
    """Validate a lat/lon bounding box.

    Parameters
    ----------
    bbox : tuple of float
        Bounding box as ``(west, south, east, north)`` in degrees.

    Returns
    -------
    tuple of float
        The validated ``(west, south, east, north)`` box.

    Raises
    ------
    ValueError
        If the box has out-of-range coordinates, ``west >= east``, or
        ``south >= north``.
    """
    west, south, east, north = (float(v) for v in bbox)
    if not (-180 <= west <= 180 and -180 <= east <= 180):
        raise ValueError(f"longitudes must be in [-180, 180]: W={west}, E={east}")
    if not (-90 <= south <= 90 and -90 <= north <= 90):
        raise ValueError(f"latitudes must be in [-90, 90]: S={south}, N={north}")
    if west >= east:
        raise ValueError(f"west ({west}) must be less than east ({east})")
    if south >= north:
        raise ValueError(f"south ({south}) must be less than north ({north})")
    return (west, south, east, north)


def granule_start_time(granule) -> datetime:
    """Return a granule's acquisition start time.

    Parameters
    ----------
    granule : earthaccess.DataGranule
        A granule search result (a UMM-G mapping).

    Returns
    -------
    datetime.datetime
        The granule's ``BeginningDateTime`` from its UMM temporal extent.
    """
    begin = granule["umm"]["TemporalExtent"]["RangeDateTime"]["BeginningDateTime"]
    return datetime.fromisoformat(begin.replace("Z", "+00:00"))


def granule_filename(granule) -> str:
    """Return the ``.h5`` product filename of a granule.

    Parameters
    ----------
    granule : earthaccess.DataGranule
        A granule search result.

    Returns
    -------
    str
        Basename of the granule's ``.h5`` download link.

    Raises
    ------
    ValueError
        If the granule has no ``.h5`` download link.
    """
    for link in granule.data_links(access="external"):
        name = link.rsplit("/", 1)[-1]
        if name.endswith(".h5"):
            return name
    raise ValueError(f"granule has no .h5 download link: {granule}")


def search_granules(bbox: tuple[float, float, float, float]) -> list:
    """Search CMR for SME2 granules intersecting a lat/lon box.

    Collections in :data:`SHORT_NAMES` are searched in order; results
    from the first collection with any match are returned, sorted by
    acquisition start time (newest first).

    Parameters
    ----------
    bbox : tuple of float
        Bounding box as ``(west, south, east, north)`` in degrees.

    Returns
    -------
    list of earthaccess.DataGranule
        Matching granules, newest first; empty if no collection matches.
    """
    bbox = validate_bbox(bbox)
    for short_name in SHORT_NAMES:
        results = earthaccess.search_data(short_name=short_name, bounding_box=bbox)
        if results:
            return sorted(results, key=granule_start_time, reverse=True)
    return []


def select_most_recent(results: list):
    """Return the most recently acquired granule from search results.

    Parameters
    ----------
    results : list of earthaccess.DataGranule
        Granules sorted newest first (as returned by
        :func:`search_granules`).

    Returns
    -------
    earthaccess.DataGranule
        The most recently acquired granule.

    Raises
    ------
    FileNotFoundError
        If *results* is empty (no granule intersects the box).
    """
    if not results:
        raise FileNotFoundError(
            "No SME2 granule intersects the requested lat/lon box"
        )
    return results[0]


def fetch_granule(granule, dest_dir: Path = DATA_DIR) -> Path:
    """Download a granule to a directory, unless it is already cached.

    Parameters
    ----------
    granule : earthaccess.DataGranule
        The granule to download.
    dest_dir : Path, optional
        Destination directory. Defaults to a local ``data/`` directory.
        Created if it does not exist.

    Returns
    -------
    Path
        Local path of the granule's ``.h5`` file.

    Raises
    ------
    RuntimeError
        If Earthdata Login fails or the download does not produce the
        expected file.
    """
    dest_dir = Path(dest_dir)
    local_path = dest_dir / granule_filename(granule)
    if local_path.exists():
        print(f"Using cached granule {local_path}")
        return local_path

    try:
        auth = earthaccess.login()
    except EOFError:
        auth = None
    if auth is None or not auth.authenticated:
        raise RuntimeError(
            "Earthdata Login failed; configure ~/.netrc for "
            "urs.earthdata.nasa.gov (see README) or set "
            "EARTHDATA_USERNAME/EARTHDATA_PASSWORD"
        )
    dest_dir.mkdir(parents=True, exist_ok=True)
    print(f"Downloading {local_path.name} to {dest_dir}/ ...")
    earthaccess.download([granule], str(dest_dir))
    if not local_path.exists():
        raise RuntimeError(f"Download did not produce {local_path}")
    return local_path


def fetch_for_bbox(
    bbox: tuple[float, float, float, float], dest_dir: Path = DATA_DIR
) -> Path:
    """Fetch the most recent SME2 granule intersecting a lat/lon box.

    Searches CMR for granules whose footprint intersects *bbox*, selects
    the most recently acquired one, and downloads it (or reuses the
    cached copy).

    Parameters
    ----------
    bbox : tuple of float
        Bounding box as ``(west, south, east, north)`` in degrees.
    dest_dir : Path, optional
        Download directory. Defaults to a local ``data/`` directory.

    Returns
    -------
    Path
        Local path of the fetched ``.h5`` file.

    Raises
    ------
    ValueError
        If *bbox* is invalid.
    FileNotFoundError
        If no SME2 granule intersects the box.
    """
    granule = select_most_recent(search_granules(bbox))
    print(f"Selected granule {granule_filename(granule)} "
          f"(acquired {granule_start_time(granule):%Y-%m-%d %H:%M:%S})")
    return fetch_granule(granule, dest_dir)
