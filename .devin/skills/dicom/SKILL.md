---
name: dicom
description: Understanding DICOM (Digital Imaging and Communications in Medicine) file format, data elements, transfer syntaxes, pixel data encoding, and how to implement a read-only DICOM UDF handler for NEP.
metadata:
  author: netcdf-analysis
  version: "1.0"
  date: "2026-07-25"
---

# DICOM Skill

This skill covers the DICOM file format and the design of a read-only DICOM reader plugin for NEP that exposes DICOM image data through the NetCDF API.

## Overview

**DICOM** (Digital Imaging and Communications in Medicine) is the international standard for medical images and related information (ISO 12052). The standard is maintained by NEMA and published as the **DICOM PS3.x** documents at https://dicom.nema.org/standard/current/.

For NEP, DICOM is read-only. The reader opens a DICOM file, extracts metadata attributes, and presents the image pixel data as NetCDF variables with dimensions such as frame, row, column, and sample.

## File Format (DICOM PS3.10)

A DICOM file is a single **SOP Instance** written to a Part 10 file:

```
+-------------------------+  byte offset 0
| 128-byte File Preamble  |
+-------------------------+  byte offset 128
| 4-byte prefix "DICM"    |
+-------------------------+
| File Meta Information   |  Group 0x0002, Explicit VR Little Endian
|   (data elements)       |
+-------------------------+
| Data Set                |  Main DICOM object, encoding determined by Transfer Syntax UID
|   (data elements)       |
+-------------------------+
```

- **Preamble**: 128 bytes, reserved for media storage compatibility. Often all zeros.
- **Prefix**: exactly the ASCII string `DICM`. Some non-conforming files omit both preamble and prefix.
- **File Meta Information**: encoded with **Explicit VR Little Endian** (UID `1.2.840.10008.1.2.1`) regardless of the Transfer Syntax used for the Data Set.
- **Data Set**: one occurrence of one SOP Instance. Encoding is controlled by the **Transfer Syntax UID** tag `(0002,0010)`.

## Data Elements (DICOM PS3.5)

A Data Set is an ordered sequence of **Data Elements**. Each element has one of three structures:

### Explicit VR (with short length)

```
+--------+------+------+------+-------+
|  Tag   |  VR  | Length| Value        |
| 4 bytes| 2 b  | 2 b   | length bytes |
+--------+------+------+-------+------+
```

### Explicit VR (with long length)

```
+--------+------+------+------+-------+
|  Tag   |  VR  | 0x0000| Length| Value |
| 4 bytes| 2 b  | 2 b   | 4 b   | ...   |
+--------+------+------+------+-------+
```

### Implicit VR

```
+--------+------+-------+
|  Tag   | Length| Value |
| 4 bytes| 4 b   | ...   |
+--------+------+-------+
```

- **Tag**: two little-endian `uint16` values: `(gggg,eeee)` where `gggg` is the group number and `eeee` is the element number. Standard tags have even group numbers; private tags have odd group numbers.
- **VR**: two-byte **Value Representation** code. Required in Explicit VR; absent in Implicit VR.
- **Length**: number of bytes in the Value Field. May be `0xFFFFFFFF` to mean undefined length (used for SQ sequences, some OB/OW pixel data, and UN).
- **Value**: the element payload. Length is always even (padded if necessary).

## Transfer Syntax

The Transfer Syntax UID in `(0002,0010)` determines how the Data Set is encoded:

| UID | Name | VR | Endian | Notes |
|-----|------|----|--------|-------|
| `1.2.840.10008.1.2` | Implicit VR Little Endian | Implicit | LE | Default. Cannot use OB for Pixel Data. |
| `1.2.840.10008.1.2.1` | Explicit VR Little Endian | Explicit | LE | Required for File Meta Information. |
| `1.2.840.10008.1.2.2` | Explicit VR Big Endian | Explicit | BE | Retired. |
| `1.2.840.10008.1.2.4.50` | JPEG Baseline 8-bit | Explicit | LE | Encapsulated. |
| `1.2.840.10008.1.2.4.70` | JPEG Lossless | Explicit | LE | Encapsulated. |
| `1.2.840.10008.1.2.4.90` | JPEG 2000 Lossless | Explicit | LE | Encapsulated. |
| `1.2.840.10008.1.2.4.91` | JPEG 2000 | Explicit | LE | Encapsulated. |
| `1.2.840.10008.1.2.5` | RLE Lossless | Explicit | LE | Encapsulated. |

