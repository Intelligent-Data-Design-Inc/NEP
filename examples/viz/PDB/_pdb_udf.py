"""Minimal ctypes helper to read legacy PDB atom coordinates through the NetCDF-C UDF interface.

The netCDF4-python Dataset constructor does not expose the NC_UDF7 mode flag
required to open legacy PDB files whose magic string ("HEADER") is checked by
the NEP dispatch layer, so this helper loads libncpdb, calls
NC_PDB_initialize(), and then uses nc_open() with NC_UDF7 and nc_get_vara_*
to read the atom_site_Cartn_x/y/z and atom_site_group_PDB variables.

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
NC_UDF7 = 0x800000
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
            f"the NetCDF-C and NEP PDB library directories"
        ) from exc


def _nc_strerror(libnc, status):
    """Return a NetCDF error message string."""
    libnc.nc_strerror.restype = c_char_p
    return libnc.nc_strerror(status).decode("utf-8", errors="replace")


def _check(libnc, status):
    """Raise RuntimeError on a non-zero NetCDF status."""
    if status != NC_NOERR:
        raise RuntimeError(f"NetCDF error {status}: {_nc_strerror(libnc, status)}")


def _ensure_pdb_handler(libpdb):
    """Load and initialize the NEP PDB UDF handler.

    This mirrors the explicit NC_PDB_initialize() call in the C/Fortran
    PDB tests. The call is safe even when the handler is already registered
    (e.g., via .ncrc autoload).
    """
    libpdb.NC_PDB_initialize.restype = c_void_p
    libpdb.NC_PDB_initialize()


def read_structure(path):
    """Open *path* via the PDB UDF handler and read atom coordinates.

    Returns a dict with keys ``x``, ``y``, ``z`` (each a ``numpy.ndarray``
    shaped ``[model][atom]``) and ``group`` (a list of ``natoms`` strings,
    each ``"ATOM"`` or ``"HETATM"``).
    """
    path = Path(path)
    if not path.is_file():
        raise FileNotFoundError(path)

    libnc = _load_library("libnetcdf.so.22")
    libpdb = _load_library("libncpdb.so")

    libnc.nc_open.argtypes = [c_char_p, c_int, ctypes.POINTER(c_int)]
    libnc.nc_open.restype = c_int

    _ensure_pdb_handler(libpdb)

    ncid = c_int()
    _check(libnc, libnc.nc_open(str(path).encode("utf-8"), NC_UDF7 | NC_NOWRITE, ctypes.byref(ncid)))
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

        def _read_float_var(var_name):
            varid = c_int()
            _check(libnc, libnc.nc_inq_varid(ncid.value, var_name.encode("utf-8"), ctypes.byref(varid)))
            data = np.empty(shape, dtype=np.float32, order="C")
            libnc.nc_get_var_float.argtypes = [c_int, c_int, c_void_p]
            libnc.nc_get_var_float.restype = c_int
            _check(libnc, libnc.nc_get_var_float(ncid.value, varid.value, data.ctypes.data_as(c_void_p)))
            return data

        x = _read_float_var("atom_site_Cartn_x")
        y = _read_float_var("atom_site_Cartn_y")
        z = _read_float_var("atom_site_Cartn_z")

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
