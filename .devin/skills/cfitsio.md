# CFITSIO — Reading FITS Files for NetCDF Presentation

## Overview

CFITSIO is a C library for reading (and writing) FITS (Flexible Image Transport System) files — the standard format for astronomical data. Version 4.6.4 is the latest release. For NEP we use CFITSIO **read-only** to open FITS files and present their contents through the netCDF API via a dispatch table plugin.

- **Header**: `#include <fitsio.h>`
- **Library**: `libcfitsio` (`-lcfitsio`)
- **Dependencies**: zlib (optional: libcurl for URL access)
- **Source**: https://github.com/HEASARC/cfitsio
- **Docs**: https://heasarc.gsfc.nasa.gov/docs/software/fitsio/c/c_user/cfitsio.html

---

## FITS Data Model

A FITS file contains one or more **Header-Data Units (HDUs)**:

```
HDU 1  (Primary HDU)  — always an image (may be empty / 0-axis)
HDU 2  (Extension)    — image, ASCII table, or binary table
HDU 3  (Extension)    — ...
...
```

Each HDU has:
- **Header**: sequence of 80-character keyword records (`KEYWORD = value / comment`)
- **Data**: optional N-dimensional image array or table rows+columns

### HDU Types

| Type constant  | Value | Description |
|---------------|-------|-------------|
| `IMAGE_HDU`   | 0     | N-dimensional pixel array |
| `ASCII_TBL`   | 1     | ASCII table (rows × columns) |
| `BINARY_TBL`  | 2     | Binary table (rows × columns, more efficient) |

### Image BITPIX Values (pixel data types)

| Symbolic constant | BITPIX | C type / netCDF type |
|------------------|--------|---------------------|
| `BYTE_IMG`       | 8      | `unsigned char` / `NC_UBYTE` |
| `SHORT_IMG`      | 16     | `short` / `NC_SHORT` |
| `LONG_IMG`       | 32     | `int` / `NC_INT` |
| `LONGLONG_IMG`   | 64     | `long long` / `NC_INT64` |
| `FLOAT_IMG`      | -32    | `float` / `NC_FLOAT` |
| `DOUBLE_IMG`     | -64    | `double` / `NC_DOUBLE` |

Unsigned integers are represented via `BZERO`/`BSCALE` keywords (CFITSIO handles this transparently when reading with the `fits_get_eqcoltype` or by reading into the appropriate C type).

### Table Column TFORM Codes (binary tables)

| Code | Meaning | netCDF type |
|------|---------|-------------|
| `rA` | character string | `NC_CHAR` (or `NC_STRING`) |
| `rL` | logical | `NC_UBYTE` (0/1) |
| `rB` | unsigned byte | `NC_UBYTE` |
| `rI` | 16-bit signed int | `NC_SHORT` |
| `rJ` | 32-bit signed int | `NC_INT` |
| `rK` | 64-bit signed int | `NC_INT64` |
| `rE` | 32-bit float | `NC_FLOAT` |
| `rD` | 64-bit float | `NC_DOUBLE` |
| `rC` | 32-bit complex pair | 2×`NC_FLOAT` |
| `rM` | 64-bit complex pair | 2×`NC_DOUBLE` |
| `rX` | bit | `NC_UBYTE` (packed) |

Where `r` = vector repeat count (default 1). If `r > 1`, the column is a vector column.

### Key Header Keywords

**Image HDUs**:
- `BITPIX` — pixel data type
- `NAXIS` — number of dimensions (0 = empty)
- `NAXISn` — size of dimension n
- `BSCALE`, `BZERO` — linear scaling: `physical = BZERO + BSCALE * array_value`
- `BUNIT` — physical units of the data
- `BLANK` — integer value representing undefined pixels (integer images only)

**Table HDUs**:
- `TFIELDS` — number of columns
- `NAXIS2` — number of rows
- `TTYPEn` — column name
- `TFORMn` — column data format
- `TUNITn` — column units (optional)
- `TDISPn` — display format (optional)
- `TSCALn`, `TZEROn` — column scaling
- `TNULLn` — null value for integer columns

**WCS (World Coordinate System)**:
- `CRPIXn` — reference pixel coordinate
- `CRVALn` — reference world coordinate value
- `CDELTn` — coordinate increment per pixel
- `CTYPEn` — coordinate type (e.g., `RA---TAN`, `DEC--TAN`, `FREQ`)
- `CUNITn` — coordinate units
- `CROTAn` or `CDi_j` — rotation matrix

