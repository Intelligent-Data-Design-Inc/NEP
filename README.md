# NFEP
NetCDF4 / HDF5 Format Extension Pack

Problem: NASA's Earth Science missions generate unprecedented volumes of observational data in diverse formats. These datasets must be integrated with simulation outputs and analyzed across heterogeneous computing environments, including exascale environments. Existing approaches of data ingest are not efficient and maintainable since they either require data conversion, or separate processing pipeline for each format require, or specialized custom solution. 

Solution: We propose enhancements to create a unified framework where the benefits of specialized formats can be retained while simultaneously enabling interoperability with the broader scientific data ecosystem through netCDF4 and HDF5 interfaces. The NetCDF4/HDF5 Format Expansion Pack (NFEP) enables access to data in non-HDF5 file formats via existing HDF5 interface and run-time pluggable capabilities. HDF5 automatically detects file in a NASA’s format at read time, loads an appropriate file format connector thus making access to data completely transparent to the user. 

Significance: NFEP represents a significant advancement beyond current approaches by combining the preservation of format-specific optimizations with standardized access patterns, and compatibility with existing netCDF4 and HDF5 codes. This innovative approach will dramatically improve NASA's ability to efficiently manage and analyze diverse Earth Science datasets while reducing storage requirements and computational overhead. It will allow NASA programmers to use existing science codes on GRIB2, BUFR, CDF, and GeoTIFF datasets.

---

### Visual Overview

![NFEP with VOLs for HDF5 Users](docs/images/NFEP%20with%20VOLs%20for%20HDF5%20Users.png)
*Figure 1: NFEP with VOLs for HDF5 Users*

![NFEP with VOLs for NetCDF Users](docs/images/NFEP%20with%20VOLs%20for%20NetCDF%20Users.png)
*Figure 2: NFEP with VOLs for NetCDF Users*

---
