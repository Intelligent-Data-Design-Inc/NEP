"""Search and download GOES-R ABI CMIP granules from NOAA AWS Open Data.

Uses ``boto3`` to list and download from the public NOAA GOES buckets
(``noaa-goes16``, ``noaa-goes18``, ``noaa-goes19``).  No AWS credentials are
required.  The requested date/time is converted to a year/day-of-year/hour
prefix; the closest matching ``OR_ABI-L2-CMIP...`` NetCDF granule is
downloaded into a local ``data/`` directory.  Downloads are skipped when the
granule file is already present (caching).
"""

import re
from datetime import datetime, timezone
from pathlib import Path

import boto3
from botocore import UNSIGNED
from botocore.config import Config

from .reader import DATA_DIR

#: Satellite name to NOAA GOES AWS bucket.  Keys are normalized to
#: lowercase for case-insensitive matching.
SATELLITE_BUCKET = {
    "goes-16": "noaa-goes16",
    "goes-17": "noaa-goes17",
    "goes-18": "noaa-goes18",
    "goes-19": "noaa-goes19",
    "east": "noaa-goes16",
    "west": "noaa-goes18",
    "g16": "noaa-goes16",
    "g17": "noaa-goes17",
    "g18": "noaa-goes18",
    "g19": "noaa-goes19",
}

#: Valid ABI channels (1–16).
VALID_CHANNELS = tuple(f"{c:02d}" for c in range(1, 17))

#: Valid CMIP regions.
VALID_REGIONS = ("F", "C", "M1", "M2")


def validate_inputs(
    satellite: str, channel: str, region: str
) -> tuple[str, str, str]:
    """Validate and normalize satellite, channel, and region inputs.

    Parameters
    ----------
    satellite : str
        Satellite identifier, e.g. ``GOES-16``, ``GOES-18``, ``east``,
        ``west``, ``G16``.
    channel : str
        ABI channel number as a string, ``01``–``16``.
    region : str
        Sector: ``F`` (full disk), ``C`` (CONUS), ``M1`` (mesoscale 1),
        ``M2`` (mesoscale 2).

    Returns
    -------
    tuple of str
        Normalized ``(bucket_name, channel, region)``.

    Raises
    ------
    ValueError
        If any input is invalid.
    """
    key = satellite.strip().lower()
    if key not in SATELLITE_BUCKET:
        raise ValueError(
            f"unknown satellite {satellite!r}; expected one of "
            f"{list(SATELLITE_BUCKET.keys())}"
        )
    bucket = SATELLITE_BUCKET[key]

    chan = channel.strip().zfill(2)
    if chan not in VALID_CHANNELS:
        raise ValueError(
            f"invalid channel {channel!r}; expected 01–16"
        )

    reg = region.strip().upper()
    if reg not in VALID_REGIONS:
        raise ValueError(
            f"invalid region {region!r}; expected one of {VALID_REGIONS}"
        )

    return bucket, chan, reg


def parse_datetime(date_str: str) -> datetime:
    """Parse an ISO-like datetime string into a UTC datetime.

    Accepts ``YYYY-MM-DDTHH:MM`` or ``YYYY-MM-DDTHH:MM:SS``.  Missing
    seconds default to ``0``; missing timezone is treated as UTC.

    Parameters
    ----------
    date_str : str
        Datetime string to parse.

    Returns
    -------
    datetime.datetime
        UTC datetime.

    Raises
    ------
    ValueError
        If *date_str* cannot be parsed.
    """
    date_str = date_str.strip()
    for fmt in ("%Y-%m-%dT%H:%M:%S", "%Y-%m-%dT%H:%M", "%Y-%m-%d"):
        try:
            dt = datetime.strptime(date_str, fmt)
            break
        except ValueError:
            continue
    else:
        raise ValueError(
            f"cannot parse date {date_str!r}; expected YYYY-MM-DDTHH:MM "
            f"or YYYY-MM-DDTHH:MM:SS"
        )
    return dt.replace(tzinfo=timezone.utc)


def _s3_client():
    """Return a boto3 S3 client configured for anonymous access."""
    return boto3.client(
        "s3", config=Config(signature_version=UNSIGNED)
    )


def _build_prefix(product: str, dt: datetime) -> str:
    """Build the S3 key prefix for a product and datetime.

    Mesoscale sectors (``M1``/``M2``) share the ``ABI-L2-CMIPM/`` prefix;
    full disk (``F``) and CONUS (``C``) have their own top-level prefixes.
    """
    if product.startswith("ABI-L2-CMIPM"):
        return (
            f"ABI-L2-CMIPM/{dt:%Y}/{dt.timetuple().tm_yday:03d}/"
            f"{dt.hour:02d}/"
        )
    return (
        f"{product}/{dt:%Y}/{dt.timetuple().tm_yday:03d}/"
        f"{dt.hour:02d}/"
    )


def _granule_timestamp(key: str) -> datetime | None:
    """Extract the ABI start timestamp from a granule S3 key.

    Example key:
    ``.../OR_ABI-L2-CMIPM1-M6C09_G16_s20242341850206_e20242341859578_c20242341900015.nc``

    Parameters
    ----------
    key : str
        S3 object key.

    Returns
    -------
    datetime.datetime or None
        Parsed start time in UTC, or None if the key does not match the
        expected pattern.
    """
    match = re.search(r"_s(\d{14})_e\d{14}_c\d+\.nc$", key)
    if not match:
        return None
    ts = match.group(1)
    try:
        # ABI timestamps are year + day-of-year + hour + minute + second +
        # tenth-of-second.  Drop the final tenth-of-second digit before
        # parsing with the standard strptime format.
        return datetime.strptime(ts[:13], "%Y%j%H%M%S").replace(
            tzinfo=timezone.utc
        )
    except ValueError:
        return None