**Common metadata**:
- `EXTNAME` — extension name (maps well to netCDF group name or variable name)
- `EXTVER` — extension version
- `DATE-OBS` — observation date
- `TELESCOP`, `INSTRUME`, `OBJECT` — source metadata

---

## CFITSIO Read-Only API Reference

### Error Handling

All CFITSIO routines use an `int status` parameter passed by pointer. A nonzero status on input causes the routine to return immediately. Always initialize `status = 0`.

```c
int status = 0;

/* After any call, check and report: */
if (status) {
    fits_report_error(stderr, status);  /* prints error stack to stderr */
    return status;
}

/* Or get just the text: */
char errtxt[31];
fits_get_errstatus(status, errtxt);

/* Get library version: */
float ver;
fits_get_version(&ver);
```

### Opening and Closing Files

```c
fitsfile *fptr = NULL;
int status = 0;

/* Open any FITS file (positions to primary HDU) */
fits_open_file(&fptr, "myfile.fits", READONLY, &status);

/* Open and move to first HDU with significant data (skips empty primary, GTI) */
fits_open_data(&fptr, "myfile.fits", READONLY, &status);

/* Open and move to first image HDU with NAXIS > 0 */
fits_open_image(&fptr, "myfile.fits", READONLY, &status);

/* Open and move to first table HDU */
fits_open_table(&fptr, "myfile.fits", READONLY, &status);

/* Open a specific extension by name or number (extended filename syntax): */
fits_open_file(&fptr, "myfile.fits[EVENTS]", READONLY, &status);    /* by EXTNAME */
fits_open_file(&fptr, "myfile.fits[2]", READONLY, &status);          /* by HDU number */
fits_open_file(&fptr, "myfile.fits[EVENTS][counts > 0]", READONLY, &status); /* with filter */

/* Open URL (read-only): */
fits_open_file(&fptr, "https://example.com/data.fits", READONLY, &status);

/* Open compressed (.gz, .zip, .Z) — automatic, read-only: */
fits_open_file(&fptr, "myfile.fits.gz", READONLY, &status);

/* Close */
fits_close_file(fptr, &status);
```

**Notes**:
- `READONLY` and `READWRITE` are defined in `fitsio.h`
- Compressed and URL files are read-only
- The `fitsfile` struct is opaque — never access members directly
- Memory for `fitsfile` is allocated/freed by open/close

### HDU Navigation

```c
int num_hdus, current_hdu, hdutype;

/* Total number of HDUs */
fits_get_num_hdus(fptr, &num_hdus, &status);

/* Current HDU number (1-based) */
fits_get_hdu_num(fptr, &current_hdu);  /* note: no status param */

/* Move to absolute HDU number (1-based) */
fits_movabs_hdu(fptr, 2, &hdutype, &status);

/* Move relative (forward/backward) */
fits_movrel_hdu(fptr, 1, &hdutype, &status);   /* next HDU */
fits_movrel_hdu(fptr, -1, &hdutype, &status);   /* previous HDU */

/* Move to named extension */
fits_movnam_hdu(fptr, ANY_HDU, "EVENTS", 0, &status);
/* hdutype filter: IMAGE_HDU, ASCII_TBL, BINARY_TBL, or ANY_HDU */
/* extver=0 means match any version */

/* Get type of current HDU */
fits_get_hdu_type(fptr, &hdutype, &status);
/* Returns IMAGE_HDU, ASCII_TBL, or BINARY_TBL */
```

### Reading Header Keywords (Metadata)

```c
int nkeys, morekeys;

/* How many keywords exist in current HDU header? */
fits_get_hdrspace(fptr, &nkeys, &morekeys, &status);

/* Read keyword by number (1-based, sequential scan) */
char record[81];
fits_read_record(fptr, keynum, record, &status);

/* Read full 80-char card by keyword name */
char card[81];
fits_read_card(fptr, "TELESCOP", card, &status);

/* Read typed keyword value + comment */
char strval[FLEN_VALUE];   /* FLEN_VALUE = 71 */
char comment[FLEN_COMMENT]; /* FLEN_COMMENT = 73 */
fits_read_key(fptr, TSTRING, "OBJECT", strval, comment, &status);

int intval;
fits_read_key(fptr, TINT, "NAXIS", &intval, comment, &status);

double dblval;
fits_read_key(fptr, TDOUBLE, "CRVAL1", &dblval, comment, &status);

int logval;
fits_read_key(fptr, TLOGICAL, "SIMPLE", &logval, comment, &status);

/* Read keyword units (from comment field, enclosed in [brackets]) */
char unit[FLEN_VALUE];
fits_read_key_unit(fptr, "BUNIT", unit, &status);

/* Wildcard keyword search — find all matching keywords */
char *inclist[] = {"NAXIS*"};  /* include pattern */
char *exclist[] = {"NAXIS"};   /* exclude pattern */
fits_find_nextkey(fptr, inclist, 1, exclist, 1, card, &status);
/* Repeat until status == KEY_NO_EXIST */
```

