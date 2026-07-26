"""Minimal ctypes helper to read DICOM pixel_data through the NetCDF-C UDF interface.

The netCDF4-python Dataset constructor does not expose the NC_UDF6 mode flag
required to open DICOM files whose magic number is not at byte zero, so this
helper loads libncdicom, calls NC_DICOM_initialize(), and then uses nc_open()
with NC_UDF6 and nc_get_var_* to read the "pixel_data" variable.

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
NC_UDF6 = 0x400000
NC_MAX_NAME = 256

NC_NAT = 0
NC_BYTE = 1
NC_CHAR = 2
NC_SHORT = 3
NC_INT = 4
NC_FLOAT = 5
NC_DOUBLE = 6
NC_UBYTE = 7
NC_USHORT = 8


def _load_library(name):
    """Load a shared library, respecting LD_LIBRARY_PATH."""
    path = os.environ.get(name.upper().replace("-", "_") + "_LIBRARY", name)
    try:
        return ctypes.CDLL(path, mode=ctypes.RTLD_GLOBAL)
    except OSError as exc:
        raise OSError(
            f"unable to load {name}; ensure LD_LIBRARY_PATH includes "
            f"the NetCDF-C and NEP DICOM library directories"
        ) from exc


def _nc_strerror(libnc, status):
    """Return a NetCDF error message string."""
    libnc.nc_strerror.restype = c_char_p
    return libnc.nc_strerror(status).decode("utf-8", errors="replace")


def _check(libnc, status):
    """Raise RuntimeError on a non-zero NetCDF status."""
    if status != NC_NOERR:
        raise RuntimeError(f"NetCDF error {status}: {_nc_strerror(libnc, status)}")


def _ensure_dicom_handler(libdicom):
    """Load and initialize the NEP DICOM UDF handler.

    This mirrors the explicit NC_DICOM_initialize() call in the C/Fortran
    DICOM tests. The call is safe even when the handler is already registered
    (e.g., via .ncrc autoload).
    """
    libdicom.NC_DICOM_initialize.restype = c_int
    libdicom.NC_DICOM_initialize()


def _get_var_function(libnc, xtype):
    """Return the nc_get_var_* function and numpy dtype for a netCDF xtype."""
    if xtype == NC_UBYTE:
        return libnc.nc_get_var_uchar, np.dtype(np.uint8)
    if xtype == NC_BYTE:
        return libnc.nc_get_var_schar, np.dtype(np.int8)
    if xtype == NC_SHORT:
        return libnc.nc_get_var_short, np.dtype(np.int16)
    if xtype == NC_USHORT:
        return libnc.nc_get_var_ushort, np.dtype(np.uint16)
    if xtype == NC_INT:
        return libnc.nc_get_var_int, np.dtype(np.int32)
    if xtype == NC_FLOAT:
        return libnc.nc_get_var_float, np.dtype(np.float32)
    if xtype == NC_DOUBLE:
        return libnc.nc_get_var_double, np.dtype(np.float64)
    raise ValueError(f"unsupported netCDF xtype: {xtype}")


def read_pixel_data(path):
    """Open *path* via the DICOM UDF handler and read ``pixel_data``.

    Returns a ``numpy.ndarray`` containing the full pixel_data variable in
    the native netCDF type (e.g., uint8 for JPEG Baseline frames, int16 or
    uint16 for MR images).
    """
    path = Path(path)
    if not path.is_file():
        raise FileNotFoundError(path)

    libnc = _load_library("libnetcdf.so.22")
    libdicom = _load_library("libncdicom.so")

    libnc.nc_open.argtypes = [c_char_p, c_int, ctypes.POINTER(c_int)]
    libnc.nc_open.restype = c_int

    _ensure_dicom_handler(libdicom)

    ncid = c_int()
    _check(libnc, libnc.nc_open(str(path).encode("utf-8"), NC_UDF6 | NC_NOWRITE, ctypes.byref(ncid)))
    try:
        varid = c_int()
        _check(libnc, libnc.nc_inq_varid(ncid.value, b"pixel_data", ctypes.byref(varid)))

        name = ctypes.create_string_buffer(NC_MAX_NAME)
        xtype = c_int()
        ndims = c_int()
        dimids = (c_int * 3)()
        natts = c_int()
        libnc.nc_inq_var.argtypes = [
            c_int, c_int, c_char_p, ctypes.POINTER(c_int),
            ctypes.POINTER(c_int), ctypes.POINTER(c_int), ctypes.POINTER(c_int),
        ]
        libnc.nc_inq_var.restype = c_int
        _check(
            libnc,
            libnc.nc_inq_var(
                ncid.value, varid.value, name, ctypes.byref(xtype),
                ctypes.byref(ndims), dimids, ctypes.byref(natts),
            ),
        )

        shape = []
        libnc.nc_inq_dimlen.argtypes = [c_int, c_int, ctypes.POINTER(c_size_t)]
        libnc.nc_inq_dimlen.restype = c_int
        for i in range(ndims.value):
            dimlen = c_size_t()
            _check(libnc, libnc.nc_inq_dimlen(ncid.value, dimids[i], ctypes.byref(dimlen)))
            shape.append(dimlen.value)

        func, dtype = _get_var_function(libnc, xtype.value)
        data = np.empty(shape, dtype=dtype, order="C")
        func.argtypes = [c_int, c_int, c_void_p]
        func.restype = c_int
        _check(libnc, func(ncid.value, varid.value, data.ctypes.data_as(c_void_p)))
        return data
    finally:
        libnc.nc_close(ncid.value)
