# DICOM Format Reader

DICOM (Digital Imaging and Communications in Medicine) is the standard format for medical imaging objects such as CT, MR, XA, and ultrasound.  NEP exposes single-frame and multi-frame DICOM images through the standard NetCDF API via a User-Defined Format (UDF) handler.

**Transparent Access**: Open DICOM files with `nc_open()` and read metadata, dimensions, and pixel data using standard NetCDF functions.

**Dimension Mapping**:
- Frame → NetCDF `frame` dimension (`NumberOfFrames`)
- Rows → NetCDF `row` dimension (`Rows`)
- Columns → NetCDF `column` dimension (`Columns`)
- Samples → NetCDF `sample` dimension (`SamplesPerPixel`, if > 1)

**Data Variable**: `pixel_data` with a type determined by `BitsAllocated` and `PixelRepresentation`:
- 8-bit unsigned → `NC_UBYTE`
- 8-bit signed → `NC_BYTE`
- 16-bit unsigned → `NC_USHORT`
- 16-bit signed → `NC_SHORT`
- 32-bit unsigned → `NC_UINT`
- 32-bit signed → `NC_INT`

**Global Attributes**: `PatientName`, `PatientID`, `Modality`, `PhotometricInterpretation`, `TransferSyntaxUID`, `StudyInstanceUID`, `SeriesInstanceUID`, `SOPClassUID`, `SOPInstanceUID`.

**Variable Attributes**: `NumberOfFrames`, `SamplesPerPixel`, `BitsAllocated`, `BitsStored`, `HighBit`, `PixelRepresentation`, `PlanarConfiguration`.

**Use Cases**: Medical imaging analysis, radiology research, clinical image processing, multi-frame cine loops.

**Enabling:**
```bash
cmake -B build -DNEP_ENABLE_DICOM=ON   # CMake
./configure --enable-dicom             # Autotools
```

**Dependencies**: libdicom, libjpeg or libjpeg-turbo.

**Resources**: [libdicom on GitHub](https://github.com/ImagingDataCommons/libdicom) · [DICOM Standard](https://www.dicomstandard.org/)

**Example:**
```c
nc_open("MRBRAIN.DCM", NC_UDF6, &ncid);
nc_inq_varid(ncid, "pixel_data", &varid);
nc_get_vara_ushort(ncid, varid, start, count, buf);
nc_close(ncid);
```

## Visualization

NEP includes Python visualization examples in `examples/viz/` that open DICOM files through the NetCDF UDF interface and write publication-ready PNGs.

- `plot_dicom_mrbrain.py` — plots the single-frame 512×512 MR image from `test/data/DICOM/MRBRAIN.DCM`, normalizing the 16-bit `pixel_data` to 8-bit grayscale.
- `plot_dicom_xa_montage.py` — plots a compact montage of all 17 frames from the encapsulated JPEG Baseline file `test/data/DICOM/0003.DCM`.

Enable the examples with:

```bash
# CMake
cmake -B build -DNEP_BUILD_EXAMPLES=ON -DNEP_ENABLE_VIZ_EXAMPLES=ON -DNEP_ENABLE_DICOM=ON

# Autotools
./configure --enable-examples --enable-viz-examples --enable-dicom
```

Run only the DICOM visualizations with `ctest -R viz_dicom --output-on-failure` (CMake) or `make check` (Autotools). Generated artifacts are `dicom_mrbrain_image.png` + `_metadata.txt` and `dicom_xa_frame_montage.png` + `_metadata.txt` in the visualization build directory.

Because DICOM magic is at byte offset 128, `netCDF4.Dataset` cannot pass the `NC_UDF6` mode flag required for direct open. The scripts load `libncdicom.so` and call `NC_DICOM_initialize()` via `examples/viz/_dicom_udf.py`, then read `pixel_data` through the NetCDF-C UDF API. Make sure `LD_LIBRARY_PATH` includes the directory containing `libncdicom.so` (the build systems set this automatically).

DICOM files are read-only and are opened via UDF slot 6 (`NC_UDF6`).
