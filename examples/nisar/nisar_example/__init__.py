"""nisar_example: open a NISAR Level 3 Soil Moisture (SME2) product and plot it.

SME2 products are HDF5 files structured to be CF/netCDF-compliant. This
package is a standalone NEP example ported from the ``nisar_play`` project
and focuses on reading the soil moisture grid and the granule's lat/lon
footprint with ``xarray``/``netCDF4`` and plotting them with ``matplotlib``
and ``cartopy``. See ``examples/nisar/README.md`` for usage.
"""

__version__ = "0.1.0"
