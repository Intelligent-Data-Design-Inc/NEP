# The NetCDF Expansion Pack (NEP) - Extending the Capabilities of NetCDF

Edward Hartnett, Intelligent Data Design
2026-07-17

# Abstract

# Introduction

The netCDF C library (netcdf-c) has become the basis for many scientific and operational workflows. The netcdf-c library is maintained at NSF Unidata/UCAR and is free and open source.

The NetCDF Expansion Pack (NEP) expands the capabilities of the netCDF C library, adding additional formats and compression filters. It also includes a complete set of netCDF C/Fortran example programs, demonstrating netCDF features like remote access, compression with quantization, and ncZarr cloud access.

The NEP is free and open source, maitained by the author at Intelligent Data Design, Inc.

# Getting the NetCDF Expansion Pack

The current version of the NetCDF Expansion Pack is 2.7.1 (July, 2026). It requires the most recently released netcdf-c (4.10.1) and any recent netcdf-fortran.

## Spack

The NEP is available as spack package "nep".

## Conda

The NEP is available on condaforge as "nep".

## Building from Source

Complete and up-to-date instructions for building from source can be found in the README of the NetCDF Expansion Pack. The NEP depends on netcdf-c, and, optionally, netcdf-fortran.

# Compression

## LZ4 - Faster than Zstd

## BZIP2 - Slow but Compressive

# Reading Other Formats with Existing NetCDF Applications

## GRIB2

## GeoTIFF

## NASA CDF

## FITS

## NASA/ESA PDS4

# Example Programs

# Future Plans

# Summary