**Datatype constants for `fits_read_key`**: `TSTRING`, `TLOGICAL` (int), `TBYTE`, `TSHORT`, `TUSHORT`, `TINT`, `TUINT`, `TLONG`, `TULONG`, `TLONGLONG`, `TFLOAT`, `TDOUBLE`.

### Reading Image Data

```c
int bitpix, naxis;
long naxes[10];

/* Get image parameters in one call */
fits_get_img_param(fptr, 10, &bitpix, &naxis, naxes, &status);

/* Or individually: */
fits_get_img_type(fptr, &bitpix, &status);    /* BITPIX value */
fits_get_img_dim(fptr, &naxis, &status);      /* number of axes */
fits_get_img_size(fptr, 10, naxes, &status);  /* axis lengths */

/* Read pixels — entire image */
long fpixel[2] = {1, 1};  /* 1-based first pixel coords */
long nelements = naxes[0] * naxes[1];
float *data = malloc(nelements * sizeof(float));
int anynul;
float nulval = 0.0;
fits_read_pix(fptr, TFLOAT, fpixel, nelements, &nulval, data, &anynul, &status);

/* Read a single row (row 10 of a 2D image) */
fpixel[0] = 1;
fpixel[1] = 10;
fits_read_pix(fptr, TFLOAT, fpixel, naxes[0], &nulval, rowdata, &anynul, &status);

/* Read a rectangular subset */
long fpix[2] = {10, 20};   /* lower-left corner (1-based) */
long lpix[2] = {100, 200}; /* upper-right corner */
long inc[2] = {1, 1};      /* stride (1 = every pixel) */
fits_read_subset(fptr, TFLOAT, fpix, lpix, inc, &nulval, data, &anynul, &status);
```

**Important**:
- Pixel coordinates are **1-based** (FITS convention), not 0-based
- `datatype` specifies the C array type — CFITSIO converts automatically if it differs from BITPIX
- `nulval`: if NULL, no null-checking is done; otherwise undefined pixels get this value
- `anynul`: set to 1 if any null pixels were found
- FITS images are stored in **column-major (Fortran)** order: NAXIS1 is the fastest-varying dimension. When mapping to netCDF (row-major C order), **reverse the dimension order**: FITS `[NAXIS1, NAXIS2, ...]` → netCDF `[..., NAXIS2, NAXIS1]`

### Reading Table Data

```c
long nrows;
int ncols;

/* Get table dimensions */
fits_get_num_rows(fptr, &nrows, &status);
fits_get_num_cols(fptr, &ncols, &status);

/* Find column number by name (1-based) */
int colnum;
fits_get_colnum(fptr, CASEINSEN, "ENERGY", &colnum, &status);

/* Find column number with wildcard, also returns matched name */
char colname[FLEN_VALUE];
fits_get_colname(fptr, CASEINSEN, "ENER*", colname, &colnum, &status);

/* Get column data type info */
int typecode;
long repeat, width;
fits_get_coltype(fptr, colnum, &typecode, &repeat, &width, &status);
/* typecode: TSTRING, TSHORT, TLONG, TFLOAT, TDOUBLE, TLOGICAL,
   TBIT, TBYTE, TINT32BIT, TCOMPLEX, TDBLCOMPLEX
   Negative typecode = variable-length array column.
   repeat: vector length (1 for scalar columns)
   width: bytes per element */

/* Equivalent type (accounts for TSCAL/TZERO scaling) */
fits_get_eqcoltype(fptr, colnum, &typecode, &repeat, &width, &status);
/* May return TUSHORT, TULONG etc. for scaled integer columns */

/* Read column data — scalar columns */
float *col_data = malloc(nrows * sizeof(float));
float nulval = -999.0;
int anynul;
fits_read_col(fptr, TFLOAT, colnum, 1 /*firstrow*/, 1 /*firstelem*/,
              nrows /*nelements*/, &nulval, col_data, &anynul, &status);

/* Read column data — string column */
char **str_data = malloc(nrows * sizeof(char*));
for (long i = 0; i < nrows; i++)
    str_data[i] = malloc(width + 1);
char *snulval = "N/A";
fits_read_col(fptr, TSTRING, colnum, 1, 1, nrows, snulval,
              str_data, &anynul, &status);

/* Read vector column (repeat > 1): read all elements of row 5 */
float vec[100];
fits_read_col(fptr, TFLOAT, colnum, 5 /*row*/, 1 /*firstelem*/,
              repeat, &nulval, vec, &anynul, &status);
```

