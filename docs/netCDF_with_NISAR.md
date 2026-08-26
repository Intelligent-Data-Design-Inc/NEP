# netCDF with NISAR: Finding Soil Moisture Data

*By Edward Hartnett, Intelligent Data Design, Inc.*  
*26 August 2026*

NISAR (the NASA-ISRO Synthetic Aperture Radar, or SAR, mission) is a
joint mission of NASA (the National Aeronautics and Space
Administration) and ISRO (the Indian Space Research Organisation). Its
standard data products are Hierarchical Data Format version 5 (HDF5)
files, but they are built to be Climate and Forecast (CF) 1.7 compliant,
which means every NISAR product is also a valid Network Common Data Form
(netCDF) version 4 file. You can open one with `nc_open()`, `ncdump`,
`netCDF4`, or `xarray`, exactly as you would any other netCDF-4 dataset.
You can also open the same file with plain HDF5 tools, because netCDF-4
is HDF5 underneath.

This document walks through a real NISAR Level 3 Soil Moisture EASE
(Equal-Area Scalable Earth) Grid 2.0 (SME2) granule two ways:
through the netCDF-4 view (`ncdump`) and through the
native HDF5 view (`h5dump`). The goal is to show exactly where the soil
moisture data lives and how the two views relate. The tools used here are
built from `/usr/local/netcdf-c` and `/usr/local/hdf5-2.1.1`:

```bash
export LD_LIBRARY_PATH=/usr/local/netcdf-c/lib:/usr/local/hdf5-2.1.1/lib:$LD_LIBRARY_PATH
NCDUMP=/usr/local/netcdf-c/bin/ncdump
H5DUMP=/usr/local/hdf5-2.1.1/bin/h5dump
```

NEP (the NetCDF Expansion Pack) is a superset of netCDF-C that adds
compression filters and read layers for scientific data formats netCDF
does not natively understand. NISAR SME2 files need none of that: they
are already CF-1.7 netCDF-4 files, so a plain `nc_open()` or `ncdump`
opens them directly. NEP still ships a working, standalone Python
example that opens an SME2 file and plots the soil moisture grid, in
`examples/nisar/`; see `examples/nisar/README.md`. Code from that
example is used throughout this document.

---

## How NISAR Products Map to netCDF-4

An HDF5 file is a hierarchy of groups and datasets. A netCDF-4 file is
the same thing, with netCDF concepts layered on top: HDF5 groups become
netCDF groups, HDF5 datasets become netCDF variables, and HDF5
dimension-scale datasets become netCDF dimensions. NetCDF-4 did not
invent a new storage format. It defined a convention for using HDF5's
existing features, and NISAR's Science Data System (SDS) follows that
convention.

That means the same file, opened two different ways, describes itself
in two different vocabularies. `ncdump` shows dimensions, variables, and
attributes. `h5dump` shows groups, datasets, and HDF5 attributes,
including the `CLASS` and `DIMENSION_LIST` attributes netCDF-4 uses
internally to implement shared dimensions. Neither view is more correct.
They are the same bytes, described at different layers.

An SME2 granule's group structure, from the root down through the
L-band SAR (LSAR) group to the soil moisture grid, looks like this:

```
/
  science/
    LSAR/
      SME2/
        grids/              <- soil moisture lives here
        metadata/
      identification/
```

---

## The netCDF-4 View: `ncdump -h -s`

`ncdump -h` prints header metadata only (dimensions, variables,
attributes) without reading any data. The `-s` flag adds the special
virtual attributes (`_Storage`, `_ChunkSizes`, `_DeflateLevel`,
`_Endianness`, `_NoFill`) that expose HDF5-level storage properties
through the netCDF API. Run against a real SME2 product:

```bash
$NCDUMP -h -s NISAR_L3_PR_SME2_028_005_A_020_4005_DHDH_A_20260813T125218_20260813T125253_P05023_N_F_J_001.h5
```

The file opens as one netCDF-4 dataset with a single, short set of
global attributes identifying the mission and the CF convention:

```
netcdf NISAR_L3_PR_SME2_028_005_A_020_4005_DHDH_A_20260813T125218_20260813T125253_P05023_N_F_J_001 {

// global attributes:
                :Conventions = "CF-1.7" ;
                :contact = "nisar-sds-ops@jpl.nasa.gov" ;
                :institution = "NASA JPL" ;
                :mission_name = "NISAR" ;
                :title = "NISAR L3 SME2 Product" ;
                :reference_document = "D-107677 NISAR NASA SDS Product Specification Level-3 Soil Moisture" ;
                :_SuperblockVersion = 2 ;
                :_IsNetcdf4 = 1 ;
                :_Format = "netCDF-4" ;

group: science {
  group: LSAR {
    group: SME2 {
                :long_name = "200m soil moisture at EASE-Grid 2.0 projection" ;

      group: grids {
        dimensions:
                xCoordinates = 2025 ;
                yCoordinates = 1665 ;
        variables:
                ...
```

Three of those global attributes matter more than the rest:
`Conventions = "CF-1.7"` says the file follows CF metadata conventions,
`_IsNetcdf4 = 1` confirms the file uses the enhanced netCDF-4 data model
(as opposed to a plain HDF5 file with no netCDF markers), and
`title = "NISAR L3 SME2 Product"` names the product. Everything below
`group: science` is NISAR's own hierarchy, not something netCDF imposes.

The interesting twist is that this file was not produced with netCDF-4, but the netcdf-c library still identifies this as a netCDF-4 file. The reason is that netcdf-c does not really know whether the file was created with netcdf-c, it simply looks for artifacts that match the netcdf-c way of creating a HDF5 file.

In the case of the NISAR file, the careful programmers at ISRO used the HDF5 dimension scales feature to encode the dimension information. HDF5 does not contain any native way of marking dimensions as shared between datasets. In HDF5, two datasets may have the same shape (i.e. the same size along every dimension) but there is no easy way to specify that the dimensions are shared.

To support the netCDF-4 project, the HDF5 team were required to add some form of shared dimensions to HDF5. The (unsatisfactory) result was dimension scales. These are awkward, slow, and hard to work with. Kudos to the ISRO programmers for figuring them out.

Because of this extra effort by these diligent programmers, the NISAR file will open as a netCDF file as readily as it will open in HDF5.

### The `grids` Group

`/science/LSAR/SME2/grids` is where the soil moisture data lives. It has
two dimensions, `xCoordinates` (2025) and `yCoordinates` (1665), which
size the EASE-Grid 2.0 projected grid, and a flat list of 2-D variables
sharing those dimensions:

```
      group: grids {
        dimensions:
                xCoordinates = 2025 ;
                yCoordinates = 1665 ;
        variables:
                int EASEGridColumnIndex(xCoordinates) ;
                int EASEGridRowIndex(yCoordinates) ;
                float latitude(yCoordinates) ;
                float longitude(xCoordinates) ;
                uint projection ;
                        projection:grid_mapping_name = "lambert_cylindrical_equal_area" ;
                        projection:epsg_code = 6933 ;
                short retrievalQualityFlag(yCoordinates, xCoordinates) ;
                        retrievalQualityFlag:description = "Soil moisture retrieval quality flag. The least significant bit indicates whether the retrieval is recommended (0 = recommended, 1 = not recommended). ..." ;
                float soilMoisture(yCoordinates, xCoordinates) ;
                        soilMoisture:long_name = "Soil moisture" ;
                        soilMoisture:units = "meter^3/meter^3" ;
                        soilMoisture:_FillValue = -9999.f ;
                        soilMoisture:grid_mapping = "projection" ;
                        soilMoisture:min_value = 0.02f ;
                        soilMoisture:max_value = 0.5692517f ;
                        soilMoisture:mean_value = 0.1122438f ;
                        soilMoisture:_Storage = "chunked" ;
                        soilMoisture:_ChunkSizes = 512, 512 ;
                        soilMoisture:_DeflateLevel = 1 ;
                float soilMoistureUncertainty(yCoordinates, xCoordinates) ;
                short surfaceQualityFlag(yCoordinates, xCoordinates) ;
                double xCoordinates(xCoordinates) ;
                double yCoordinates(yCoordinates) ;
      }
```

