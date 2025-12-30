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
    /* Propate the endianness to the variable */
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
 * Phase 2: Full dispatch layer integration with GTIFNew initialization
 * and metadata extraction.
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
    int is_geotiff = 0;
    FILE *fp = NULL;
    int is_little_endian;
    int is_bigtiff;
    unsigned int ifd_offset;

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
 * Phase 2: Retrieve file info from dispatch layer and cleanup.
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
    int ret;

    /* Find our metadata for this file */
    if ((ret = nc4_find_nc_grp_h5(ncid, &nc, &grp, (NC_FILE_INFO_T **)&h5)))
        return ret;
    
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

    /* Free file info structure */
    free(geotiff_info);
    h5->format_file_info = NULL;

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
    size_t type_size;
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
        if ((retval = nc4_var_set_ndims(var, 3)))
            return retval;
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
        if ((retval = nc4_var_set_ndims(var, 2)))
            return retval;
        var->dim[0] = dim_y;
        var->dimids[0] = dim_y->hdr.id;
        var->dim[1] = dim_x;
        var->dimids[1] = dim_x->hdr.id;
    }

    /* Extract CRS information if GTIFNew succeeded */
    if (gtif)
    {
        GTIFDefn defn;
        memset(&defn, 0, sizeof(GTIFDefn));
        
        if (GTIFGetDefn(gtif, &defn))
        {
            /* Store CRS information as attributes */
            /* Note: Full CRS attribute extraction will be implemented in Phase 3 */
            /* For now, just validate that GTIFGetDefn succeeded */
            if (defn.Model != 0)
            {
                /* CRS information is available but not yet extracted */
                /* This will be implemented when we add full CF-compliant grid mapping support */
            }
        }
    }

    return NC_NOERR;
}

/**
 * @internal Read data from a GeoTIFF variable.
 *
 * This is a Phase 3 function - stub implementation for Phase 2.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param startp Start indices.
 * @param countp Count of values to read.
 * @param value Pointer to data buffer.
 *
 * @return NC_ENOTNC4 Not yet implemented.
 * @author Edward Hartnett
 */
int
NC_GEOTIFF_get_vara(int ncid, int varid, const size_t *startp,
                    const size_t *countp, void *value, nc_type memtype)
{
    /* Phase 3: Implement raster data reading */
    return NC_ENOTNC4;
}

#endif /* HAVE_GEOTIFF */
