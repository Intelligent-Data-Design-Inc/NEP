# NCEPLIBS-g2c Library

## Overview

NCEPLIBS-g2c encodes and decodes GRIB Edition 2 messages. Starting with version 2.0.0 it has two distinct API layers — the **file-based API** (new, preferred for new code like NEP) and the **legacy message-based API** (historical, Fortran-era).

## Two APIs — Which to Use

| API | When introduced | Status | Use in NEP |
|-----|----------------|--------|------------|
| File-based (`g2c_*`) | v2.0.0 | **Preferred** | Yes |
| Legacy message-based (`g2_*`, `seekgb`) | Pre-2.0 | Historical/deprecated | Avoid |

**For NEP: always use the file-based `g2c_*` API. Avoid `g2_getfld`, `seekgb`, `g2c_get_msg` for new code.**

---

## File-Based API (v2.0.0+, use in NEP)

### File lifecycle

```c
int g2c_open(const char *path, int mode, int *g2cid);
int g2c_open_index(const char *data_file, const char *index_file, int mode, int *g2cid);
int g2c_close(int g2cid);
```

- `mode`: `G2C_NOWRITE` for read-only
- File IDs are 0-based integers managed by the library
- **Always call `g2c_close` to release resources**

### Inquiry

```c
int g2c_inq(int g2cid, int *num_msg);

int g2c_inq_msg(int g2cid, int msg_num,
                unsigned char *discipline, int *num_fields, int *num_local,
                short *center, short *subcenter,
                unsigned char *master_version, unsigned char *local_version);

int g2c_inq_msg_time(int g2cid, int msg_num,
                     unsigned char *sig_ref_time, short *year, unsigned char *month,
                     unsigned char *day, unsigned char *hour, unsigned char *minute,
                     unsigned char *second);

int g2c_inq_prod(int g2cid, int msg_num, int prod_num,
                 int *pds_template_len, long long int *pds_template,
                 int *gds_template_len, long long int *gds_template,
                 int *drs_template_len, long long int *drs_template);

int g2c_inq_dim(int g2cid, int msg_num, int prod_num, int dim_num,
                size_t *len, char *name, double *values);

int g2c_inq_dim_info(int g2cid, int msg_num, int prod_num, int dim_num,
                     size_t *len, char *name);
```

Key notes:
- `msg_num` and `prod_num` are **0-based**
- `pds_template[0]` = parameter category, `pds_template[1]` = parameter number
- `g2c_inq_dim_info` is a shortcut for `g2c_inq_dim` that skips the coordinate values array
- **`dim_name` buffer passed to `g2c_inq_dim`/`g2c_inq_dim_info` must be 1024 bytes** — internally `strncpy(..., 1024)` is used regardless of the actual name length
- Template array max sizes: PDS=50, GDS=28, DRS=18 entries (safe to use `long long int arr[200]`)

### Reading product data — **preferred method**

```c
int g2c_get_prod(int g2cid, int msg_num, int prod_num,
                 int *num_data_points, float **data);
```

- Reads and unpacks the data for one product
- Allocates `*data`; caller must `free(*data)`
- `*num_data_points` = total grid points (nx * ny)
- **This is the correct way to read data in new code — replaces the `g2_getfld`/`g2c_get_msg`/`seekgb` chain**
- Returns `G2C_NOERROR` (0) on success

### Parameter abbreviations

```c
int g2c_param_abbrev(int g2disc, int g2cat, int g2num, char *abbrev);
```

- `abbrev` buffer should be `NC_GRIB2_ABBREV_LEN` bytes (64 is safe; `G2C_MAX_NOAA_ABBREV_LEN` = 11)

### Error codes

```c
G2C_NOERROR      // 0 - success
G2C_ERROR        // generic error
G2C_EINVAL       // invalid input
G2C_EBADID       // bad g2cid
G2C_EFILE        // file I/O error
G2C_ENOMEM       // out of memory
G2C_ENOMSG       // no GRIB message found
G2C_ENOPRODUCT   // product not found
G2C_ENOSECTION   // section not found
G2C_ETOOMANYFILES // too many open files
```

---

## Legacy Message-Based API (historical — avoid in NEP)

These exist for backward compatibility. Do **not** use for new code:

| Function | Problem |
|----------|---------|
| `seekgb(FILE*, g2int iseek, g2int mseek, g2int *lskip, g2int *lgrib)` | `g2int` is 32-bit; truncates GRIB2 message lengths > 2GB; returns wrong length for large messages |
| `g2c_seekmsg(g2cid, skip_bytes, *offset, *msglen)` | `skip` is a **byte offset** (not message index); returns absolute file byte offset in `*offset` |
| `g2c_get_msg(g2cid, skip_bytes, max_bytes, *bytes_to_msg, *bytes_in_msg, **cbuf)` | Uses `seekgb` internally — truncates 8-byte GRIB2 message length to 4 bytes; `*bytes_to_msg` is an absolute file offset (not a buffer offset); `cbuf[0]` IS the message start |
| `g2_getfld(cbuf, ifldnum, unpack, expand, **gfld)` | `ifldnum` is 1-based; requires raw GRIB2 message bytes at `cbuf[0]`; returns allocated `gribfield*`, caller must `g2_free(gfld)` |
| `g2_info`, `g2_create`, `g2_addgrid`, etc. | Legacy encoding/decoding; use file-based API instead |

