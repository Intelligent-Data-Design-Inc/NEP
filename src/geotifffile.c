/**
 * @file
 * @internal GeoTIFF file format detection and operations.
 *
 * This file implements GeoTIFF format detection for the NetCDF API.
 * It validates TIFF magic numbers, headers, and GeoTIFF-specific tags
 * to distinguish GeoTIFF files from regular TIFF files.
 *
 * @author Edward Hartnett
 * @date 2025-12-26
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include "geotiffdispatch.h"
#include "nc.h"
#include "nc4internal.h"
#include "hdf5internal.h"

#ifdef HAVE_GEOTIFF
#include <tiffio.h>
#include <geotiff/geotiff.h>
#include <geotiff/geo_normalize.h>
#include <geotiff/geotiffio.h>
#include <geotiff/xtiffio.h>
#endif

/** GeoTIFF key directory tag */
#define GEOTIFF_KEY_DIRECTORY_TAG 34735

/** Maximum reasonable IFD offset (100 MB) */
#define MAX_IFD_OFFSET 104857600

/** TIFF IFD entry size */
#define TIFF_IFD_ENTRY_SIZE 12

/** Maximum buffer size for reading (1 GB) */
#define MAX_BUFFER_SIZE 1073741824

/** Maximum number of IFD entries to process */
#define MAX_IFD_ENTRIES 4096

/**
 * Byte-swap a 16-bit value.
 *
 * @param val Value to swap.
 * @return Swapped value.
 */
static unsigned short
swap16(unsigned short val)
{
    return ((val & 0xFF) << 8) | ((val >> 8) & 0xFF);
}

/**
 * Byte-swap a 32-bit value.
 *
 * @param val Value to swap.
 * @return Swapped value.
 */
static unsigned int
swap32(unsigned int val)
{
    return ((val & 0xFF) << 24) | ((val & 0xFF00) << 8) |
           ((val >> 8) & 0xFF00) | ((val >> 24) & 0xFF);
}

#ifdef HAVE_GEOTIFF
/* Forward declarations for helper functions */
static int detect_tiff_organization(TIFF *tiff, NC_GEOTIFF_FILE_INFO_T *geotiff_info);
static void *allocate_read_buffer(NC_GEOTIFF_FILE_INFO_T *geotiff_info, size_t type_size);
static void free_read_buffer(void *buffer);
static int validate_hyperslab(NC_VAR_INFO_T *var, const size_t *start, const size_t *count);
static int read_scanline(TIFF *tiff, uint32_t row, void *buffer, uint16_t samples_per_pixel);
static int read_tile(TIFF *tiff, uint32_t tile_x, uint32_t tile_y, void *buffer);
static int read_single_band_hyperslab(NC_FILE_INFO_T *h5, NC_VAR_INFO_T *var,
                                     const size_t *start, const size_t *count,
                                     void *value, size_t type_size);
static int read_multi_band_hyperslab(NC_FILE_INFO_T *h5, NC_VAR_INFO_T *var,
                                    const size_t *start, const size_t *count,
                                    void *value, size_t type_size);
#endif

/**
 * @internal Set the type of a netCDF-4 variable.
 *
 * @param xtype A netcdf type.
 * @param endianness The endianness of the data.
 * @param type_size The size in bytes of one element of this type.
 * @param type_name A name for the type.
 * @param typep Pointer to a pointer that gets the TYPE_INFO_T struct.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static int
nc4_set_var_type(nc_type xtype, int endianness, size_t type_size, char *type_name,
                 NC_TYPE_INFO_T **typep)
{
    NC_TYPE_INFO_T *type;

    /* Check inputs. */
    assert(typep);

    /* Allocate space for the type info struct. */
    if (!(type = calloc(1, sizeof(NC_TYPE_INFO_T))))
        return NC_ENOMEM;
    if (!(type->hdr.name = strdup(type_name)))
    {
        free(type);
        return NC_ENOMEM;
    }
    type->hdr.sort = NCTYP;

    /* Determine the type class. */
    if (xtype == NC_FLOAT)
        type->nc_type_class = NC_FLOAT;
    else if (xtype == NC_DOUBLE)
        type->nc_type_class = NC_DOUBLE;
    else if (xtype == NC_CHAR)
        type->nc_type_class = NC_STRING;
    else
        type->nc_type_class = NC_INT;

    /* Set other type info values. */
    type->endianness = endianness;
    type->size = type_size;
    type->hdr.id = (size_t)xtype;

    /* Return to caller. */
    *typep = type;

    return NC_NOERR;
}

/**
 * @internal Create a new variable and insert int relevant lists
 *
 * @param grp the containing group
 * @param name the name for the new variable
 * @param ndims the rank of the new variable
 * @param format_var_info Pointer to format-specific var info struct.
 * @param var Pointer in which to return a pointer to the new var.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
static int
nc4_var_list_add_full(NC_GRP_INFO_T* grp, const char* name, int ndims, nc_type xtype,
                      int endianness, size_t type_size, char *type_name, void *fill_value,
                      int contiguous, size_t *chunksizes, void *format_var_info,
                      NC_VAR_INFO_T **var)
{
    int d;
    int retval;

    /* Add the VAR_INFO_T struct to our list of vars. */
    if ((retval = nc4_var_list_add(grp, name, ndims, var)))
        return retval;
    (*var)->created = NC_TRUE;
    (*var)->written_to = NC_TRUE;
    (*var)->format_var_info = format_var_info;
    (*var)->atts_read = 1;

    /* Fill special type_info struct for variable type information. */
    if ((retval = nc4_set_var_type(xtype, endianness, type_size, type_name,
                                   &(*var)->type_info)))
        return retval;

    /* Propagate the endianness to the variable */
    (*var)->endianness = (*var)->type_info->endianness;

    (*var)->type_info->rc++;

    /* Handle fill value, if provided. */
    if (fill_value)
    {
        if (!((*var)->fill_value = malloc(type_size)))
            return NC_ENOMEM;
        memcpy((*var)->fill_value, fill_value, type_size);
    }

    /* Var contiguous or chunked? */
    if (contiguous)
        (*var)->storage = NC_CONTIGUOUS;
    else
        (*var)->storage = NC_CHUNKED;

    /* Were chunksizes provided? */
    if (chunksizes)
    {
        if (!((*var)->chunksizes = malloc(ndims * sizeof(size_t))))
            return NC_ENOMEM;
        for (d = 0; d < ndims; d++)
            (*var)->chunksizes[d] = chunksizes[d];
    }

    return NC_NOERR;
}

/**
 * Read and validate TIFF header.
 *
 * @param fp File pointer.
 * @param is_little_endian Output: 1 if little-endian, 0 if big-endian.
 * @param is_bigtiff Output: 1 if BigTIFF, 0 if classic TIFF.
 * @param ifd_offset Output: IFD offset.
 * @return NC_NOERR on success, error code otherwise.
 */
static int
read_tiff_header(FILE *fp, int *is_little_endian, int *is_bigtiff,
                 unsigned int *ifd_offset)
{
    unsigned char header[TIFF_HEADER_SIZE];
    unsigned short magic, version;
    int need_swap = 0;

    /* Read header */
    if (fread(header, 1, TIFF_HEADER_SIZE, fp) != TIFF_HEADER_SIZE)
        return NC_ENOTNC;

    /* Check magic number */
    magic = (header[0] << 8) | header[1];
    if (magic == TIFF_MAGIC_BE)
    {
        *is_little_endian = 0;
#ifdef WORDS_BIGENDIAN
        need_swap = 0;
#else
        need_swap = 1;
#endif
    }
    else if (magic == ((TIFF_MAGIC_LE >> 8) | ((TIFF_MAGIC_LE & 0xFF) << 8)))
    {
        *is_little_endian = 1;
#ifdef WORDS_BIGENDIAN
        need_swap = 1;
#else
        need_swap = 0;
#endif
    }
    else
    {
        return NC_ENOTNC;
    }

    /* Read version */
    version = (header[2]) | (header[3] << 8);
    if (need_swap)
        version = swap16(version);

    if (version == TIFF_VERSION_CLASSIC)
    {
        *is_bigtiff = 0;
        /* Read IFD offset (4 bytes at offset 4) */
        *ifd_offset = (header[4]) | (header[5] << 8) |
                      (header[6] << 16) | (header[7] << 24);
        if (need_swap)
            *ifd_offset = swap32(*ifd_offset);
    }
    else if (version == TIFF_VERSION_BIGTIFF)
    {
        *is_bigtiff = 1;
        /* For BigTIFF, we need to read more bytes for the 64-bit offset */
        /* For now, we'll just read the lower 32 bits */
        unsigned char bigtiff_header[8];
        if (fread(bigtiff_header, 1, 8, fp) != 8)
            return NC_ENOTNC;
        *ifd_offset = (bigtiff_header[0]) | (bigtiff_header[1] << 8) |
                      (bigtiff_header[2] << 16) | (bigtiff_header[3] << 24);
        if (need_swap)
            *ifd_offset = swap32(*ifd_offset);
    }
    else
    {
        return NC_ENOTNC;
    }

    /* Validate IFD offset */
    if (*ifd_offset < TIFF_HEADER_SIZE || *ifd_offset > MAX_IFD_OFFSET)
        return NC_ENOTNC;

    return NC_NOERR;
}

