# FITS Format Reader

FITS (Flexible Image Transport System) is the standard format for astronomical data from telescopes and instruments such as HST, Chandra, and JWST.

**Transparent Access**: Read FITS files with `nc_open()` after calling `NC_FITS_initialize()`.

**HDU-to-Group Mapping**:
- Primary HDU → root group; variable named `image`
- Each extension HDU → child group named from `EXTNAME` or `hdu_N`
- Binary/ASCII table HDUs → one netCDF variable per column

**Metadata Mapping**: `BUNIT`→`units`, `BZERO`→`add_offset`, `BSCALE`→`scale_factor`, `BLANK`→`_FillValue`; all other header keywords become string attributes.

**Use Cases**: HST imaging, Chandra X-ray data, JWST observations, any FITS-format observatory data.

**Enabling:**
```bash
cmake -B build -DNEP_ENABLE_FITS=ON   # CMake
./configure --enable-fits          # Autotools
```
**Dependencies**: CFITSIO ≥ 3.0.

**Resources**: [FITS Standard](https://fits.gsfc.nasa.gov/fits_standard.html) · [CFITSIO Home](https://heasarc.gsfc.nasa.gov/docs/software/fitsio/) · [FITS Documentation Index](https://fits.gsfc.nasa.gov/fits_documentation.html)

**Example:**
```c
#include "fitsdispatch.h"
NC_FITS_initialize();   /* register UDF3; safe to call even if already registered */
nc_open("image.fits", NC_NOWRITE, &ncid);
nc_inq_varid(ncid, "image", &varid);
nc_get_vara_float(ncid, varid, start, count, pixels);
nc_close(ncid);
```
