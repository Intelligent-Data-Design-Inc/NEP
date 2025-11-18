/**
 * @file
 * This is the code file for ncsqueeze.
 *
 * @author Edward Hartnett
 * @date Nov 13, 2025
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

/**
 * ncsqueeze supports compression filters for netCDF/HDF5 files which
 * are not natively supported by the netCDF C library.
 *
 * BZIP2
 *
 * Bzip2 is a free and open-source file compression program that uses
 * the Burrows–Wheeler algorithm. For more information see
 * https://www.sourceware.org/bzip2/ and
 * https://en.wikipedia.org/wiki/Bzip2.
 *
 * In C:
 * - nc_def_var_bzip2()
 * - nc_inq_var_bzip2()
 *
 * In Fortran:
 * - nf90_def_var_bzip2()
 * - nf90_inq_var_bzip2()
 *
 */

#include "config.h"
#include "ncsqueeze.h"
#include <hdf5.h>
#include <H5DSpublic.h>
#include <stdlib.h>

#if BUILD_BZIP2
/**
 * Turn on bzip2 compression for a variable.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param level From 1 to 9. Set the block size to 100k, 200k ... 900k
 * when compressing. (bzip2 default level is 9).
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_def_var_bzip2(int ncid, int varid, int level)
{
    unsigned int cd_value = level;
    int ret;

    /* Level must be between 1 and 9. */
    if (level < 1 || level > 9)
        return NC_EINVAL;

    if (!H5Zfilter_avail(BZIP2_ID))
    {
        printf ("bzip2 filter not available.\n");
        return NC_EFILTER;
    }
    /* Set up the bzip2 filter for this var. */
    if ((ret = nc_def_var_filter(ncid, varid, BZIP2_ID, 1, &cd_value)))
        return ret;

    return 0;
}

/**
 * Learn whether bzip2 compression is on for a variable, and, if so,
 * the level setting.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param bzip2p Pointer that gets a 0 if bzip2 is not in use for this
 * var, and a 1 if it is. Ignored if NULL.
 * @param levelp Pointer that gets the level setting (from 1 to 9), if
 * bzip2 is in use. Ignored if NULL.
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_inq_var_bzip2(int ncid, int varid, int *bzip2p, int *levelp)
{
    unsigned int level;
    size_t nparams;
    int bzip2 = 0; /* Is bzip2 in use? */
    int ret;

    {
	size_t nfilters;
	unsigned int *filterids;
	size_t f;
	
	/* Get filter information. */
	if ((ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, NULL)))
	    return ret;
	
	/* If there are no filters, we're done. */
	if (nfilters == 0)
	{
	    if (bzip2p)
		*bzip2p = 0;
	    return 0;
	}

	/* Allocate storage for filter IDs. */
	if (!(filterids = malloc(nfilters * sizeof(unsigned int))))
	    return NC_ENOMEM;

	/* Get the filter IDs. */
	if ((ret = nc_inq_var_filter_ids(ncid, varid, &nfilters, filterids)))
	    return ret;
    
	/* Check each filter to see if it is Bzip2. */
	for (f = 0; f < nfilters; f++)
	{
	    if (filterids[f] == BZIP2_ID)
		bzip2++;

	    /* If Bzip2 is in use, check parameter. */
	    if (bzip2)
	    {
	    
		if ((ret = nc_inq_var_filter_info(ncid, varid, filterids[f], &nparams, &level)))
		    return ret;

		/* For Bzip2, there is one parameter. */
		if (nparams != 1)
		    return NC_EFILTER;

		/* Tell the caller, if they want to know. */
		if (levelp)
		    *levelp = (int)level;

		/* Exit loop to report parameters (neglect remaining filters) */
		break;
	    }
	}

	/* Free resources. */
	free(filterids);

	if (bzip2p)
	    *bzip2p = bzip2;
    }

    return 0;
}

#endif /* BUILD_BZIP2 */
#if BUILD_LZ4

/**
 * Turn on lz4 compression for a variable.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param level From 1 to 9. Set the block size to 100k, 200k ... 900k
 * when compressing. (lz4 default level is 9).
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_def_var_lz4(int ncid, int varid, int level)
{
    unsigned int cd_value = level;
    int ret;

    /* Level must be between 1 and 9. */
    if (level < 1 || level > 9)
        return NC_EINVAL;

    if (!H5Zfilter_avail(LZ4_ID))
    {
        printf ("lz4 filter not available.\n");
        return NC_EFILTER;
    }

    /* Set up the lz4 filter for this var. */
    if ((ret = nc_def_var_filter(ncid, varid, LZ4_ID, 1, &cd_value)))
        return ret;

    return 0;
}

/**
 * Learn whether lz4 compression is on for a variable, and, if so,
 * the level setting.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param lz4p Pointer that gets a 0 if lz4 is not in use for this
 * var, and a 1 if it is. Ignored if NULL.
 * @param levelp Pointer that gets the acceleration setting (from 1 to 9).
 * Ignored if NULL.
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_inq_var_lz4(int ncid, int varid, int *lz4p, int *levelp)
{
    unsigned int level;
    unsigned int id;
    size_t nparams;
    int lz4 = 0; /* Is lz4 in use? */
    int ret;

    /* Get filter information. */
    ret = nc_inq_var_filter(ncid, varid, &id, &nparams, &level);
    if (ret == NC_ENOFILTER)
    {
	if (lz4p)
	    *lz4p = 0;
	return 0;
    }
    else if (ret)
	return ret;

    /* Is lz4 in use? */
    if (id == LZ4_ID)
        lz4++;

    /* Does caller want to know if lz4 is in use? */
    if (lz4p)
        *lz4p = lz4;

    /* If lz4 is in use, check parameter. */
    if (lz4)
    {
        /* For lz4, there is one parameter. */
        if (nparams != 1)
            return NC_EFILTER;

        /* Tell the caller, if they want to know. */
        if (levelp)
            *levelp = level;
    }
    
    return 0;
}