/**
 * Check if IFD contains GeoTIFF tags.
 *
 * @param fp File pointer.
 * @param ifd_offset IFD offset.
 * @param is_little_endian 1 if little-endian, 0 if big-endian.
 * @param is_bigtiff 1 if BigTIFF, 0 if classic TIFF.
 * @param has_geotiff_tags Output: 1 if GeoTIFF tags found, 0 otherwise.
 * @return NC_NOERR on success, error code otherwise.
 */
static int
check_geotiff_tags(FILE *fp, unsigned int ifd_offset, int is_little_endian,
                   int is_bigtiff, int *has_geotiff_tags)
{
    unsigned long long num_entries;
    unsigned char entry_count_buf[8];
    int need_swap;
    int i;
    size_t entry_size;
    size_t count_size;

    *has_geotiff_tags = 0;

#ifdef WORDS_BIGENDIAN
    need_swap = is_little_endian;
#else
    need_swap = !is_little_endian;
#endif

    /* Seek to IFD */
    if (fseek(fp, ifd_offset, SEEK_SET) != 0)
        return NC_ENOTNC;

    /* BigTIFF uses 8-byte entry count, classic TIFF uses 2-byte */
    if (is_bigtiff)
    {
        count_size = 8;
        entry_size = 20;  /* BigTIFF IFD entries are 20 bytes */
    }
    else
    {
        count_size = 2;
        entry_size = 12;  /* Classic TIFF IFD entries are 12 bytes */
    }

    /* Read number of entries */
    if (fread(entry_count_buf, 1, count_size, fp) != count_size)
        return NC_ENOTNC;

    if (is_bigtiff)
    {
        /* BigTIFF: 8-byte entry count */
        num_entries = (unsigned long long)entry_count_buf[0] |
                     ((unsigned long long)entry_count_buf[1] << 8) |
                     ((unsigned long long)entry_count_buf[2] << 16) |
                     ((unsigned long long)entry_count_buf[3] << 24) |
                     ((unsigned long long)entry_count_buf[4] << 32) |
                     ((unsigned long long)entry_count_buf[5] << 40) |
                     ((unsigned long long)entry_count_buf[6] << 48) |
                     ((unsigned long long)entry_count_buf[7] << 56);
        /* For simplicity, we only handle reasonable entry counts */
        if (num_entries > MAX_IFD_ENTRIES)
            return NC_ENOTNC;
    }
    else
    {
        /* Classic TIFF: 2-byte entry count */
        num_entries = (entry_count_buf[0]) | (entry_count_buf[1] << 8);
        if (need_swap)
            num_entries = swap16((unsigned short)num_entries);
        if (num_entries > MAX_IFD_ENTRIES)
            return NC_ENOTNC;
    }

    /* Read and check each IFD entry */
    for (i = 0; i < (int)num_entries; i++)
    {
        unsigned char entry[20];  /* Max size for BigTIFF */
        unsigned short tag;

        if (fread(entry, 1, entry_size, fp) != entry_size)
            return NC_ENOTNC;

        /* Extract tag ID (first 2 bytes - same for both formats) */
        tag = (entry[0]) | (entry[1] << 8);
        if (need_swap)
            tag = swap16(tag);

        /* Check for GeoTIFF key directory tag */
        if (tag == GEOTIFF_KEY_DIRECTORY_TAG)
        {
            *has_geotiff_tags = 1;
            return NC_NOERR;
        }
    }

    return NC_NOERR;
}

/**
 * @internal Detect if a file is in GeoTIFF format.
 *
 * This function checks if a file is a valid GeoTIFF file by:
 * 1. Validating TIFF magic number (little-endian or big-endian)
 * 2. Validating TIFF version (classic TIFF 42 or BigTIFF 43)
 * 3. Validating IFD offset
 * 4. Checking for GeoTIFF-specific tags (tag 34735)
 *
 * @param path Path to the file to check.
 * @param is_geotiff Output parameter: set to 1 if file is GeoTIFF, 0 otherwise.
 *
 * @return NC_NOERR on success (whether file is GeoTIFF or not).
 * @return NC_EINVAL if parameters are invalid.
 * @return NC_ENOTNC if file cannot be read or is not a valid TIFF.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_detect_format(const char *path, int *is_geotiff)
{
    FILE *fp = NULL;
    int is_little_endian;
    int is_bigtiff;
    unsigned int ifd_offset;
    int has_geotiff_tags;
    int ret;

    /* Validate parameters */
    if (!path || !is_geotiff)
        return NC_EINVAL;

    /* Initialize output */
    *is_geotiff = 0;

    /* Open file */
    fp = fopen(path, "rb");
    if (!fp)
        return NC_ENOTNC;

    /* Read and validate TIFF header */
    ret = read_tiff_header(fp, &is_little_endian, &is_bigtiff, &ifd_offset);
    if (ret != NC_NOERR)
    {
        fclose(fp);
        /* Not a valid TIFF file, but not an error */
        if (ret == NC_ENOTNC)
        {
            *is_geotiff = 0;
            return NC_NOERR;
        }
        return ret;
    }

    /* Check for GeoTIFF tags */
    ret = check_geotiff_tags(fp, ifd_offset, is_little_endian, is_bigtiff,
                             &has_geotiff_tags);
    fclose(fp);

    if (ret != NC_NOERR)
    {
        /* Error reading IFD, treat as not GeoTIFF */
        if (ret == NC_ENOTNC)
        {
            *is_geotiff = 0;
            return NC_NOERR;
        }
        return ret;
    }

    *is_geotiff = has_geotiff_tags;
    return NC_NOERR;
}