**Notes**:
- Rows are **1-based**
- `firstelem` is for vector columns only (1-based index into the vector); ignored for scalar columns
- Reading spans across rows automatically if `nelements > repeat`
- Any column can be read as `TSTRING` — CFITSIO formats it using `TDISPn` or a default format
- Variable-length array columns have `typecode < 0` from `fits_get_coltype`

---

## Mapping FITS to NetCDF

### Overall Structure

| FITS concept | NetCDF mapping |
|-------------|---------------|
| FITS file | netCDF file (single dataset) |
| HDU (Header-Data Unit) | netCDF group (or flattened with prefix) |
| Image HDU | netCDF variable (N-dimensional) |
| Table HDU | Collection of netCDF variables sharing a row dimension |
| Header keyword | netCDF global/group attribute |
| Image pixel array | netCDF variable data |
| Table column | netCDF variable |
| `EXTNAME` keyword | Group name or variable name prefix |

### Dimension Mapping

**Image HDUs**:
- FITS `NAXISn` → netCDF dimensions, **reversed order**
- FITS stores column-major (Fortran order): `NAXIS1` varies fastest
- netCDF C is row-major: last dimension varies fastest
- Example: FITS 3D `[NAXIS1=100, NAXIS2=200, NAXIS3=5]` → netCDF dims `[5, 200, 100]`
- Use WCS keywords (`CRPIXn`, `CRVALn`, `CDELTn`, `CTYPEn`) to create coordinate variables

**Table HDUs**:
- Row dimension: `NAXIS2` → a shared netCDF dimension (name from `EXTNAME` or `row`)
- Scalar columns → 1D netCDF variables `[nrows]`
- Vector columns (repeat > 1) → 2D netCDF variables `[nrows, repeat]`
- String columns → netCDF `NC_CHAR` with `[nrows, width]` or `NC_STRING` with `[nrows]`

### Type Mapping

| FITS BITPIX / TFORM | netCDF type |
|---------------------|-------------|
| `BYTE_IMG` (8) | `NC_UBYTE` |
| `SHORT_IMG` (16) | `NC_SHORT` |
| `LONG_IMG` (32) | `NC_INT` |
| `LONGLONG_IMG` (64) | `NC_INT64` |
| `FLOAT_IMG` (-32) | `NC_FLOAT` |
| `DOUBLE_IMG` (-64) | `NC_DOUBLE` |
| `rB` (unsigned byte) | `NC_UBYTE` |
| `rI` (16-bit int) | `NC_SHORT` |
| `rJ` (32-bit int) | `NC_INT` |
| `rK` (64-bit int) | `NC_INT64` |
| `rE` (32-bit float) | `NC_FLOAT` |
| `rD` (64-bit float) | `NC_DOUBLE` |
| `rA` (string) | `NC_CHAR` or `NC_STRING` |
| `rL` (logical) | `NC_UBYTE` |
| `rC` (complex float) | compound type or 2×`NC_FLOAT` |
| `rM` (complex double) | compound type or 2×`NC_DOUBLE` |

### Attribute Mapping

| FITS keyword | netCDF attribute | Notes |
|-------------|-----------------|-------|
| `BUNIT` | `units` | Physical units of data |
| `BSCALE` | `scale_factor` | Linear scaling |
| `BZERO` | `add_offset` | Linear offset |
| `BLANK` | `_FillValue` or `missing_value` | Undefined pixels (integer) |
| `OBJECT` | `title` or custom | Observation target |
| `DATE-OBS` | custom global attribute | Observation date |
| `TELESCOP` | custom global attribute | Telescope name |
| `INSTRUME` | custom global attribute | Instrument name |
| `COMMENT` | `comment` | Concatenated comment strings |
| `HISTORY` | `history` | Concatenated history strings |
| `TTYPEn` | variable name | Column name → variable name |
| `TUNITn` | `units` | Column units attribute |
| `CTYPEn` | coordinate variable `long_name` | WCS axis type |
| `CUNITn` | coordinate variable `units` | WCS axis units |
| All other keywords | attribute with same name | Preserve metadata |

