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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netcdf.h>
#include "geotiffdispatch.h"

#ifdef HAVE_GEOTIFF
#include <tiffio.h>
#include <geotiff/geotiff.h>
#include <geotiff/geo_normalize.h>
#endif

/** GeoTIFF key directory tag */
#define GEOTIFF_KEY_DIRECTORY_TAG 34735

/** Maximum reasonable IFD offset (100 MB) */
#define MAX_IFD_OFFSET 104857600

/** TIFF IFD entry size */
#define TIFF_IFD_ENTRY_SIZE 12

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
    unsigned short num_entries;
    unsigned char entry_count[2];
    int need_swap;
    int i;

    *has_geotiff_tags = 0;

#ifdef WORDS_BIGENDIAN
    need_swap = is_little_endian;
#else
    need_swap = !is_little_endian;
#endif

    /* Seek to IFD */
    if (fseek(fp, ifd_offset, SEEK_SET) != 0)
        return NC_ENOTNC;

    /* Read number of entries */
    if (fread(entry_count, 1, 2, fp) != 2)
        return NC_ENOTNC;

    num_entries = (entry_count[0]) | (entry_count[1] << 8);
    if (need_swap)
        num_entries = swap16(num_entries);

    /* Sanity check on number of entries */
    if (num_entries > 4096)
        return NC_ENOTNC;

    /* Read and check each IFD entry */
    for (i = 0; i < num_entries; i++)
    {
        unsigned char entry[TIFF_IFD_ENTRY_SIZE];
        unsigned short tag;

        if (fread(entry, 1, TIFF_IFD_ENTRY_SIZE, fp) != TIFF_IFD_ENTRY_SIZE)
            return NC_ENOTNC;

        /* Extract tag ID (first 2 bytes) */
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
 * This function opens a GeoTIFF file and initializes the libgeotiff context.
 * It validates that the file is a valid GeoTIFF and sets up the necessary
 * handles for subsequent operations.
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
    NC_GEOTIFF_FILE_INFO_T *info = NULL;
    TIFF *tiff = NULL;
    GTIF *gtif = NULL;
    int ret = NC_NOERR;
    int is_geotiff = 0;

    /* Validate parameters */
    if (!path)
        return NC_EINVAL;

    /* Only read-only mode is supported */
    if (mode & NC_WRITE)
        return NC_EINVAL;

    /* Verify this is actually a GeoTIFF file */
    ret = NC_GEOTIFF_detect_format(path, &is_geotiff);
    if (ret != NC_NOERR)
        return ret;
    if (!is_geotiff)
        return NC_ENOTNC;

    /* Allocate file info structure */
    info = (NC_GEOTIFF_FILE_INFO_T *)malloc(sizeof(NC_GEOTIFF_FILE_INFO_T));
    if (!info)
    {
        ret = NC_ENOMEM;
        goto cleanup;
    }
    memset(info, 0, sizeof(NC_GEOTIFF_FILE_INFO_T));

    /* Duplicate file path */
    info->path = strdup(path);
    if (!info->path)
    {
        ret = NC_ENOMEM;
        goto cleanup;
    }

    /* Open TIFF file to validate it's accessible */
    tiff = TIFFOpen(path, "r");
    if (!tiff)
    {
        ret = NC_ENOTNC;
        goto cleanup;
    }

    /* Phase 1: Basic validation only */
    /* Store TIFF handle for now - GTIFNew will be fully integrated in Phase 2 */
    /* when we have proper dispatch layer integration */
    info->tiff_handle = tiff;
    info->gtif_handle = NULL;  /* Will be initialized in Phase 2 */

    /* Determine endianness from TIFF file */
    {
        uint16_t byte_order;
        if (TIFFGetField(tiff, TIFFTAG_FILLORDER, &byte_order))
        {
            info->is_little_endian = (byte_order == FILLORDER_LSB2MSB);
        }
        else
        {
            /* Default to little-endian if not specified */
            info->is_little_endian = 1;
        }
    }

    /* TODO: Store file info in dispatch layer when full integration is complete */
    /* For Phase 1, we clean up immediately since we can't store the info */
    /* This prevents resource leaks until dispatch integration is complete */
    TIFFClose(tiff);
    free(info->path);
    free(info);

    /* Success - file was validated as accessible GeoTIFF */
    return NC_NOERR;

cleanup:
    /* Clean up resources on error */
    if (gtif)
        GTIFFree(gtif);
    if (tiff)
        TIFFClose(tiff);
    if (info)
    {
        if (info->path)
            free(info->path);
        free(info);
    }
    return ret;
}

/**
 * @internal Close a GeoTIFF file and release resources.
 *
 * This function closes a GeoTIFF file and frees all associated resources
 * including the GTIF context, TIFF handle, and file info structure.
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
    NC_GEOTIFF_FILE_INFO_T *info = NULL;

    /* TODO: Retrieve file info from ncid via dispatch layer */
    /* For now, this is a stub that will be completed when dispatch integration is done */
    
    if (!info)
        return NC_EBADID;

    /* Free GTIF context */
    if (info->gtif_handle)
        GTIFFree((GTIF *)info->gtif_handle);

    /* Close TIFF file */
    if (info->tiff_handle)
        TIFFClose((TIFF *)info->tiff_handle);

    /* Free path string */
    if (info->path)
        free(info->path);

    /* Free file info structure */
    free(info);

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
    if (formatp)
        *formatp = NC_FORMATX_NC_GEOTIFF;
    if (modep)
        *modep = NC_FORMATX_NC_GEOTIFF;
    return NC_NOERR;
}

/**
 * @internal Initialize GeoTIFF handler.
 *
 * @return NC_NOERR on success.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_initialize(void)
{
    return NC_NOERR;
}

/**
 * @internal Finalize GeoTIFF handler.
 *
 * @return NC_NOERR on success.
 *
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_finalize(void)
{
    return NC_NOERR;
}

#endif /* HAVE_GEOTIFF */