#ifdef HAVE_GEOTIFF
/**
 * @internal Open a GeoTIFF file and initialize file handle.
 *
 * @param path Path to the GeoTIFF file.
 * @param mode File open mode (must be NC_NOWRITE for read-only).
 * @param basepe Unused (for parallel I/O).
 * @param chunksizehintp Unused (for chunking).
 * @param parameters Unused (for format-specific parameters).
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID for this file.
 *
 * @return NC_NOERR on success.
 * @return NC_EINVAL if parameters are invalid or write mode requested.
 * @return NC_ENOTNC if file is not a valid GeoTIFF.
 * @return NC_ENOMEM if memory allocation fails.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
                void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC_FILE_INFO_T *h5 = NULL;
    NC_GEOTIFF_FILE_INFO_T *geotiff_info = NULL;
    NC *nc = NULL;
    TIFF *tiff = NULL;
    GTIF *gtif = NULL;
    int ret = NC_NOERR;
    FILE *fp = NULL;
    int is_little_endian;
    int is_bigtiff;
    unsigned int ifd_offset;

    (void)basepe;
    (void)chunksizehintp;
    (void)parameters;
    (void)dispatch;

    /* Validate parameters */
    if (!path)
        return NC_EINVAL;

    /* Only read-only mode is supported */
    if (mode & NC_WRITE)
        return NC_EINVAL;

    /* Find pointer to NC */
    if ((ret = NC_check_id(ncid, &nc)))
        return ret;

    /* Read TIFF header to get endianness */
    fp = fopen(path, "rb");
    if (!fp)
        return NC_ENOTNC;

    ret = read_tiff_header(fp, &is_little_endian, &is_bigtiff, &ifd_offset);
    if (ret != NC_NOERR)
    {
        fclose(fp);
        return ret;
    }

    /* Check for GeoTIFF tags */
    int has_geotiff_tags;
    ret = check_geotiff_tags(fp, ifd_offset, is_little_endian, is_bigtiff, &has_geotiff_tags);
    fclose(fp);

    if (ret != NC_NOERR)
        return ret;
    if (!has_geotiff_tags)
        return NC_ENOTNC;

    /* Add necessary structs to hold netcdf-4 file data */
    if ((ret = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return ret;
    if (!h5 || !h5->root_grp)
        return NC_ENOMEM;
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Allocate GeoTIFF-specific file info structure */
    geotiff_info = (NC_GEOTIFF_FILE_INFO_T *)malloc(sizeof(NC_GEOTIFF_FILE_INFO_T));
    if (!geotiff_info)
    {
        ret = NC_ENOMEM;
        goto cleanup;
    }
    memset(geotiff_info, 0, sizeof(NC_GEOTIFF_FILE_INFO_T));

    /* Duplicate file path */
    geotiff_info->path = strdup(path);
    if (!geotiff_info->path)
    {
        ret = NC_ENOMEM;
        goto cleanup;
    }

    /* Open TIFF file using XTIFFOpen for better GeoTIFF compatibility */
    tiff = XTIFFOpen(path, "r");
    if (!tiff)
    {
        ret = NC_ENOTNC;
        goto cleanup;
    }
    geotiff_info->tiff_handle = tiff;

    /* Store endianness from TIFF header */
    geotiff_info->is_little_endian = is_little_endian;

    /* Initialize GTIFNew context with error handling for malformed tags */
    gtif = GTIFNew(tiff);
    if (!gtif)
    {
        /* GTIFNew can fail with malformed GeoTIFF tags */
        /* This is not necessarily fatal - we can still read the raster data */
        /* but we won't have georeferencing information */
        geotiff_info->gtif_handle = NULL;
    }
    else
    {
        /* Validate GeoTIFF directory version */
        int versions[3];
        int keycount;
        GTIFDirectoryInfo(gtif, versions, &keycount);

        /* Check version compatibility (should be version 1) */
        if (versions[0] > 1)
        {
            /* Future version - may not be compatible */
            GTIFFree(gtif);
            geotiff_info->gtif_handle = NULL;
        }
        else
        {
            geotiff_info->gtif_handle = gtif;
        }
    }

    /* Store GeoTIFF file info in dispatch layer */
    h5->format_file_info = geotiff_info;

    /* Extract metadata: dimensions, data types, and CRS */
    if ((ret = NC_GEOTIFF_extract_metadata(h5, geotiff_info)))
        goto cleanup;

    return NC_NOERR;

cleanup:
    /* Clean up resources on error */
    if (geotiff_info)
    {
        if (geotiff_info->gtif_handle)
            GTIFFree((GTIF *)geotiff_info->gtif_handle);
        if (geotiff_info->tiff_handle)
            XTIFFClose((TIFF *)geotiff_info->tiff_handle);
        if (geotiff_info->path)
            free(geotiff_info->path);
        free(geotiff_info);
    }
    return ret;
}

/**
 * @internal Close a GeoTIFF file and release resources.
 *
 * @param ncid NetCDF ID for this file.
 * @param ignore Unused parameter (for dispatch compatibility).
 *
 * @return NC_NOERR on success.
 * @return NC_EBADID if ncid is invalid.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_close(int ncid, void *ignore)
{
    NC_GRP_INFO_T *grp;
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_GEOTIFF_FILE_INFO_T *geotiff_info;
    int retval;

    (void)ignore;

    /* Find our metadata for this file */
    if ((retval = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5)))
        return retval;

    if (!h5 || !h5->format_file_info)
        return NC_EBADID;

    /* Get GeoTIFF-specific file info */
    geotiff_info = (NC_GEOTIFF_FILE_INFO_T *)h5->format_file_info;

    /* Free GTIF context */
    if (geotiff_info->gtif_handle)
        GTIFFree((GTIF *)geotiff_info->gtif_handle);

    /* Close TIFF file */
    if (geotiff_info->tiff_handle)
        XTIFFClose((TIFF *)geotiff_info->tiff_handle);

    /* Free path string */
    if (geotiff_info->path)
        free(geotiff_info->path);

    /* Free GeoTIFF file info structure */
    free(geotiff_info);

    /* Free the NC_FILE_INFO_T struct */
    if ((retval = nc4_nc4f_list_del(h5)))
        return retval;

    return NC_NOERR;
}

/**
 * @internal Abort a GeoTIFF file operation and release resources.
 *
 * This function is called on error paths to clean up resources.
 * It performs the same cleanup as NC_GEOTIFF_close but is used
 * when an error has occurred during file operations.
 *
 * @param ncid NetCDF ID for this file.
 *
 * @return NC_NOERR on success.
 * @return NC_EBADID if ncid is invalid.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_abort(int ncid)
{
    /* Abort is the same as close for read-only operations */
    return NC_GEOTIFF_close(ncid, NULL);
}