### WCS → Coordinate Variables

For image HDUs with WCS keywords, generate 1D coordinate variables:

```c
/* For each axis n (1..NAXIS): */
/* Read CRPIXn, CRVALn, CDELTn, CTYPEn, CUNITn from header */
/* Generate coordinate array: coord[i] = CRVAL + (i + 1 - CRPIX) * CDELT */
/* Create netCDF dimension + coordinate variable with:
   - name derived from CTYPEn (e.g., "RA", "DEC", "FREQ")
   - units from CUNITn
   - values from computed coord[] array */
```

### Scaling

CFITSIO automatically applies `BSCALE`/`BZERO` and `TSCALn`/`TZEROn` when reading data into floating-point types. If you read into the native integer type, raw unscaled values are returned. For netCDF presentation:
- **Option A**: Read as float/double (CFITSIO scales automatically), store `scale_factor=1, add_offset=0` or omit them
- **Option B**: Read raw integers, store `scale_factor=BSCALE, add_offset=BZERO` as netCDF attributes

Option A is simpler and recommended for the initial implementation.

---

## Reading Pattern for NEP Plugin

### During `open` — Build Inventory

```c
fitsfile *fptr = NULL;
int status = 0;

fits_open_file(&fptr, path, READONLY, &status);

int num_hdus;
fits_get_num_hdus(fptr, &num_hdus, &status);

for (int h = 1; h <= num_hdus; h++) {
    int hdutype;
    fits_movabs_hdu(fptr, h, &hdutype, &status);

    /* Read EXTNAME if present */
    char extname[FLEN_VALUE] = "";
    fits_read_key(fptr, TSTRING, "EXTNAME", extname, NULL, &status);
    if (status == KEY_NO_EXIST) { status = 0; strcpy(extname, ""); }

    /* Read all keywords → netCDF attributes */
    int nkeys;
    fits_get_hdrspace(fptr, &nkeys, NULL, &status);
    for (int k = 1; k <= nkeys; k++) {
        char card[81];
        fits_read_record(fptr, k, card, &status);
        /* Parse card → store as attribute */
    }

    if (hdutype == IMAGE_HDU) {
        int bitpix, naxis;
        long naxes[10];
        fits_get_img_param(fptr, 10, &bitpix, &naxis, naxes, &status);
        if (naxis == 0) continue;  /* empty image, skip */
        /* Create dimensions (reversed order) and variable in NC metadata */
    }
    else if (hdutype == ASCII_TBL || hdutype == BINARY_TBL) {
        long nrows;
        int ncols;
        fits_get_num_rows(fptr, &nrows, &status);
        fits_get_num_cols(fptr, &ncols, &status);
        /* Create row dimension, then for each column: */
        for (int c = 1; c <= ncols; c++) {
            char ttype[FLEN_VALUE];
            int typecode;
            long repeat, width;
            fits_get_coltype(fptr, c, &typecode, &repeat, &width, &status);
            fits_read_key(fptr, TSTRING, "TTYPE%d" or use fits_get_colname, ...);
            /* Create variable: 1D [nrows] or 2D [nrows, repeat] */
        }
    }
}
/* Store fptr in format_file_info for later reads; do NOT close yet */
```

### During `get_vara` — Read Data on Demand

```c
/* For image variable: */
/* Convert netCDF 0-based start[]/count[] to FITS 1-based fpixel[]/lpixel[] */
/* Remember to reverse dimension order */
for (int d = 0; d < ndims; d++) {
    int fits_dim = ndims - 1 - d;
    fpixel[fits_dim] = (long)(start[d] + 1);
    lpixel[fits_dim] = (long)(start[d] + count[d]);
    inc[fits_dim] = 1;
}
fits_movabs_hdu(fptr, hdu_number, NULL, &status);
fits_read_subset(fptr, datatype, fpixel, lpixel, inc, &nulval, buf, &anynul, &status);

/* For table column variable: */
fits_movabs_hdu(fptr, hdu_number, NULL, &status);
long firstrow = start[0] + 1;  /* 1-based */
long firstelem = (ndims > 1) ? start[1] + 1 : 1;
long nelements = count[0] * ((ndims > 1) ? count[1] : 1);
fits_read_col(fptr, datatype, colnum, firstrow, firstelem, nelements,
              &nulval, buf, &anynul, &status);
```