#endif /* BUILD_LZ4 */
#if BUILD_JPEG

/**
 * Turn on jpeg compression for a variable.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param qulity_factor Quality factor, between 1 and 100.
 * @param nx size of X in image. Must be > 0.
 * @param ny size of Y in image. Must be > 0.
 * @param rgb color mode: 1 for RGB, 0 for MONO.
 *
 * @note The number of bytes passed in to each write operation must be
 * nx * ny bytes for rgb = 0, or nx * xy * 3 if rgb = 1.
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_def_var_jpeg(int ncid, int varid, int quality_factor, int nx,
		int ny, int rgb)
{
    unsigned int cd_value[NCC_JPEG_NELEM];
    int ret;

    /* Check inputs. */
    if (quality_factor < 1 || quality_factor > 100 || nx < 1 || ny < 1 || (rgb != 0 && rgb != 1))
        return NC_EINVAL;

    /* Fill parameter array. */
    cd_value[0] = quality_factor;
    cd_value[1] = nx;
    cd_value[2] = ny;
    cd_value[3] = rgb;

    /* Ensure filter is available. */
    if (!H5Zfilter_avail(JPEG_ID))
    {
        printf ("jpeg filter not available.\n");
        return NC_EFILTER;
    }

    /* Set up the jpeg filter for this var. */
    if ((ret = nc_def_var_filter(ncid, varid, JPEG_ID, NCC_JPEG_NELEM, cd_value)))
        return ret;

    return 0;
}

/**
 * Learn whether jpeg compression is on for a variable, and, if so,
 * the level setting.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param jpegp Pointer that gets a 0 if jpeg is not in use for this
 * var, and a 1 if it is. Ignored if NULL.
 * @param qulity_factorp Pointer to int which gets quality factor,
 * between 1 and 100. Ignored if NULL.
 * @param nxp Pointer to int which gets size of X in image. Ignored if
 * NULL.
 * @param nyp Pointer to int which gets size of Y in image. Ignored if
 * NULL.
 * @param rgbp Pointer to int which gets color mode: 1 for RGB, 0 for
 * MONO. Ignored if NULL.
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_inq_var_jpeg(int ncid, int varid, int *jpegp, int *quality_factorp, int *nxp,
		int *nyp, int *rgbp)
{
    unsigned int level;
    unsigned int id;
    unsigned int cd_value[NCC_JPEG_NELEM];    
    size_t nparams;
    int jpeg = 0; /* Is jpeg in use? */
    int ret;

    /* Get filter information. */
    ret = nc_inq_var_filter(ncid, varid, &id, &nparams, cd_value);
    if (ret == NC_ENOFILTER)
    {
	if (jpegp)
	    *jpegp = 0;
	return 0;
    }
    else if (ret)
	return ret;

    /* Is jpeg in use? */
    if (id == JPEG_ID)
        jpeg++;

    /* Does caller want to know if jpeg is in use? */
    if (jpegp)
        *jpegp = jpeg;

    /* If jpeg is in use, check parameter. */
    if (jpeg)
    {
        /* For jpeg, there is one parameter. */
        if (nparams != NCC_JPEG_NELEM)
            return NC_EFILTER;

        /* Tell the caller, if they want to know. */
        if (quality_factorp)
            *quality_factorp = cd_value[0];
        if (nxp)
            *nxp = cd_value[1];
        if (nyp)
            *nyp = cd_value[2];
        if (rgbp)
            *rgbp = cd_value[3];
    }
    
    return 0;
}

#endif /* BUILD_JPEG */
#if BUILD_LZF

/**
 * Turn on lzf compression for a variable.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_def_var_lzf(int ncid, int varid)
{
    int ret;

    if (!H5Zfilter_avail(LZF_ID))
    {
        printf ("lzf filter not available.\n");
        return NC_EFILTER;
    }

    /* Set up the lzf filter for this var. */
    if ((ret = nc_def_var_filter(ncid, varid, LZF_ID, 0, NULL)))
        return ret;

    return 0;
}

/**
 * Learn whether lzf compression is on for a variable.
 *
 * @param ncid File ID.
 * @param varid Variable ID.
 * @param lzfp Pointer that gets a 0 if lzf is not in use for this
 * var, and a 1 if it is. Ignored if NULL.
 *
 * @return 0 for success, error code otherwise.
 * @author Edward Hartnett
 */
int
nc_inq_var_lzf(int ncid, int varid, int *lzfp)
{
    unsigned int id;
    size_t nparams;
    int lzf = 0; /* Is lzf in use? */
    int ret;

    /* Get filter information. */
    ret = nc_inq_var_filter(ncid, varid, &id, &nparams, NULL);
    if (ret == NC_ENOFILTER)
    {
	if (lzfp)
	    *lzfp = 0;
	return 0;
    }
    else if (ret)
	return ret;

    /* Is lzf in use? */
    if (id == LZF_ID)
        lzf++;

    /* Does caller want to know if lzf is in use? */
    if (lzfp)
        *lzfp = lzf;

    return 0;
}

#endif /* BUILD_LZF */