`soilMoisture(yCoordinates, xCoordinates)` is the variable to read. Its
`_FillValue` of -9999 marks missing pixels, its `grid_mapping` attribute
points at the `projection` variable for the EPSG (European Petroleum
Survey Group) code 6933 (EASE-Grid 2.0), and
its `_Storage`/`_ChunkSizes`/`_DeflateLevel` attributes say the data is
stored as 512x512 chunks compressed with DEFLATE level 1.

`grids` also contains three subgroups —
`algorithmCandidates` (per-algorithm soil moisture from the candidate
retrieval algorithms before they are combined into the top-level
estimate), `ancillaryData` (land cover, incidence angle, waterbody
fraction), and `radarData` (per-frequency backscatter inputs).

### The `identification` Group

`/science/LSAR/identification` carries scalar metadata about the
granule: acquisition geometry, processing provenance, and product
identifiers. Reading a handful of those variables from the sample
granule gives:

```
frameNumber = 20 ;
granuleId = "NISAR_L3_PR_SME2_028_005_A_020_4005_DHDH_A_20260813T125218_20260813T125253_P05023_N_F_J_001" ;
lookDirection = "Left" ;
orbitPassDirection = "Ascending" ;
productType = "SME2" ;
trackNumber = 5 ;
```

Note the `lookDirection`/`orbitPassDirection`, provided we need to know about the swath geometry.

---

## The HDF5 View: `h5dump`

`h5dump -n` lists every group and dataset path in the file without
printing any data or attribute detail, which is the fastest way to see
the raw HDF5 layout:

```bash
$H5DUMP -n NISAR_L3_PR_SME2_028_005_A_020_4005_DHDH_A_20260813T125218_20260813T125253_P05023_N_F_J_001.h5
```

```
FILE_CONTENTS {
 group      /
 group      /science
 group      /science/LSAR
 group      /science/LSAR/SME2
 group      /science/LSAR/SME2/grids
 dataset    /science/LSAR/SME2/grids/EASEGridColumnIndex
 dataset    /science/LSAR/SME2/grids/EASEGridRowIndex
 group      /science/LSAR/SME2/grids/algorithmCandidates
 group      /science/LSAR/SME2/grids/algorithmCandidates/DSG
 ...
 dataset    /science/LSAR/SME2/grids/latitude
 dataset    /science/LSAR/SME2/grids/longitude
 dataset    /science/LSAR/SME2/grids/soilMoisture
 dataset    /science/LSAR/SME2/grids/soilMoistureUncertainty
 ...
}
```

Every netCDF path from the `ncdump` output above has a matching HDF5
path here.

### The `soilMoisture` Dataset, at the HDF5 Level

`h5dump -H -A -d` prints a dataset's header and attributes without its
data:

```bash
$H5DUMP -H -A -d /science/LSAR/SME2/grids/soilMoisture NISAR_L3_PR_SME2_028_005_A_020_4005_DHDH_A_20260813T125218_20260813T125253_P05023_N_F_J_001.h5
```

```
DATASET "/science/LSAR/SME2/grids/soilMoisture" {
   DATATYPE  H5T_IEEE_F32LE
   DATASPACE  SIMPLE { ( 1665, 2025 ) / ( 1665, 2025 ) }
   ATTRIBUTE "DIMENSION_LIST" {
      DATATYPE  H5T_VLEN { H5T_REFERENCE { H5T_STD_REF_OBJECT } }
      DATASPACE  SIMPLE { ( 2 ) / ( 2 ) }
      DATA {
      (0): (DATASET 100227485777056 "/science/LSAR/SME2/grids/yCoordinates"),
      (1): (DATASET 100227485777872 "/science/LSAR/SME2/grids/xCoordinates")
      }
   }
   ATTRIBUTE "_FillValue" {
      DATATYPE  H5T_IEEE_F32LE
      DATA { (0): -9999 }
   }
   ATTRIBUTE "description" {
      DATA { (0): "Averaged soil moisture. If recommended retrievals are available, the average is computed using only the recommended retrievals. ..." }
   }
   ATTRIBUTE "grid_mapping" { DATA { (0): "projection" } }
   ATTRIBUTE "long_name" { DATA { (0): "Soil moisture" } }
   ATTRIBUTE "max_value" { DATA { (0): 0.569252 } }
   ATTRIBUTE "mean_value" { DATA { (0): 0.112244 } }
   ATTRIBUTE "min_value" { DATA { (0): 0.02 } }
   ATTRIBUTE "sample_stddev" { DATA { (0): 0.0443102 } }
   ATTRIBUTE "units" { DATA { (0): "meter^3/meter^3" } }
}
```