### Critical `seekgb` bug
`seekgb` reads the GRIB2 message length via `gbit(..., (k+12)*BYTE, 4*BYTE)` — only 4 bytes — but GRIB2 Section 0 stores message length as an **8-byte big-endian integer** at offset 8. For messages > ~4GB this truncates, but even for normal messages the value read is wrong because it reads bytes 12-15 (4 bytes) instead of the full 8-byte field at bytes 8-15.

---

## GRIB2 Message Structure

```
Section 0: Indicator (16 bytes: "GRIB", reserved, discipline, edition=2, total_length[8 bytes])
Section 1: Identification (center, subcenter, master/local version, ref time, ...)
Section 2: Local Use (optional, repeatable)
Section 3: Grid Definition (GDS) — repeatable
Section 4: Product Definition (PDS) — repeatable, per-field
Section 5: Data Representation (DRS) — repeatable
Section 6: Bit-map — repeatable
Section 7: Data — repeatable
Section 8: End ("7777")
```

- Sections 3-7 repeat for each field within a message
- A single GRIB2 file typically has one message per field, but can have multiple fields per message
- PDS template 0 fields: `[0]=category, [1]=param_number, [2]=type_of_generating_process, ...`

---

## Correct Pattern for Reading Data in NEP (grib2file.c)

**During open** (`NC_GRIB2_open`): use `g2c_inq*` functions to build inventory. Also call `g2c_seekmsg` (NOT `seekgb`) per message to capture correct byte offsets:

```c
size_t msg_btm, msg_bim, file_pos = 0;
for (int m = 0; m < num_msg; m++) {
    g2c_seekmsg(g2cid, file_pos, &msg_btm, &msg_bim);  /* uses hton64 - correct 8-byte length */
    file_pos = msg_btm + msg_bim;
    /* store msg_btm, msg_bim in product/var info */
}
```

**During get_vara** (`NC_GRIB2_get_vara`): use direct `fopen`/`fread` + `g2_getfld` with `expand=1`:

```c
/* Open file and read raw message bytes at the known offset */
FILE *f = fopen(path, "rb");
fseeko(f, (off_t)bytes_to_msg, SEEK_SET);
unsigned char *msgbuf = malloc(bytes_in_msg);
fread(msgbuf, 1, bytes_in_msg, f);
fclose(f);

/* Unpack with bitmap expansion. prod_index is 0-based; ifldnum is 1-based. */
gribfield *gfld = NULL;
g2_getfld(msgbuf, prod_index + 1, 1 /*unpack*/, 1 /*expand*/, &gfld);
free(msgbuf);

/* With expand=1, gfld->fld is already ngrdpts (=nx*ny) elements.
 * gfld->bmap[i]==1 means valid data; ==0 means land/masked.
 * Substitute _FillValue for masked points: */
for (int i = 0; i < nx * ny; i++)
    full_buf[i] = (gfld->expanded && gfld->ibmap == 0 && gfld->bmap && !gfld->bmap[i])
                  ? FILL_VALUE : gfld->fld[i];
g2_free(gfld);
```

### Why not `g2c_get_prod`?
`g2c_get_prod` takes a **pre-allocated** `float *data` buffer (caller must size it) and returns only the `ndpts` bitmap-active data points — it does **not** expand the bitmap or return a full `ngrdpts` grid. There is no public API to get the bitmap separately, so reconstructing the full grid is not possible without private headers.

### Why not `g2c_get_msg` + `g2_getfld`?
`g2c_get_msg` uses `seekgb` internally. `seekgb` reads the message length using `gbit(..., 4 bytes)` which only reads 4 of the 8 bytes of the GRIB2 Section 0 length field — corrupting the length for any messages > ~4GB (and sometimes even smaller messages). Use `g2c_seekmsg` instead, which uses `hton64` for the full 8-byte read.

---

## Header Files

```c
#include <grib2.h>         // public API: g2c_*, g2_*, seekgb
                            // installed to $prefix/include/grib2.h
```

Internal header (not for external use):
```c
#include "grib2_int.h"     // G2C_MESSAGE_INFO_T, G2C_FILE_INFO_T, etc.
```

## Build Notes

- Library: `libg2c` (shared: `libg2c.so`, static: `libg2c.a`)
- Requires: Jasper (JPEG2000), libpng, zlib for compression support
- `LD_LIBRARY_PATH` must include the g2c lib dir and Jasper lib dir at runtime
- Typical install prefix: `/usr/local/NCEPLIBS-g2c-2.3.0`