/**
 * @internal Query the format of a GeoTIFF file.
 *
 * @param ncid NetCDF ID for this file.
 * @param formatp Pointer to store format value.
 *
 * @return NC_NOERR on success.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_GEOTIFF;
    return NC_NOERR;
}

/**
 * @internal Query the extended format of a GeoTIFF file.
 *
 * @param ncid NetCDF ID for this file.
 * @param formatp Pointer to store format value.
 * @param modep Pointer to store mode value.
 *
 * @return NC_NOERR on success.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_GEOTIFF;
    if (modep)
        *modep = NC_FORMATX_NC_GEOTIFF;
    return NC_NOERR;
}

/**
 * @internal Extract metadata from GeoTIFF file.
 *
 * This function extracts dimensions, data types, and coordinate reference
 * system information from a GeoTIFF file and populates the NetCDF metadata
 * structures.
 *
 * @param h5 Pointer to NC_FILE_INFO_T structure.
 * @param geotiff_info Pointer to GeoTIFF-specific file info.
 *
 * @return NC_NOERR on success.
 * @return NC_EHDFERR on TIFF/GeoTIFF errors.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_extract_metadata(NC_FILE_INFO_T *h5, NC_GEOTIFF_FILE_INFO_T *geotiff_info)
{
    TIFF *tiff;
    GTIF *gtif;
    uint32_t width, height;
    uint16_t samples_per_pixel = 1;
    uint16_t bits_per_sample = 0;
    uint16_t sample_format = SAMPLEFORMAT_UINT;
    NC_GRP_INFO_T *grp;
    NC_DIM_INFO_T *dim_x = NULL, *dim_y = NULL, *dim_band = NULL;
    NC_VAR_INFO_T *var = NULL;
    nc_type xtype = NC_UBYTE;
    int endianness;
    size_t type_size = 0;
    char type_name[NC_MAX_NAME + 1];
    int retval;

    if (!h5 || !geotiff_info || !geotiff_info->tiff_handle)
        return NC_EINVAL;

    tiff = (TIFF *)geotiff_info->tiff_handle;
    gtif = (GTIF *)geotiff_info->gtif_handle;
    grp = h5->root_grp;

    /* Extract image dimensions */
    if (!TIFFGetField(tiff, TIFFTAG_IMAGEWIDTH, &width))
        return NC_EHDFERR;
    if (!TIFFGetField(tiff, TIFFTAG_IMAGELENGTH, &height))
        return NC_EHDFERR;

    /* Get samples per pixel (bands) */
    TIFFGetField(tiff, TIFFTAG_SAMPLESPERPIXEL, &samples_per_pixel);

    /* Cache image dimensions and samples in geotiff_info for later use */
    geotiff_info->image_width = width;
    geotiff_info->image_height = height;
    geotiff_info->samples_per_pixel = samples_per_pixel;

    /* Detect and cache TIFF organization (tiled vs striped, planar config) */
    if ((retval = detect_tiff_organization(tiff, geotiff_info)))
        return retval;

    /* Get data type information */
    TIFFGetField(tiff, TIFFTAG_BITSPERSAMPLE, &bits_per_sample);
    TIFFGetField(tiff, TIFFTAG_SAMPLEFORMAT, &sample_format);

    /* Map TIFF data type to NetCDF type */
    if (sample_format == SAMPLEFORMAT_UINT)
    {
        if (bits_per_sample == 8)
        {
            xtype = NC_UBYTE;
            type_size = sizeof(unsigned char);
            strcpy(type_name, "ubyte");
        }
        else if (bits_per_sample == 16)
        {
            xtype = NC_USHORT;
            type_size = sizeof(unsigned short);
            strcpy(type_name, "ushort");
        }
        else if (bits_per_sample == 32)
        {
            xtype = NC_UINT;
            type_size = sizeof(unsigned int);
            strcpy(type_name, "uint");
        }
        else
        {
            xtype = NC_UBYTE;
            type_size = sizeof(unsigned char);
            strcpy(type_name, "ubyte");
        }
    }
    else if (sample_format == SAMPLEFORMAT_INT)
    {
        if (bits_per_sample == 8)
        {
            xtype = NC_BYTE;
            type_size = sizeof(char);
            strcpy(type_name, "byte");
        }
        else if (bits_per_sample == 16)
        {
            xtype = NC_SHORT;
            type_size = sizeof(short);
            strcpy(type_name, "short");
        }
        else if (bits_per_sample == 32)
        {
            xtype = NC_INT;
            type_size = sizeof(int);
            strcpy(type_name, "int");
        }
        else
        {
            xtype = NC_SHORT;
            type_size = sizeof(short);
            strcpy(type_name, "short");
        }
    }
    else if (sample_format == SAMPLEFORMAT_IEEEFP)
    {
        if (bits_per_sample == 32)
        {
            xtype = NC_FLOAT;
            type_size = sizeof(float);
            strcpy(type_name, "float");
        }
        else if (bits_per_sample == 64)
        {
            xtype = NC_DOUBLE;
            type_size = sizeof(double);
            strcpy(type_name, "double");
        }
        else
        {
            xtype = NC_FLOAT;
            type_size = sizeof(float);
            strcpy(type_name, "float");
        }
    }

    /* Determine endianness from GeoTIFF file */
    endianness = geotiff_info->is_little_endian ? NC_ENDIAN_LITTLE : NC_ENDIAN_BIG;

    /* Create dimensions */
    if ((retval = nc4_dim_list_add(grp, "x", width, -1, &dim_x)))
        return retval;
    if ((retval = nc4_dim_list_add(grp, "y", height, -1, &dim_y)))
        return retval;

    /* Create band dimension if multi-band */
    if (samples_per_pixel > 1)
    {
        if ((retval = nc4_dim_list_add(grp, "band", samples_per_pixel, -1, &dim_band)))
            return retval;
    }

    /* Create variable for raster data */
    if (samples_per_pixel > 1)
    {
        /* Multi-band: 3D variable (band, y, x) */
        if ((retval = nc4_var_list_add_full(grp, "data", 3, xtype, endianness,
                                            type_size, type_name, NULL, NC_TRUE,
                                            NULL, NULL, &var)))
            return retval;
        /* dimids and dim arrays already allocated by nc4_var_list_add */
        var->dim[0] = dim_band;
        var->dimids[0] = dim_band->hdr.id;
        var->dim[1] = dim_y;
        var->dimids[1] = dim_y->hdr.id;
        var->dim[2] = dim_x;
        var->dimids[2] = dim_x->hdr.id;
    }
    else
    {
        /* Single-band: 2D variable (y, x) */
        if ((retval = nc4_var_list_add_full(grp, "data", 2, xtype, endianness,
                                            type_size, type_name, NULL, NC_TRUE,
                                            NULL, NULL, &var)))
            return retval;
        /* dimids and dim arrays already allocated by nc4_var_list_add */
        var->dim[0] = dim_y;
        var->dimids[0] = dim_y->hdr.id;
        var->dim[1] = dim_x;
        var->dimids[1] = dim_x->hdr.id;
    }

    /* Extract CRS information if GTIFNew succeeded */
    if (gtif)
    {
        /* Extract CRS parameters using helper function */
        int retval = extract_crs_parameters(gtif, &geotiff_info->crs_info);
        if (retval == NC_NOERR)
        {
            /* Validate CRS completeness */
            retval = validate_crs_completeness(&geotiff_info->crs_info);
            if (retval == NC_NOERR)
            {
                /* Map CRS parameters to NetCDF attributes */
                NC_ATT_INFO_T *att = NULL;
                char att_name[NC_MAX_NAME + 1];
                void *att_data = NULL;
                
                /* Add EPSG code attribute */
                if (geotiff_info->crs_info.epsg_code != 0)
                {
                    strcpy(att_name, "geotiff_epsg_code");
                    att_data = malloc(sizeof(int));
                    if (att_data)
                    {
                        *(int*)att_data = geotiff_info->crs_info.epsg_code;
                        if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                        {
                            att->data = att_data;
                            att->len = 1;
                            att->nc_typeid = NC_INT;
                        }
                        else
                        {
                            free(att_data);
                        }
                    }
                }
                
                /* Add CRS name attribute */
                if (strlen(geotiff_info->crs_info.crs_name) > 0)
                {
                    strcpy(att_name, "geotiff_crs_name");
                    att_data = malloc(strlen(geotiff_info->crs_info.crs_name) + 1);
                    if (att_data)
                    {
                        strcpy(att_data, geotiff_info->crs_info.crs_name);
                        if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                        {
                            att->data = att_data;
                            att->len = strlen(geotiff_info->crs_info.crs_name) + 1;
                            att->nc_typeid = NC_CHAR;
                        }
                        else
                        {
                            free(att_data);
                        }
                    }
                }
                
                /* Add ellipsoid parameters */
                if (geotiff_info->crs_info.semi_major_axis != 0.0)
                {
                    strcpy(att_name, "geotiff_semi_major_axis");
                    att_data = malloc(sizeof(double));
                    if (att_data)
                    {
                        *(double*)att_data = geotiff_info->crs_info.semi_major_axis;
                        if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                        {
                            att->data = att_data;
                            att->len = 1;
                            att->nc_typeid = NC_DOUBLE;
                        }
                        else
                        {
                            free(att_data);
                        }
                    }
                }
                
                if (geotiff_info->crs_info.inverse_flattening != 0.0)
                {
                    strcpy(att_name, "geotiff_inverse_flattening");
                    att_data = malloc(sizeof(double));
                    if (att_data)
                    {
                        *(double*)att_data = geotiff_info->crs_info.inverse_flattening;
                        if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                        {
                            att->data = att_data;
                            att->len = 1;
                            att->nc_typeid = NC_DOUBLE;
                        }
                        else
                        {
                            free(att_data);
                        }
                    }
                }
                
                /* Add projection parameters for projected CRS */
                if (geotiff_info->crs_info.crs_type == NC_GEOTIFF_CRS_PROJECTED)
                {
                    if (geotiff_info->crs_info.false_easting != 0.0)
                    {
                        strcpy(att_name, "geotiff_false_easting");
                        att_data = malloc(sizeof(double));
                        if (att_data)
                        {
                            *(double*)att_data = geotiff_info->crs_info.false_easting;
                            if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                            {
                                att->data = att_data;
                                att->len = 1;
                                att->nc_typeid = NC_DOUBLE;
                            }
                            else
                            {
                                free(att_data);
                            }
                        }
                    }
                    
                    if (geotiff_info->crs_info.false_northing != 0.0)
                    {
                        strcpy(att_name, "geotiff_false_northing");
                        att_data = malloc(sizeof(double));
                        if (att_data)
                        {
                            *(double*)att_data = geotiff_info->crs_info.false_northing;
                            if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                            {
                                att->data = att_data;
                                att->len = 1;
                                att->nc_typeid = NC_DOUBLE;
                            }
                            else
                            {
                                free(att_data);
                            }
                        }
                    }
                    
                    if (geotiff_info->crs_info.scale_factor != 0.0)
                    {
                        strcpy(att_name, "geotiff_scale_factor");
                        att_data = malloc(sizeof(double));
                        if (att_data)
                        {
                            *(double*)att_data = geotiff_info->crs_info.scale_factor;
                            if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                            {
                                att->data = att_data;
                                att->len = 1;
                                att->nc_typeid = NC_DOUBLE;
                            }
                            else
                            {
                                free(att_data);
                            }
                        }
                    }
                    
                    if (geotiff_info->crs_info.central_meridian != 0.0)
                    {
                        strcpy(att_name, "geotiff_central_meridian");
                        att_data = malloc(sizeof(double));
                        if (att_data)
                        {
                            *(double*)att_data = geotiff_info->crs_info.central_meridian;
                            if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                            {
                                att->data = att_data;
                                att->len = 1;
                                att->nc_typeid = NC_DOUBLE;
                            }
                            else
                            {
                                free(att_data);
                            }
                        }
                    }
                    
                    if (geotiff_info->crs_info.latitude_of_origin != 0.0)
                    {
                        strcpy(att_name, "geotiff_latitude_of_origin");
                        att_data = malloc(sizeof(double));
                        if (att_data)
                        {
                            *(double*)att_data = geotiff_info->crs_info.latitude_of_origin;
                            if ((retval = nc4_att_list_add(grp->att, att_name, &att)) == NC_NOERR)
                            {
                                att->data = att_data;
                                att->len = 1;
                                att->nc_typeid = NC_DOUBLE;
                            }
                            else
                            {
                                free(att_data);
                            }
                        }
                    }
                }
            }
            else
            {
                /* CRS validation failed - continue without CRS but log warning */
                /* In a production environment, you might want to add logging here */
            }
        }
        else
        {
            /* CRS extraction failed - continue without CRS but this is expected for files without CRS */
        }
    }

    return NC_NOERR;
}