This is the mechanism behind the netCDF-4 dimension model. `DATASPACE
SIMPLE { ( 1665, 2025 ) / ( 1665, 2025 ) }` is the raw HDF5 shape;
netCDF reports the same shape as `(yCoordinates, xCoordinates)` because
of the `DIMENSION_LIST` attribute, which references the two dimension
datasets by object reference rather than by name. That reference-based
binding is what lets HDF5 groups be renamed or moved without breaking
netCDF's dimension bookkeeping.

### The `yCoordinates` Dimension, at the HDF5 Level

The other half of that binding lives on the dimension dataset itself.
`yCoordinates` carries a `CLASS = "DIMENSION_SCALE"` attribute and a
`REFERENCE_LIST` pointing back at every variable that uses it:

```
DATASET "/science/LSAR/SME2/grids/yCoordinates" {
   DATATYPE  H5T_IEEE_F64LE
   DATASPACE  SIMPLE { ( 1665 ) / ( 1665 ) }
   ATTRIBUTE "CLASS" { DATA { (0): "DIMENSION_SCALE" } }
   ATTRIBUTE "REFERENCE_LIST" {
      DATATYPE  H5T_COMPOUND {
         H5T_REFERENCE { H5T_STD_REF_OBJECT } "dataset";
         H5T_STD_U32LE "dimension";
      }
      DATASPACE  SIMPLE { ( 4 ) / ( 4 ) }
      DATA {
      (0): { DATASET "/science/LSAR/SME2/grids/surfaceQualityFlag", 0 },
      (1): { DATASET "/science/LSAR/SME2/grids/soilMoisture", 0 },
      (2): { DATASET "/science/LSAR/SME2/grids/soilMoistureUncertainty", 0 },
      (3): { DATASET "/science/LSAR/SME2/grids/retrievalQualityFlag", 0 }
      }
   }
   ATTRIBUTE "standard_name" { DATA { (0): "projection_y_coordinate" } }
   ATTRIBUTE "units" { DATA { (0): "meters" } }
}
```

`CLASS`/`DIMENSION_LIST`/`REFERENCE_LIST` are the HDF5 Dimension Scale
API's own attributes. This is where the ISRO programmers went the extra kilometer and made the shared dimensions discoverable by netCDF.

---

## Finding Soil Moisture: Two Equivalent Paths

**Through netCDF:**

```c
int ncid, grp_id, varid;
nc_open(path, NC_NOWRITE, &ncid);
nc_inq_grp_full_ncid(ncid, "/science/LSAR/SME2/grids", &grp_id);
nc_inq_varid(grp_id, "soilMoisture", &varid);
nc_get_var_float(grp_id, varid, soil_moisture);
```

**Through HDF5:**

```c
hid_t file_id, dset_id;
file_id = H5Fopen(path, H5F_ACC_RDONLY, H5P_DEFAULT);
dset_id = H5Dopen2(file_id, "/science/LSAR/SME2/grids/soilMoisture", H5P_DEFAULT);
H5Dread(dset_id, H5T_NATIVE_FLOAT, H5S_ALL, H5S_ALL, H5P_DEFAULT, soil_moisture);
```

Both calls read the same 1665x2025 array of 32-bit floats out of the
same chunked, DEFLATE-compressed dataset. The netCDF path additionally
gives you the `_FillValue`, `units`, and `grid_mapping` attributes
through a uniform API that works the same way across classic and
netCDF-4 files; the HDF5 path gives you everything in the file,
including the `DIMENSION_LIST`/`CLASS`/`REFERENCE_LIST` bookkeeping
attributes netCDF hides by design.

(Note that error handling is removed from this sample code, but you better believe I would never write netcdf-c or HDF5 code without checking the results of every function call!)

