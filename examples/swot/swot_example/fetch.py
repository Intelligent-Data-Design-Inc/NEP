"""Search and download SWOT L2/L3 SSH granules by lat/lon bounding box.

Uses the `earthaccess <https://earthaccess.readthedocs.io/>`_ package to
query NASA's Common Metadata Repository (CMR) for SWOT low-rate sea
surface height granules whose footprint intersects a user-specified
lat/lon box, select the most recently acquired match, and download it into
a local ``data/`` directory.  Downloads are skipped when the granule file
is already present (caching).  Downloading requires a NASA Earthdata
Login account for PO.DAAC-hosted collections; credentials are resolved by
``earthaccess.login`` (environment variables, then ``~/.netrc``, then an
interactive prompt).
"""

from datetime import datetime
from pathlib import Path

import earthaccess

from .l3_ssh import DATA_DIR

#: CMR collection short names to search, in order of preference.
#: Prefer the small Basic files, then the parent collection.
#: AVISO+ L3_LR_SSH collections are not hosted in CMR, so they are handled
#: via direct THREDDS/FTP access if the user selects that product.
SHORT_NAMES = (
    "SWOT_L2_LR_SSH_Basic_D",
    "SWOT_L2_LR_SSH_D",
    "SWOT_L2_LR_SSH_2.0",
)


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
        If coordinates are out of range or ``west >= east`` / ``south >= north``.
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
        A granule search result.

    Returns
    -------
    datetime.datetime
        The granule's ``BeginningDateTime`` from its UMM temporal extent.
    """
    begin = granule["umm"]["TemporalExtent"]["RangeDateTime"]["BeginningDateTime"]
    return datetime.fromisoformat(begin.replace("Z", "+00:00"))


def granule_filename(granule) -> str:
    """Return the NetCDF product filename of a granule.

    Parameters
    ----------
    granule : earthaccess.DataGranule
        A granule search result.

    Returns
    -------
    str
        Basename of the granule's ``.nc`` download link.

    Raises
    ------
    ValueError
        If the granule has no ``.nc`` download link.
    """
    for link in granule.data_links(access="external"):
        name = link.rsplit("/", 1)[-1]
        if name.endswith(".nc"):
            return name
    raise ValueError(f"granule has no .nc download link: {granule}")


def search_granules(bbox: tuple[float, float, float, float]) -> list:
    """Search CMR for SWOT SSH granules intersecting a lat/lon box.

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
        Granules sorted newest first.

    Returns
    -------
    earthaccess.DataGranule
        The most recently acquired granule.

    Raises
    ------
    FileNotFoundError
        If *results* is empty.
    """
    if not results:
        raise FileNotFoundError(
            "No SWOT L2/L3 SSH granule intersects the requested lat/lon box"
        )
    return results[0]


def fetch_granule(granule, dest_dir: Path = DATA_DIR) -> Path:
    """Download a granule to a directory, unless it is already cached.

    Parameters
    ----------
    granule : earthaccess.DataGranule
        The granule to download.
    dest_dir : Path, optional
        Destination directory.  Defaults to a local ``data/`` directory.

    Returns
    -------
    Path
        Local path of the granule's ``.nc`` file.

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
    """Fetch the most recent SWOT SSH granule intersecting a lat/lon box.

    Parameters
    ----------
    bbox : tuple of float
        Bounding box as ``(west, south, east, north)`` in degrees.
    dest_dir : Path, optional
        Download directory.  Defaults to a local ``data/`` directory.

    Returns
    -------
    Path
        Local path of the fetched ``.nc`` file.

    Raises
    ------
    ValueError
        If *bbox* is invalid.
    FileNotFoundError
        If no SWOT SSH granule intersects the box.
    RuntimeError
        If Earthdata Login or download fails.
    """
    granule = select_most_recent(search_granules(bbox))
    print(
        f"Selected granule {granule_filename(granule)} "
        f"(acquired {granule_start_time(granule):%Y-%m-%d %H:%M:%S})"
    )
    return fetch_granule(granule, dest_dir)