/**
 * @internal Extract CRS parameters from GTIF definition.
 *
 * This function parses the GTIFDefn structure and extracts relevant CRS
 * parameters into a structured format suitable for NetCDF attribute creation.
 *
 * @param gtif GTIF handle.
 * @param crs_info CRS info structure to populate.
 * @return NC_NOERR on success, error code otherwise.
 */
int
extract_crs_parameters(GTIF *gtif, NC_GEOTIFF_CRS_INFO_T *crs_info)
{
    GTIFDefn defn;
    
    if (!gtif || !crs_info)
        return NC_EINVAL;
    
    /* Initialize CRS info structure */
    memset(crs_info, 0, sizeof(NC_GEOTIFF_CRS_INFO_T));
    crs_info->crs_type = NC_GEOTIFF_CRS_UNKNOWN;
    
    /* Get GTIF definition */
    memset(&defn, 0, sizeof(GTIFDefn));
    if (!GTIFGetDefn(gtif, &defn))
        return NC_NOERR; /* No CRS data available, but not an error */
    
    /* Extract basic CRS information */
    /* Note: GTIFDefn doesn't have EPSGCode field directly */
    crs_info->epsg_code = 0; /* Could be derived from other fields if needed */
    
    /* Determine CRS type */
    if (defn.Model == ModelTypeGeographic)
    {
        crs_info->crs_type = NC_GEOTIFF_CRS_GEOGRAPHIC;
        strncpy(crs_info->crs_name, "Geographic", NC_MAX_NAME);
    }
    else if (defn.Model == ModelTypeProjected)
    {
        crs_info->crs_type = NC_GEOTIFF_CRS_PROJECTED;
        strncpy(crs_info->crs_name, "Projected", NC_MAX_NAME);
    }
    else
    {
        crs_info->crs_type = NC_GEOTIFF_CRS_UNKNOWN;
        strncpy(crs_info->crs_name, "Unknown", NC_MAX_NAME);
    }
    
    /* Extract ellipsoid parameters */
    if (defn.SemiMajor != 0.0)
        crs_info->semi_major_axis = defn.SemiMajor;
    
    /* Calculate inverse flattening from semi-major and semi-minor axes */
    if (defn.SemiMajor != 0.0 && defn.SemiMinor != 0.0 && defn.SemiMajor != defn.SemiMinor)
        crs_info->inverse_flattening = defn.SemiMajor / (defn.SemiMajor - defn.SemiMinor);
    
    /* Extract projection parameters for projected CRS */
    if (crs_info->crs_type == NC_GEOTIFF_CRS_PROJECTED)
    {
        /* Look for common projection parameters in ProjParm array */
        for (int i = 0; i < defn.nParms; i++)
        {
            switch (defn.ProjParmId[i])
            {
                case ProjFalseEastingGeoKey:
                    crs_info->false_easting = defn.ProjParm[i];
                    break;
                case ProjFalseNorthingGeoKey:
                    crs_info->false_northing = defn.ProjParm[i];
                    break;
                case ProjScaleAtOriginGeoKey:
                    crs_info->scale_factor = defn.ProjParm[i];
                    break;
                case ProjNatOriginLongGeoKey:
                    crs_info->central_meridian = defn.ProjParm[i];
                    break;
                case ProjNatOriginLatGeoKey:
                    crs_info->latitude_of_origin = defn.ProjParm[i];
                    break;
                default:
                    /* Other projection parameters not used in this implementation */
                    break;
            }
        }
    }
    
    return NC_NOERR;
}

/**
 * @internal Map GeoTIFF CRS parameters to CF-compliant attributes.
 *
 * This function converts the extracted CRS parameters into NetCDF attributes
 * following CF conventions where applicable.
 *
 * @param crs_info CRS info structure.
 * @param atts Pointer to array of attributes (output).
 * @param num_atts Pointer to number of attributes (output).
 * @return NC_NOERR on success, error code otherwise.
 */