- **Implicit VR**: the VR is inferred from the tag using the data dictionary (PS3.6).
- **Encapsulated pixel data**: stored as a sequence of fragments inside the Pixel Data element; each fragment is an Item with tag `(FFFE,E000)`. The first fragment is the optional Basic Offset Table.

## Value Representations

Common VRs relevant to image reading:

| VR | Meaning | C type | Notes |
|----|---------|--------|-------|
| AE | Application Entity | string | |
| AS | Age String | string | |
| AT | Attribute Tag | two `uint16` | |
| CS | Code String | string | |
| DA | Date | string | `YYYYMMDD` |
| DS | Decimal String | string | numeric values as ASCII |
| DT | Date Time | string | |
| FL | Floating Point Single | `float` | 4 bytes |
| FD | Floating Point Double | `double` | 8 bytes |
| IS | Integer String | string | integer as ASCII |
| LO | Long String | string | |
| LT | Long Text | string | |
| OB | Other Byte String | bytes | Used for encapsulated pixel data. |
| OD | Other Double String | `double[]` | |
| OF | Other Float String | `float[]` | |
| OW | Other Word String | `uint16[]` | Native pixel data when Bits Allocated > 8. |
| PN | Person Name | string | |
| SH | Short String | string | |
| SL | Signed Long | `int32` | |
| SQ | Sequence of Items | nested data set | Used for nested objects. |
| SS | Signed Short | `int16` | |
| ST | Short Text | string | |
| TM | Time | string | |
| UC | Unlimited Characters | string | |
| UI | Unique Identifier | string | UIDs end with an even-length null padding byte. |
| UL | Unsigned Long | `uint32` | |
| UN | Unknown | bytes | |
| US | Unsigned Short | `uint16` | |
| UT | Unlimited Text | string | |

## Sequences and Items

A Data Element with VR **SQ** contains zero or more **Items**. Each item has the structure:

```
Item tag (FFFE,E000) + Item Length + Data Set
```

An item with undefined length is terminated by an Item Delimitation tag `(FFFE,E00D)` with length `0x00000000`. A sequence with undefined length is terminated by a Sequence Delimitation tag `(FFFE,E0DD)` with length `0x00000000`.

## Key Tags for a DICOM Image Reader

### Patient / Study / Series / Instance

| Tag | Keyword | Purpose |
|-----|---------|---------|
| `(0010,0010)` | PatientName | Patient name |
| `(0010,0020)` | PatientID | Patient identifier |
| `(0020,000D)` | StudyInstanceUID | Study UID |
| `(0020,000E)` | SeriesInstanceUID | Series UID |
| `(0008,0016)` | SOPClassUID | SOP Class UID (in File Meta) |
| `(0008,0018)` | SOPInstanceUID | SOP Instance UID (in File Meta) |

### Image Pixel Module

| Tag | Keyword | Purpose |
|-----|---------|---------|
| `(0028,0002)` | SamplesPerPixel | Number of color planes (1 or 3 for standard images) |
| `(0028,0004)` | PhotometricInterpretation | `MONOCHROME1`, `MONOCHROME2`, `RGB`, `YBR_FULL`, etc. |
| `(0028,0010)` | Rows | Number of rows in each frame |
| `(0028,0011)` | Columns | Number of columns in each frame |
| `(0028,0030)` | PixelSpacing | Physical spacing between rows and columns |
| `(0028,0100)` | BitsAllocated | Bits allocated per sample (8, 16, 32, 64) |
| `(0028,0101)` | BitsStored | Number of significant bits |
| `(0028,0102)` | HighBit | Zero-based index of high bit |
| `(0028,0103)` | PixelRepresentation | `0` = unsigned, `1` = signed |
| `(0028,0106)` | SmallestImagePixelValue | Min pixel value (optional) |
| `(0028,0107)` | LargestImagePixelValue | Max pixel value (optional) |
| `(0028,1052)` | RescaleIntercept | `m` in `output = m + x * slope` |
| `(0028,1053)` | RescaleSlope | `slope` in rescale formula |
| `(0028,1054)` | RescaleType | Unit of rescaled values |
| `(0028,0006)` | PlanarConfiguration | For RGB: `0` = color-by-pixel, `1` = color-by-plane |
| `(0028,0008)` | NumberOfFrames | Number of frames (>1 for multi-frame images) |
| `(7FE0,0010)` | PixelData | The image pixel data |

### Multi-frame Functional Groups

Modern multi-frame objects (CT/MR Enhanced, etc.) store per-frame information in the **Shared Functional Groups Sequence** `(5200,9229)` and **Per-frame Functional Groups Sequence** `(5200,9230)`. Important nested tags include:

| Tag | Keyword | Purpose |
|-----|---------|---------|
| `(0020,0032)` | ImagePositionPatient | 3D position of the first voxel |
| `(0020,0037)` | ImageOrientationPatient | 6-value direction cosine |
| `(0018,0050)` | SliceThickness | Nominal slice thickness |
| `(0018,0088)` | SpacingBetweenSlices | Spacing between slices |

## Pixel Data Encoding

### Native (uncompressed)

- Stored in tag `(7FE0,0010)`.
- **BitsAllocated <= 8**: VR may be `OB` (Explicit VR) or `OW` (Implicit VR uses only `OW`).
- **BitsAllocated > 8**: VR is `OW`.
- Layout for a single-frame grayscale image: row-major, rows × columns.
- Layout with SamplesPerPixel > 1 depends on **PlanarConfiguration**:
  - `0`: interleaved per pixel (`RGBRGBRGB...`).
  - `1`: separate color planes (all R, then all G, then all B).
- The actual sample type (signed/unsigned) is given by **PixelRepresentation**.

### Encapsulated (compressed)

- VR is always `OB`.
- Pixel Data length is `0xFFFFFFFF` (undefined length) in top-level Data Set.
- Contents are a sequence of Items:
  - Item 0: Basic Offset Table (may be empty, zero length).
  - Items 1..N: one compressed fragment per frame (usually one fragment per frame, but not required).
- Sequence is terminated by `(FFFE,E0DD)`.
- Decoding requires the codec indicated by the Transfer Syntax UID. For an initial NEP reader, supporting only native transfer syntaxes is a reasonable scope.

## Mapping DICOM to the NetCDF Model

A minimal mapping for NEP:

- **Dimensions**:
  - `frame` = max(1, NumberOfFrames)
  - `row` = Rows
  - `column` = Columns
  - `sample` = SamplesPerPixel (omit when equal to 1)
- **Variable**:
  - `pixel_data` with shape `[frame][row][column]` or `[frame][sample][row][column]` depending on PlanarConfiguration.
  - NetCDF type derived from BitsAllocated and PixelRepresentation:

| BitsAllocated | PixelRepresentation | NetCDF type |
|---------------|---------------------|-------------|
| 8 | 0 | `NC_UBYTE` |
| 8 | 1 | `NC_BYTE` |
| 16 | 0 | `NC_USHORT` |
| 16 | 1 | `NC_SHORT` |
| 32 | 0 | `NC_UINT` |
| 32 | 1 | `NC_INT` |
| 64 | 0 | `NC_UINT64` |
| 64 | 1 | `NC_INT64` |

- **Global attributes** (strings): PatientName, PatientID, StudyInstanceUID, SeriesInstanceUID, SOPInstanceUID, PhotometricInterpretation, Modality, TransferSyntaxUID, RescaleIntercept, RescaleSlope, etc.
- **Variable attributes** on `pixel_data`: SamplesPerPixel, BitsAllocated, BitsStored, HighBit, PixelRepresentation, PlanarConfiguration, PixelSpacing.

For multi-frame functional-group data, each per-frame tag can become a one-dimensional variable over the `frame` dimension, or additional global attributes if a Shared Functional Group applies to all frames.

## Implementation Options

### 1. DCMTK (recommended if dependency is acceptable)

- C++ library with a stable C interface through `dcmdata` module.
- `DcmFileFormat` loads the whole file.
- `DcmDataset` provides `findAndGetOFStringArray`, `findAndGetUint16`, `findAndGetSint16`, `getUint8Array`, etc.
- Handles Explicit/Implicit VR, endianness, sequences, and encapsulated data parsing.
- Homepage: https://dicom.offis.de/dcmtk.php.en
- License: BSD-style.

### 2. GDCM (Grassroots DICOM)

- C++ library, read-only image reader is straightforward.
- `gdcm::ImageReader` returns `gdcm::Image` with accessors for dimensions, spacing, origin, and buffer.
- Homepage: https://gdcm.sourceforge.net/
- License: BSD.

### 3. Custom Parser

- Implement a low-level DICOM data element parser.
- Must handle:
  - Preamble + prefix detection (with fallback for non-standard files).
  - Explicit and Implicit VR.
  - Little- and Big-Endian byte order.
  - Sequence/item parsing including undefined lengths.
  - Data dictionary lookup for Implicit VR (PS3.6).
- This avoids a heavy external dependency but is significantly more work.

## NEP-Specific Guidance

### UDF Slot

DICOM should occupy the next free UDF slot. Based on current NEP allocations:

| Slot | Format |
|------|--------|
| UDF0 | GeoTIFF BigTIFF |
| UDF1 | GeoTIFF standard TIFF |
| UDF2 | GRIB2 |
| UDF3 | FITS |
| UDF4 | NASA CDF |
| UDF5 | PDS4 |
| **UDF6** | **DICOM** |

Add to `include/nep.h`:

```c
/** DICOM medical image format uses UDF6 slot */
#define NEP_UDF_DICOM NC_UDF6

/** DICOM magic number: "DICM" at byte offset 128 */
#define NEP_MAGIC_DICOM "DICM"

/** DICOM format display name */
#define NEP_FORMAT_NAME_DICOM "DICOM"
```

**Important**: the magic number check in NetCDF-C uses the first bytes of the file. Because `DICM` is at offset 128, the NEP reader or the dispatch detection logic must account for this. If NetCDF-C does not support offset magic numbers, the reader may need to be opened explicitly via the UDF mode flag or by enhancing format detection.

### Initialization

Follow the FITS pattern in `src/fitsdispatch.c`:

```c
static const NC_Dispatch DICOM_dispatcher = {
    NC_FORMATX_UDF6,
    NC_DISPATCH_VERSION,
    NC_RO_create,
    NC_DICOM_open,
    /* ... remaining read-only dispatch functions ... */
};

NC_Dispatch *
NC_DICOM_initialize(void)
{
    DICOM_dispatch_table = &DICOM_dispatcher;
    nc_def_user_format(NEP_UDF_DICOM | NC_NETCDF4,
                       (NC_Dispatch *)DICOM_dispatch_table,
                       NEP_MAGIC_DICOM);
    return (NC_Dispatch *)&DICOM_dispatcher;
}
```

### Build Options

Add to `CMakeLists.txt` and `configure.ac`:

| Build system | Option |
|--------------|--------|
| CMake | `-DNEP_ENABLE_DICOM=ON/OFF` (default OFF) |
| Autotools | `--enable-dicom` / `--disable-dicom` |

- Detect DCMTK or GDCM library and headers.
- Define `HAVE_DICOM` preprocessor macro.
- Add `src/dicomdispatch.c`, `src/dicomfile.c`, and any header files.
- Add a C test in `test/tst_dicom_udf.c` and optionally a Fortran test in `ftest/ftst_dicom_udf.F90`.

### Tests

- Use a small, freely redistributable DICOM file for testing (e.g., from the `pydicom` test suite or a public sample).
- Test cases:
  - Open a DICOM file through `nc_open` after calling `NC_DICOM_initialize()`.
  - Verify dimension lengths match Rows/Columns/NumberOfFrames.
  - Read a subset of `pixel_data` and compare against expected values.
  - Verify global attributes such as PatientName, Modality, and PhotometricInterpretation.

## Important Caveats

1. **Magic offset**: `DICM` appears at byte 128, not at offset 0. If format detection relies solely on the first bytes, the prefix will be missed. Some DICOM files also omit the preamble and prefix entirely.
2. **Implicit VR dictionary**: a custom parser needs the PS3.6 data dictionary to determine VRs for implicit transfer syntaxes.
3. **Encapsulated data**: compressed transfer syntaxes require codec support. An initial reader can reject or skip encapsulated files with `NC_EINVAL` or `NC_EINVALCOORDS`.
4. **Multi-frame and 3D data**: many DICOM objects (CT/MR Enhanced, ultrasound, etc.) use multi-frame Functional Groups. A simple reader can flatten these to `frame` × `row` × `column`, but advanced use cases need per-frame geometric attributes.
5. **Signed vs unsigned**: BitsAllocated and PixelRepresentation together determine the pixel type. Do not assume unsigned bytes.

## References

- DICOM Standard current edition: https://dicom.nema.org/standard/current/
- Part 5 — Data Structure and Encoding: https://dicom.nema.org/medical/dicom/current/output/chtml/part05/
- Part 10 — Media Storage and File Format: https://dicom.nema.org/medical/dicom/current/output/chtml/part10/
- DCMTK: https://dicom.offis.de/dcmtk.php.en
- GDCM: https://gdcm.sourceforge.net/

## When to Use This Skill

Use this skill when:
- Adding a DICOM UDF handler to NEP.
- Mapping DICOM attributes to NetCDF dimensions, variables, and attributes.
- Choosing between DCMTK, GDCM, or a custom parser for the implementation.
- Designing tests and build-system options for the DICOM reader.
- Debugging DICOM byte-order, VR, transfer-syntax, or pixel-layout issues.