### During `close`

```c
fits_close_file(fptr, &status);
/* Free all format-specific metadata */
```

---

## CFITSIO Datatype Constants for C Arrays

When calling `fits_read_pix`, `fits_read_col`, etc., `datatype` specifies the **C buffer type**:

| Constant | C type | Size |
|----------|--------|------|
| `TBYTE` | `unsigned char` | 1 |
| `TSBYTE` | `signed char` | 1 |
| `TSHORT` | `short` | 2 |
| `TUSHORT` | `unsigned short` | 2 |
| `TINT` | `int` | 4 |
| `TUINT` | `unsigned int` | 4 |
| `TLONG` | `long` | 4/8 |
| `TULONG` | `unsigned long` | 4/8 |
| `TLONGLONG` | `long long` | 8 |
| `TFLOAT` | `float` | 4 |
| `TDOUBLE` | `double` | 8 |
| `TSTRING` | `char**` | varies |

CFITSIO performs implicit type conversion between the FITS file type and the requested C type.

---

## CFITSIO String Length Constants

| Constant | Value | Use |
|----------|-------|-----|
| `FLEN_FILENAME` | 1025 | Max filename length |
| `FLEN_KEYWORD` | 72 | Max keyword name length |
| `FLEN_CARD` | 81 | Length of a keyword record (80 + null) |
| `FLEN_VALUE` | 71 | Max keyword value length |
| `FLEN_COMMENT` | 73 | Max keyword comment length |
| `FLEN_ERRMSG` | 81 | Error message length |
| `FLEN_STATUS` | 31 | Status string length |

---

## Build Notes

### Building CFITSIO from source

```bash
wget https://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio-4.6.4.tar.gz
tar xzf cfitsio-4.6.4.tar.gz
cd cfitsio-4.6.4
./configure --prefix=/usr/local/cfitsio-4.6.4
make
make install
```

### Linking

```bash
gcc -I/usr/local/cfitsio-4.6.4/include myprogram.c \
    -L/usr/local/cfitsio-4.6.4/lib -lcfitsio -lm -lz
```

### CMake

```cmake
find_package(CFITSIO REQUIRED)
target_link_libraries(mylib PRIVATE ${CFITSIO_LIBRARIES})
target_include_directories(mylib PRIVATE ${CFITSIO_INCLUDE_DIRS})
```

---

## Error Status Codes (commonly encountered)

| Code | Symbolic | Meaning |
|------|----------|---------|
| 0 | — | Success |
| 104 | `FILE_NOT_OPENED` | Could not open file |
| 107 | `END_OF_FILE` | Unexpected end of file |
| 108 | `READ_ERROR` | Error reading file |
| 202 | `KEY_NO_EXIST` | Keyword not found |
| 207 | `KEY_NOT_UNIQUE` | Multiple keywords match wildcard |
| 219 | `NOT_IMAGE` | Not an image HDU |
| 220 | `NOT_TABLE` | Not a table HDU |
| 237 | `COL_NOT_FOUND` | Column not found |
| 302 | `BAD_PIX_NUM` | Bad pixel number |
| 306 | `NEG_AXIS` | Negative axis size |
| 312 | `BAD_NAXIS` | Illegal NAXIS value |
| 314 | `BAD_BITPIX` | Illegal BITPIX value |

Use `fits_report_error(stderr, status)` for full error stack trace.

---

## Key Differences from Other NEP Formats

| Aspect | GRIB2 | GeoTIFF | FITS |
|--------|-------|---------|------|
| Data model | Messages × products | Raster bands | HDUs (images + tables) |
| Metadata | Template-based codes | TIFF tags + GeoKeys | Keyword=value records |
| Dimensions | Grid definition | Raster dims + bands | NAXIS + NAXISn |
| Coordinates | Code tables → lat/lon | GeoTransform + SRS | WCS keywords |
| Tables | No | No | Yes (ASCII + Binary) |
| Library | g2c | libgeotiff + libtiff | CFITSIO |
| Pixel order | Row-major | Row-major | Column-major (Fortran) |