int
map_geotiff_to_cf_attributes(const NC_GEOTIFF_CRS_INFO_T *crs_info, 
                           NC_ATT_INFO_T **atts, int *num_atts)
{
    if (!crs_info || !atts || !num_atts)
        return NC_EINVAL;
    
    *atts = NULL;
    *num_atts = 0;
    
    /* Only create attributes if we have valid CRS information */
    if (crs_info->crs_type == NC_GEOTIFF_CRS_UNKNOWN)
        return NC_NOERR;
    
    /* Calculate number of attributes needed */
    int att_count = 0;
    if (crs_info->epsg_code != 0) att_count++;
    if (strlen(crs_info->crs_name) > 0) att_count++;
    if (crs_info->semi_major_axis != 0.0) att_count++;
    if (crs_info->inverse_flattening != 0.0) att_count++;
    if (crs_info->false_easting != 0.0) att_count++;
    if (crs_info->false_northing != 0.0) att_count++;
    if (crs_info->scale_factor != 0.0) att_count++;
    if (crs_info->central_meridian != 0.0) att_count++;
    if (crs_info->latitude_of_origin != 0.0) att_count++;
    
    if (att_count == 0)
        return NC_NOERR;
    
    /* Allocate attribute array */
    *atts = calloc(att_count, sizeof(NC_ATT_INFO_T));
    if (!*atts)
        return NC_ENOMEM;
    
    *num_atts = att_count;
    int att_idx = 0;
    
    /* Add EPSG code attribute */
    if (crs_info->epsg_code != 0)
    {
        NC_ATT_INFO_T *att = &(*atts)[att_idx++];
        snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_epsg_code");
        att->nc_typeid = NC_INT;
        att->len = 1;
        att->data = malloc(sizeof(int));
        if (!att->data)
            return NC_ENOMEM;
        *(int*)att->data = crs_info->epsg_code;
    }
    
    /* Add CRS name attribute */
    if (strlen(crs_info->crs_name) > 0)
    {
        NC_ATT_INFO_T *att = &(*atts)[att_idx++];
        snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_crs_name");
        att->nc_typeid = NC_CHAR;
        att->len = strlen(crs_info->crs_name) + 1;
        att->data = malloc(att->len);
        if (!att->data)
            return NC_ENOMEM;
        strcpy(att->data, crs_info->crs_name);
    }
    
    /* Add ellipsoid parameters */
    if (crs_info->semi_major_axis != 0.0)
    {
        NC_ATT_INFO_T *att = &(*atts)[att_idx++];
        snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_semi_major_axis");
        att->nc_typeid = NC_DOUBLE;
        att->len = 1;
        att->data = malloc(sizeof(double));
        if (!att->data)
            return NC_ENOMEM;
        *(double*)att->data = crs_info->semi_major_axis;
    }
    
    if (crs_info->inverse_flattening != 0.0)
    {
        NC_ATT_INFO_T *att = &(*atts)[att_idx++];
        snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_inverse_flattening");
        att->nc_typeid = NC_DOUBLE;
        att->len = 1;
        att->data = malloc(sizeof(double));
        if (!att->data)
            return NC_ENOMEM;
        *(double*)att->data = crs_info->inverse_flattening;
    }
    
    /* Add projection parameters for projected CRS */
    if (crs_info->crs_type == NC_GEOTIFF_CRS_PROJECTED)
    {
        if (crs_info->false_easting != 0.0)
        {
            NC_ATT_INFO_T *att = &(*atts)[att_idx++];
            snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_false_easting");
            att->nc_typeid = NC_DOUBLE;
            att->len = 1;
            att->data = malloc(sizeof(double));
            if (!att->data)
                return NC_ENOMEM;
            *(double*)att->data = crs_info->false_easting;
        }
        
        if (crs_info->false_northing != 0.0)
        {
            NC_ATT_INFO_T *att = &(*atts)[att_idx++];
            snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_false_northing");
            att->nc_typeid = NC_DOUBLE;
            att->len = 1;
            att->data = malloc(sizeof(double));
            if (!att->data)
                return NC_ENOMEM;
            *(double*)att->data = crs_info->false_northing;
        }
        
        if (crs_info->scale_factor != 0.0)
        {
            NC_ATT_INFO_T *att = &(*atts)[att_idx++];
            snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_scale_factor");
            att->nc_typeid = NC_DOUBLE;
            att->len = 1;
            att->data = malloc(sizeof(double));
            if (!att->data)
                return NC_ENOMEM;
            *(double*)att->data = crs_info->scale_factor;
        }
        
        if (crs_info->central_meridian != 0.0)
        {
            NC_ATT_INFO_T *att = &(*atts)[att_idx++];
            snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_central_meridian");
            att->nc_typeid = NC_DOUBLE;
            att->len = 1;
            att->data = malloc(sizeof(double));
            if (!att->data)
                return NC_ENOMEM;
            *(double*)att->data = crs_info->central_meridian;
        }
        
        if (crs_info->latitude_of_origin != 0.0)
        {
            NC_ATT_INFO_T *att = &(*atts)[att_idx++];
            snprintf(att->hdr.name, NC_MAX_NAME, "geotiff_latitude_of_origin");
            att->nc_typeid = NC_DOUBLE;
            att->len = 1;
            att->data = malloc(sizeof(double));
            if (!att->data)
                return NC_ENOMEM;
            *(double*)att->data = crs_info->latitude_of_origin;
        }
    }
    
    return NC_NOERR;
}

/**
 * @internal Validate completeness of CRS information.
 *
 * This function checks if the extracted CRS information contains
 * the minimum required parameters for the CRS type.
 *
 * @param crs_info CRS info structure to validate.
 * @return NC_NOERR if complete, NC_EINVAL if incomplete.
 */
int
validate_crs_completeness(const NC_GEOTIFF_CRS_INFO_T *crs_info)
{
    if (!crs_info)
        return NC_EINVAL;
    
    /* Unknown CRS type is considered incomplete but not an error */
    if (crs_info->crs_type == NC_GEOTIFF_CRS_UNKNOWN)
        return NC_NOERR;
    
    /* For foundation implementation, be more permissive:
       - For geographic CRS, accept if we have any ellipsoid info
       - For projected CRS, accept if we have basic CRS info
       - Don't require all parameters to be present */
    
    /* For geographic CRS, at least one ellipsoid parameter should be present */
    if (crs_info->crs_type == NC_GEOTIFF_CRS_GEOGRAPHIC)
    {
        if (crs_info->semi_major_axis == 0.0)
            return NC_EINVAL; /* Need at least semi-major axis */
    }
    
    /* For projected CRS, at least CRS type should be identified */
    if (crs_info->crs_type == NC_GEOTIFF_CRS_PROJECTED)
    {
        /* For foundation implementation, accept projected CRS even without full parameters */
        /* We only need to know it's a projected CRS */
        if (crs_info->semi_major_axis == 0.0)
            return NC_EINVAL; /* Still need basic ellipsoid info */
    }
    
    return NC_NOERR;
}

/**
 * @internal Detect TIFF file organization (tiled vs striped, planar config).
 *
 * This function queries TIFF tags to determine the file's internal organization
 * and caches the results in the NC_GEOTIFF_FILE_INFO_T structure for efficient
 * data reading operations.
 *
 * @param tiff TIFF file handle.
 * @param geotiff_info GeoTIFF file info structure to populate.
 *
 * @return NC_NOERR on success.
 * @return NC_EINVAL if parameters are invalid.
 * @return NC_EHDFERR if TIFF tag reading fails.
 *
 * @author Edward Hartnett
 */
static int
detect_tiff_organization(TIFF *tiff, NC_GEOTIFF_FILE_INFO_T *geotiff_info)
{
    uint32_t tile_width = 0, tile_height = 0;
    uint32_t rows_per_strip = 0;
    uint16_t planar_config = PLANARCONFIG_CONTIG;

    if (!tiff || !geotiff_info)
        return NC_EINVAL;

    /* Check if file is tiled */
    if (TIFFIsTiled(tiff))
    {
        geotiff_info->is_tiled = 1;

        /* Get tile dimensions */
        if (!TIFFGetField(tiff, TIFFTAG_TILEWIDTH, &tile_width))
            return NC_EHDFERR;
        if (!TIFFGetField(tiff, TIFFTAG_TILELENGTH, &tile_height))
            return NC_EHDFERR;

        geotiff_info->tile_width = tile_width;
        geotiff_info->tile_height = tile_height;
        geotiff_info->rows_per_strip = 0;
    }
    else
    {
        /* File is striped */
        geotiff_info->is_tiled = 0;
        geotiff_info->tile_width = 0;
        geotiff_info->tile_height = 0;

        /* Get rows per strip */
        if (!TIFFGetField(tiff, TIFFTAG_ROWSPERSTRIP, &rows_per_strip))
        {
            /* If not specified, default to image height (single strip) */
            rows_per_strip = geotiff_info->image_height;
        }
        geotiff_info->rows_per_strip = rows_per_strip;
    }

    /* Get planar configuration */
    if (!TIFFGetField(tiff, TIFFTAG_PLANARCONFIG, &planar_config))
    {
        /* Default to contiguous (pixel-interleaved) if not specified */
        planar_config = PLANARCONFIG_CONTIG;
    }
    geotiff_info->planar_config = planar_config;

    return NC_NOERR;
}

/**
 * @internal Allocate buffer for reading TIFF tiles or scanlines.
 *
 * Allocates a buffer sized appropriately for the TIFF file organization
 * (tile or scanline based). The buffer size is calculated based on the
 * file's tile/strip dimensions and data type size.
 *
 * @param geotiff_info GeoTIFF file info structure.
 * @param type_size Size of one data element in bytes.
 *
 * @return Pointer to allocated buffer, or NULL on failure.
 *
 * @author Edward Hartnett
 */
static void *
allocate_read_buffer(NC_GEOTIFF_FILE_INFO_T *geotiff_info, size_t type_size)
{
    size_t buffer_size;

    if (!geotiff_info || type_size == 0)
        return NULL;

    if (geotiff_info->is_tiled)
    {
        /* Allocate buffer for one tile */
        buffer_size = (size_t)geotiff_info->tile_width *
                      (size_t)geotiff_info->tile_height *
                      (size_t)geotiff_info->samples_per_pixel *
                      type_size;
    }
    else
    {
        /* Allocate buffer for one scanline */
        buffer_size = (size_t)geotiff_info->image_width *
                      (size_t)geotiff_info->samples_per_pixel *
                      type_size;
    }

    /* Sanity check on buffer size */
    if (buffer_size > MAX_BUFFER_SIZE)
        return NULL;

    return malloc(buffer_size);
}