**Through NEP's `examples/nisar` example, in Python:**

NEP's `examples/nisar/nisar_example/sme2.py` opens the same `grids`
group with `xarray`'s netCDF4 engine, one function call, no group
handles or variable IDs to manage:

```python
GRIDS_GROUP = "/science/LSAR/SME2/grids"

def open_grids(path: Path) -> xr.Dataset:
    """Open the SME2 ``grids`` group of a product file.

    Fill values (``_FillValue = -9999``) are automatically masked to NaN
    by the netCDF4 engine.
    """
    return xr.open_dataset(path, engine="netcdf4", group=GRIDS_GROUP)
```

`open_dataset(..., group=GRIDS_GROUP)` does exactly what the C
`nc_inq_grp_full_ncid()`/`nc_get_var_float()` pair does above, plus
fill-value masking, for every variable in the group at once. NEP layers
one more step on top: `retrievalQualityFlag`'s least-significant bit
marks retrievals the algorithm does not recommend, so `sme2.py` masks
those out too before returning the array:

```python
#: Least significant bit of ``retrievalQualityFlag``:
#: 0 = retrieval recommended, 1 = not recommended.
NOT_RECOMMENDED_BIT = 0x1

def load_soil_moisture(path: Path, mask_quality: bool = True) -> xr.DataArray:
    """Load the soil moisture grid with lat/lon coordinates."""
    with open_grids(path) as ds:
        sm = ds["soilMoisture"].copy()
        sm = sm.assign_coords(
            latitude=ds["latitude"], longitude=ds["longitude"]
        )
        if mask_quality:
            flag = ds["retrievalQualityFlag"].values
            not_recommended = np.zeros(flag.shape, dtype=bool)
            valid_flag = np.isfinite(flag)
            not_recommended[valid_flag] = (
                flag[valid_flag].astype(np.int64) & NOT_RECOMMENDED_BIT
            ).astype(bool)
            sm = sm.where(~not_recommended)
    return sm
```

Run it from the command line:

```bash
cd examples/nisar
python -m nisar_example /path/to/NISAR_L3_..._SME2_....h5 --output-dir figures
```

which produces `figures/soil_moisture.png` — a quality-masked soil
moisture map plotted with cartopy — and `figures/footprint.png`,
the granule's lat/lon bounding box. See `examples/nisar/README.md` for
the full CLI, including fetching a granule by lat/lon box from NASA
Earthdata with `--bbox`.

---

## Command Reference

```bash
export LD_LIBRARY_PATH=/usr/local/netcdf-c/lib:/usr/local/hdf5-2.1.1/lib:$LD_LIBRARY_PATH

# Full header, with storage attributes, netCDF view
/usr/local/netcdf-c/bin/ncdump -h -s FILE.h5

# One group only
/usr/local/netcdf-c/bin/ncdump -h -s -g /science/LSAR/SME2/grids FILE.h5

# Scalar/small metadata values, one group
/usr/local/netcdf-c/bin/ncdump -v productType,granuleId,lookDirection -g /science/LSAR/identification FILE.h5

# Every group/dataset path, HDF5 view
/usr/local/hdf5-2.1.1/bin/h5dump -n FILE.h5

# One dataset's header and attributes, no data
/usr/local/hdf5-2.1.1/bin/h5dump -H -A -d /science/LSAR/SME2/grids/soilMoisture FILE.h5
```

`ncdump -h` and `h5dump -H` both skip data by design; a full `h5dump` of
`soilMoisture`'s 1665x2025 array is over 35 MB of text for a value you
almost certainly want in a NumPy array instead, not a terminal.

---

## References

- **netCDF-C**: <https://www.unidata.ucar.edu/software/netcdf/>
- **NEP (NetCDF Expansion Pack)**: <https://github.com/Intelligent-Data-Design-Inc/NEP>
- **HDF5**: <https://www.hdfgroup.org/solutions/hdf5/>
- **NISAR Mission (NASA JPL)**: <https://nisar.jpl.nasa.gov/>
- **NISAR Mission (ISRO)**: <https://www.isro.gov.in/NISAR.html>