def search_granules(
    bucket: str,
    product: str,
    dt: datetime,
    channel: str,
    region: str | None = None,
) -> list[str]:
    """List candidate ABI CMIP granule keys for a bucket/product/datetime.

    Parameters
    ----------
    bucket : str
        NOAA GOES AWS bucket name.
    product : str
        ABI product prefix, e.g. ``ABI-L2-CMIPC`` or ``ABI-L2-CMIPM``.
    dt : datetime.datetime
        Target UTC datetime.
    channel : str
        Two-digit ABI channel, e.g. ``09``.
    region : str or None, optional
        Sector code (``F``, ``C``, ``M1``, ``M2``).  For mesoscale sectors,
        this is used to filter filenames within the shared
        ``ABI-L2-CMIPM/`` prefix.

    Returns
    -------
    list of str
        Matching S3 object keys, sorted newest first.

    Raises
    ------
    FileNotFoundError
        If no matching granules are found under the prefix.
    """
    client = _s3_client()
    prefix = _build_prefix(product, dt)
    channel_pattern = f"C{channel}_"
    region_pattern = f"-CMIP{region}-" if region else None

    keys: list[tuple[str, datetime]] = []
    paginator = client.get_paginator("list_objects_v2")
    for page in paginator.paginate(Bucket=bucket, Prefix=prefix):
        for obj in page.get("Contents", []):
            key = obj["Key"]
            if channel_pattern not in key:
                continue
            if region_pattern is not None and region_pattern not in key:
                continue
            ts = _granule_timestamp(key)
            if ts is None:
                continue
            keys.append((key, ts))

    if not keys:
        raise FileNotFoundError(
            f"no ABI CMIP granules found at s3://{bucket}/{prefix} "
            f"for channel {channel}"
        )

    keys.sort(key=lambda item: item[1], reverse=True)
    return [k for k, _ in keys]


def select_nearest(
    keys: list[str], dt: datetime
) -> str:
    """Return the granule key whose start time is closest to *dt*.

    Parameters
    ----------
    keys : list of str
        Candidate S3 object keys with parseable ABI start timestamps.
    dt : datetime.datetime
        Target UTC datetime.

    Returns
    -------
    str
        The closest matching key.
    """
    best_key = keys[0]
    best_delta = abs(_granule_timestamp(best_key) - dt)
    for key in keys[1:]:
        delta = abs(_granule_timestamp(key) - dt)
        if delta < best_delta:
            best_key = key
            best_delta = delta
    return best_key


def fetch_granule(
    bucket: str,
    key: str,
    dest_dir: Path = DATA_DIR,
) -> Path:
    """Download a granule from S3, unless it is already cached.

    Parameters
    ----------
    bucket : str
        NOAA GOES AWS bucket name.
    key : str
        S3 object key to download.
    dest_dir : Path, optional
        Destination directory.  Defaults to a local ``data/`` directory.

    Returns
    -------
    Path
        Local path of the downloaded ``.nc`` file.

    Raises
    ------
    RuntimeError
        If the download fails.
    """
    dest_dir = Path(dest_dir)
    local_path = dest_dir / key.rsplit("/", 1)[-1]
    if local_path.exists():
        print(f"Using cached granule {local_path}")
        return local_path

    client = _s3_client()
    dest_dir.mkdir(parents=True, exist_ok=True)
    print(f"Downloading s3://{bucket}/{key} to {dest_dir}/ ...")
    try:
        client.download_file(bucket, key, str(local_path))
    except Exception as exc:  # pragma: no cover
        raise RuntimeError(f"failed to download {key}: {exc}") from exc
    if not local_path.exists():
        raise RuntimeError(f"download did not produce {local_path}")
    return local_path


def fetch_for_datetime(
    satellite: str,
    channel: str,
    region: str,
    date_str: str,
    dest_dir: Path = DATA_DIR,
) -> Path:
    """Fetch the closest GOES-R ABI CMIP granule to a given date/time.

    Parameters
    ----------
    satellite : str
        Satellite identifier, e.g. ``GOES-16`` or ``east``.
    channel : str
        ABI channel number ``01``–``16``.
    region : str
        Sector ``F``, ``C``, ``M1``, or ``M2``.
    date_str : str
        Target UTC datetime, e.g. ``2024-07-01T18:00``.
    dest_dir : Path, optional
        Download directory.  Defaults to ``data/``.

    Returns
    -------
    Path
        Local path of the fetched ``.nc`` file.

    Raises
    ------
    ValueError
        If inputs are invalid.
    FileNotFoundError
        If no matching granule is found on AWS.
    RuntimeError
        If the download fails.
    """
    bucket, chan, reg = validate_inputs(satellite, channel, region)
    dt = parse_datetime(date_str)
    # Mesoscale sectors share the ABI-L2-CMIPM/ prefix; full disk and
    # CONUS have their own top-level prefixes.
    if reg in ("M1", "M2"):
        product = "ABI-L2-CMIPM"
    else:
        product = f"ABI-L2-CMIP{reg}"
    keys = search_granules(bucket, product, dt, chan, region=reg)
    key = select_nearest(keys, dt)
    print(
        f"Selected granule {key.rsplit('/', 1)[-1]} "
        f"(start {_granule_timestamp(key):%Y-%m-%d %H:%M:%S} UTC)"
    )
    return fetch_granule(bucket, key, dest_dir)