/**
 * @internal Free a read buffer.
 *
 * @param buffer Buffer to free.
 *
 * @author Edward Hartnett
 */
static void
free_read_buffer(void *buffer)
{
    if (buffer)
        free(buffer);
}

/**
 * @internal Validate hyperslab coordinates against variable dimensions.
 *
 * Checks that the requested hyperslab (defined by start and count arrays)
 * is within the bounds of the variable's dimensions. Returns NC_EEDGE if
 * any coordinates are out of bounds.
 *
 * @param var Variable info structure.
 * @param start Start indices for each dimension.
 * @param count Count of values to read in each dimension.
 *
 * @return NC_NOERR if hyperslab is valid.
 * @return NC_EINVAL if parameters are invalid.
 * @return NC_EEDGE if hyperslab is out of bounds.
 *
 * @author Edward Hartnett
 */
static int
validate_hyperslab(NC_VAR_INFO_T *var, const size_t *start, const size_t *count)
{
    int d;

    if (!var || !start || !count)
        return NC_EINVAL;

    /* Check each dimension */
    for (d = 0; d < (int)var->ndims; d++)
    {
        /* Check if start is within bounds */
        if (start[d] >= var->dim[d]->len)
            return NC_EEDGE;

        /* Check if start + count exceeds dimension length */
        if (start[d] + count[d] > var->dim[d]->len)
            return NC_EEDGE;

        /* Check for zero count (invalid) */
        if (count[d] == 0)
            return NC_EEDGE;
    }

    return NC_NOERR;
}

/**
 * @internal Read a single scanline from a striped TIFF file.
 *
 * @param tiff TIFF file handle.
 * @param row Row number to read.
 * @param buffer Buffer to read into (must be pre-allocated).
 * @param samples_per_pixel Number of samples per pixel.
 *
 * @return NC_NOERR on success, NC_EHDFERR on TIFF error.
 *
 * @author Edward Hartnett
 */
static int
read_scanline(TIFF *tiff, uint32_t row, void *buffer, uint16_t samples_per_pixel)
{
    (void)samples_per_pixel;
    if (!tiff || !buffer)
        return NC_EINVAL;

    /* For single-band or PLANARCONFIG_CONTIG, read the scanline */
    if (TIFFReadScanline(tiff, buffer, row, 0) < 0)
        return NC_EHDFERR;

    return NC_NOERR;
}

/**
 * @internal Read a tile from a tiled TIFF file.
 *
 * @param tiff TIFF file handle.
 * @param tile_x Tile column index.
 * @param tile_y Tile row index.
 * @param buffer Buffer to read into (must be pre-allocated).
 *
 * @return NC_NOERR on success, NC_EHDFERR on TIFF error.
 *
 * @author Edward Hartnett
 */
static int
read_tile(TIFF *tiff, uint32_t tile_x, uint32_t tile_y, void *buffer)
{
    if (!tiff || !buffer)
        return NC_EINVAL;

    /* Read the tile */
    if (TIFFReadTile(tiff, buffer, tile_x, tile_y, 0, 0) < 0)
        return NC_EHDFERR;

    return NC_NOERR;
}

/**
 * @internal Read a hyperslab from a single-band GeoTIFF variable.
 *
 * This function implements the core reading logic for single-band (2D) rasters.
 * It handles both tiled and striped TIFF organizations.
 *
 * @param h5 File info structure.
 * @param var Variable info structure.
 * @param start Start indices [y, x].
 * @param count Count of values [height, width].
 * @param value Output buffer.
 * @param type_size Size of data type in bytes.
 *
 * @return NC_NOERR on success, error code on failure.
 *
 * @author Edward Hartnett
 */
static int
read_single_band_hyperslab(NC_FILE_INFO_T *h5, NC_VAR_INFO_T *var,
                           const size_t *start, const size_t *count,
                           void *value, size_t type_size)
{
    NC_GEOTIFF_FILE_INFO_T *geotiff_info;
    TIFF *tiff;
    void *read_buffer = NULL;
    size_t start_y, start_x, count_y, count_x;
    int retval = NC_NOERR;

    (void)var;

    /* Get GeoTIFF info */
    geotiff_info = (NC_GEOTIFF_FILE_INFO_T *)h5->format_file_info;
    if (!geotiff_info || !geotiff_info->tiff_handle)
        return NC_EINVAL;

    tiff = (TIFF *)geotiff_info->tiff_handle;

    /* Extract start and count for 2D array [y, x] */
    start_y = start[0];
    start_x = start[1];
    count_y = count[0];
    count_x = count[1];

    /* Allocate read buffer */
    read_buffer = allocate_read_buffer(geotiff_info, type_size);
    if (!read_buffer)
        return NC_ENOMEM;

    if (geotiff_info->is_tiled)
    {
        /* Tiled reading */
        uint32_t tile_width = geotiff_info->tile_width;
        uint32_t tile_height = geotiff_info->tile_height;
        size_t y;

        for (y = 0; y < count_y; y++)
        {
            size_t row = start_y + y;
            uint32_t tile_row = row / tile_height;
            uint32_t row_in_tile = row % tile_height;
            size_t x = 0;

            while (x < count_x)
            {
                size_t col = start_x + x;
                uint32_t tile_col = col / tile_width;
                uint32_t col_in_tile = col % tile_width;
                size_t pixels_in_tile = tile_width - col_in_tile;
                size_t pixels_to_copy = (pixels_in_tile < (count_x - x)) ?
                                       pixels_in_tile : (count_x - x);

                /* Read tile */
                if ((retval = read_tile(tiff, tile_col * tile_width,
                                       tile_row * tile_height, read_buffer)))
                {
                    free_read_buffer(read_buffer);
                    return retval;
                }

                /* Copy relevant portion */
                unsigned char *dst = (unsigned char *)value +
                                    (y * count_x + x) * type_size;
                unsigned char *src = (unsigned char *)read_buffer +
                                    (row_in_tile * tile_width + col_in_tile) * type_size;
                memcpy(dst, src, pixels_to_copy * type_size);

                x += pixels_to_copy;
            }
        }
    }
    else
    {
        /* Striped (scanline) reading */
        size_t y;

        for (y = 0; y < count_y; y++)
        {
            uint32_t row = start_y + y;

            /* Read scanline */
            if ((retval = read_scanline(tiff, row, read_buffer,
                                       geotiff_info->samples_per_pixel)))
            {
                free_read_buffer(read_buffer);
                return retval;
            }

            /* Copy relevant portion */
            unsigned char *dst = (unsigned char *)value + y * count_x * type_size;
            unsigned char *src = (unsigned char *)read_buffer + start_x * type_size;
            memcpy(dst, src, count_x * type_size);
        }
    }

    free_read_buffer(read_buffer);
    return NC_NOERR;
}

/**
 * @internal Read a hyperslab from a multi-band GeoTIFF variable.
 *
 * This function implements reading for multi-band (3D) rasters, handling both
 * PLANARCONFIG_CONTIG (pixel-interleaved) and PLANARCONFIG_SEPARATE (band-interleaved).
 *
 * @param h5 File info structure.
 * @param var Variable info structure.
 * @param start Start indices [band, y, x].
 * @param count Count of values [nbands, height, width].
 * @param value Output buffer.
 * @param type_size Size of data type in bytes.
 *
 * @return NC_NOERR on success, error code on failure.
 *
 * @author Edward Hartnett
 */
