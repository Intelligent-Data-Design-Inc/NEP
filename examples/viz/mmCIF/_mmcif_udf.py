"""Minimal ctypes helper to read PDBx/mmCIF atom coordinates through the NetCDF-C UDF interface.

The netCDF4-python Dataset constructor does not expose the NC_UDF8 mode flag
required to open mmCIF files whose magic string ("data_") is checked by the
NEP dispatch layer, so this helper loads libncmmcif, calls
NC_MMCIF_initialize(), and then uses nc_open() with NC_UDF8 and
nc_get_var_*/nc_get_vara_text to read the atom_site_Cartn_x/y/z and
atom_site_group_PDB variables.

Companion code for "The NetCDF Developer's Handbook: The Authoritative Guide to
Writing High-Performance Programs for Scientific Data Management, Second Edition"
(https://www.amazon.com/dp/B0H7Q1Z75L).
"""
import ctypes
import os
from ctypes import c_char_p, c_int, c_size_t, c_void_p
from pathlib import Path

import numpy as np

# NetCDF-C constants used by the helper.
NC_NOERR = 0
NC_NOWRITE = 0x0000
NC_UDF8 = 0x1000000
NC_MAX_NAME = 256

GROUP_FIELD_LEN = 6


def _load_library(name):
    """Load a shared library, respecting LD_LIBRARY_PATH."""
    path = os.environ.get(name.upper().replace("-", "_") + "_LIBRARY", name)
    try:
        return ctypes.CDLL(path, mode=ctypes.RTLD_GLOBAL)
    except OSError as exc:
        raise OSError(
            f"unable to load {name}; ensure LD_LIBRARY_PATH includes "
            f"the NetCDF-C and NEP mmCIF library directories"
        ) from exc


def _nc_strerror(libnc, status):
    """Return a NetCDF error message string."""
    libnc.nc_strerror.restype = c_char_p
    return libnc.nc_strerror(status).decode("utf-8", errors="replace")


def _check(libnc, status):
    """Raise RuntimeError on a non-zero NetCDF status."""
    if status != NC_NOERR:
        raise RuntimeError(f"NetCDF error {status}: {_nc_strerror(libnc, status)}")


def _ensure_mmcif_handler(libmmcif):
    """Load and initialize the NEP mmCIF UDF handler.

    This mirrors the explicit NC_MMCIF_initialize() call in the C/Fortran
    mmCIF tests. The call is safe even when the handler is already
    registered (e.g., via .ncrc autoload).
    """
    libmmcif.NC_MMCIF_initialize.restype = c_void_p
    libmmcif.NC_MMCIF_initialize()


def read_structure(path):
    """Open *path* via the mmCIF UDF handler and read atom coordinates.

    Returns a dict with keys ``x``, ``y``, ``z`` (each a ``numpy.ndarray``
    shaped ``[model][atom]``) and ``group`` (a list of ``natoms`` strings,
    each ``"ATOM"`` or ``"HETATM"``).
    """
    path = Path(path)
    if not path.is_file():
        raise FileNotFoundError(path)

    libnc = _load_library("libnetcdf.so.22")
    libmmcif = _load_library("libncmmcif.so")

    libnc.nc_open.argtypes = [c_char_p, c_int, ctypes.POINTER(c_int)]
    libnc.nc_open.restype = c_int

    _ensure_mmcif_handler(libmmcif)

    ncid = c_int()
    _check(libnc, libnc.nc_open(str(path).encode("utf-8"), NC_UDF8 | NC_NOWRITE, ctypes.byref(ncid)))
    try:
        model_dimid = c_int()
        atom_dimid = c_int()
        _check(libnc, libnc.nc_inq_dimid(ncid.value, b"model", ctypes.byref(model_dimid)))
        _check(libnc, libnc.nc_inq_dimid(ncid.value, b"atom", ctypes.byref(atom_dimid)))

        libnc.nc_inq_dimlen.argtypes = [c_int, c_int, ctypes.POINTER(c_size_t)]
        libnc.nc_inq_dimlen.restype = c_int

        nmodels = c_size_t()
        natoms = c_size_t()
        _check(libnc, libnc.nc_inq_dimlen(ncid.value, model_dimid.value, ctypes.byref(nmodels)))
        _check(libnc, libnc.nc_inq_dimlen(ncid.value, atom_dimid.value, ctypes.byref(natoms)))

        shape = (nmodels.value, natoms.value)

        def _read_double_var(var_name):
            varid = c_int()
            _check(libnc, libnc.nc_inq_varid(ncid.value, var_name.encode("utf-8"), ctypes.byref(varid)))
            data = np.empty(shape, dtype=np.float64, order="C")
            libnc.nc_get_var_double.argtypes = [c_int, c_int, c_void_p]
            libnc.nc_get_var_double.restype = c_int
            _check(libnc, libnc.nc_get_var_double(ncid.value, varid.value, data.ctypes.data_as(c_void_p)))
            return data

        x = _read_double_var("atom_site_Cartn_x")
        y = _read_double_var("atom_site_Cartn_y")
        z = _read_double_var("atom_site_Cartn_z")

        group_varid = c_int()
        _check(libnc, libnc.nc_inq_varid(ncid.value, b"atom_site_group_PDB", ctypes.byref(group_varid)))
        buf = ctypes.create_string_buffer(natoms.value * GROUP_FIELD_LEN)
        start = (c_size_t * 2)(0, 0)
        count = (c_size_t * 2)(natoms.value, GROUP_FIELD_LEN)
        libnc.nc_get_vara_text.argtypes = [
            c_int, c_int, ctypes.POINTER(c_size_t), ctypes.POINTER(c_size_t), c_char_p,
        ]
        libnc.nc_get_vara_text.restype = c_int
        _check(libnc, libnc.nc_get_vara_text(ncid.value, group_varid.value, start, count, buf))
        raw = buf.raw
        group = [
            raw[i * GROUP_FIELD_LEN:(i + 1) * GROUP_FIELD_LEN].rstrip(b"\x00 ").decode("ascii")
            for i in range(natoms.value)
        ]

        return {"x": x, "y": y, "z": z, "group": group}
    finally:
        libnc.nc_close(ncid.value)