static int
read_multi_band_hyperslab(NC_FILE_INFO_T *h5, NC_VAR_INFO_T *var,
                          const size_t *start, const size_t *count,
                          void *value, size_t type_size)
{
    NC_GEOTIFF_FILE_INFO_T *geotiff_info;
    TIFF *tiff;
    void *read_buffer = NULL;
    size_t start_band, start_y, start_x;
    size_t count_band, count_y, count_x;
    size_t band;
    int retval = NC_NOERR;

    (void)var;

    /* Get GeoTIFF info */
    geotiff_info = (NC_GEOTIFF_FILE_INFO_T *)h5->format_file_info;
    if (!geotiff_info || !geotiff_info->tiff_handle)
        return NC_EINVAL;

    tiff = (TIFF *)geotiff_info->tiff_handle;

    /* Extract start and count for 3D array [band, y, x] */
    start_band = start[0];
    start_y = start[1];
    start_x = start[2];
    count_band = count[0];
    count_y = count[1];
    count_x = count[2];

    /* Allocate read buffer */
    read_buffer = allocate_read_buffer(geotiff_info, type_size);
    if (!read_buffer)
        return NC_ENOMEM;

    if (geotiff_info->planar_config == PLANARCONFIG_SEPARATE)
    {
        /* Band-interleaved: each band stored separately */
        /* Read each band separately using TIFFReadScanline/TIFFReadTile with sample parameter */
        for (band = 0; band < count_band; band++)
        {
            uint16_t sample = start_band + band;
            size_t band_offset = band * count_y * count_x;

            if (geotiff_info->is_tiled)
            {
                /* Tiled reading for this band */
                uint32_t tile_width = geotiff_info->tile_width;
                uint32_t tile_height = geotiff_info->tile_height;
                size_t y;

                for (y = 0; y < count_y; y++)
                {
                    size_t row = start_y + y;
                    uint32_t tile_row = row / tile_height;
                    uint32_t row_in_tile = row % tile_height;
                    size_t x = 0;

                    while (x < count_x)
                    {
                        size_t col = start_x + x;
                        uint32_t tile_col = col / tile_width;
                        uint32_t col_in_tile = col % tile_width;
                        size_t pixels_in_tile = tile_width - col_in_tile;
                        size_t pixels_to_copy = (pixels_in_tile < (count_x - x)) ?
                                               pixels_in_tile : (count_x - x);

                        /* Read tile for this band */
                        if (TIFFReadTile(tiff, read_buffer, tile_col * tile_width,
                                        tile_row * tile_height, 0, sample) < 0)
                        {
                            free_read_buffer(read_buffer);
                            return NC_EHDFERR;
                        }

                        /* Copy relevant portion */
                        unsigned char *dst = (unsigned char *)value +
                                            (band_offset + y * count_x + x) * type_size;
                        unsigned char *src = (unsigned char *)read_buffer +
                                            (row_in_tile * tile_width + col_in_tile) * type_size;
                        memcpy(dst, src, pixels_to_copy * type_size);

                        x += pixels_to_copy;
                    }
                }
            }
            else
            {
                /* Striped reading for this band */
                size_t y;

                for (y = 0; y < count_y; y++)
                {
                    uint32_t row = start_y + y;

                    /* Read scanline for this band */
                    if (TIFFReadScanline(tiff, read_buffer, row, sample) < 0)
                    {
                        free_read_buffer(read_buffer);
                        return NC_EHDFERR;
                    }

                    /* Copy relevant portion */
                    unsigned char *dst = (unsigned char *)value +
                                        (band_offset + y * count_x) * type_size;
                    unsigned char *src = (unsigned char *)read_buffer + start_x * type_size;
                    memcpy(dst, src, count_x * type_size);
                }
            }
        }
    }
    else
    {
        /* PLANARCONFIG_CONTIG: pixel-interleaved (RGBRGBRGB...) */
        /* Read data and de-interleave into output buffer */
        if (geotiff_info->is_tiled)
        {
            /* Tiled reading */
            uint32_t tile_width = geotiff_info->tile_width;
            uint32_t tile_height = geotiff_info->tile_height;
            uint16_t samples_per_pixel = geotiff_info->samples_per_pixel;
            size_t y;

            for (y = 0; y < count_y; y++)
            {
                size_t row = start_y + y;
                uint32_t tile_row = row / tile_height;
                uint32_t row_in_tile = row % tile_height;
                size_t x = 0;

                while (x < count_x)
                {
                    size_t col = start_x + x;
                    uint32_t tile_col = col / tile_width;
                    uint32_t col_in_tile = col % tile_width;
                    size_t pixels_in_tile = tile_width - col_in_tile;
                    size_t pixels_to_copy = (pixels_in_tile < (count_x - x)) ?
                                           pixels_in_tile : (count_x - x);

                    /* Read tile */
                    if ((retval = read_tile(tiff, tile_col * tile_width,
                                           tile_row * tile_height, read_buffer)))
                    {
                        free_read_buffer(read_buffer);
                        return retval;
                    }

                    /* De-interleave pixels for requested bands */
                    size_t p;
                    for (p = 0; p < pixels_to_copy; p++)
                    {
                        size_t src_pixel = (row_in_tile * tile_width + col_in_tile + p) * samples_per_pixel;

                        for (band = 0; band < count_band; band++)
                        {
                            size_t src_offset = (src_pixel + start_band + band) * type_size;
                            size_t dst_offset = (band * count_y * count_x + y * count_x + x + p) * type_size;
                            memcpy((unsigned char *)value + dst_offset,
                                   (unsigned char *)read_buffer + src_offset,
                                   type_size);
                        }
                    }

                    x += pixels_to_copy;
                }
            }
        }
        else
        {
            /* Striped reading */
            uint16_t samples_per_pixel = geotiff_info->samples_per_pixel;
            size_t y;

            for (y = 0; y < count_y; y++)
            {
                uint32_t row = start_y + y;

                /* Read scanline */
                if ((retval = read_scanline(tiff, row, read_buffer, samples_per_pixel)))
                {
                    free_read_buffer(read_buffer);
                    return retval;
                }

                /* De-interleave pixels for requested bands */
                size_t x;
                for (x = 0; x < count_x; x++)
                {
                    size_t src_pixel = (start_x + x) * samples_per_pixel;

                    for (band = 0; band < count_band; band++)
                    {
                        size_t src_offset = (src_pixel + start_band + band) * type_size;
                        size_t dst_offset = (band * count_y * count_x + y * count_x + x) * type_size;
                        memcpy((unsigned char *)value + dst_offset,
                               (unsigned char *)read_buffer + src_offset,
                               type_size);
                    }
                }
            }
        }
    }

    free_read_buffer(read_buffer);
    return NC_NOERR;
}

/**
 * @internal Read data from a GeoTIFF variable (hyperslab).
 *
 * This function implements nc_get_vara for GeoTIFF files. It supports
 * reading rectangular subsets (hyperslabs) from both single-band (2D)
 * and multi-band (3D) rasters.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param startp Start indices.
 * @param countp Count of values to read.
 * @param value Pointer to data buffer.
 * @param memtype Memory type (currently ignored - reads in native type).
 *
 * @return NC_NOERR on success, error code on failure.
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_get_vara(int ncid, int varid, const size_t *startp,
                    const size_t *countp, void *value, nc_type memtype)
{
    NC_FILE_INFO_T *h5;
    NC_VAR_INFO_T *var;
    size_t type_size;
    int retval;

    (void)memtype;

    /* Get file and variable info */
    if ((retval = nc4_find_grp_h5_var(ncid, varid, &h5, NULL, &var)))
        return retval;

    if (!h5 || !var || !startp || !countp || !value)
        return NC_EINVAL;

    /* Validate hyperslab */
    if ((retval = validate_hyperslab(var, startp, countp)))
        return retval;

    /* Get type size */
    if ((retval = nc4_get_typelen_mem(h5, var->type_info->hdr.id, &type_size)))
        return retval;

    /* Support both 2D (single-band) and 3D (multi-band) variables */
    if (var->ndims == 2)
    {
        /* Single-band raster */
        return read_single_band_hyperslab(h5, var, startp, countp, value, type_size);
    }
    else if (var->ndims == 3)
    {
        /* Multi-band raster */
        return read_multi_band_hyperslab(h5, var, startp, countp, value, type_size);
    }
    else
    {
        /* Unsupported dimensionality */
        return NC_EINVAL;
    }
}

#endif /* HAVE_GEOTIFF */
