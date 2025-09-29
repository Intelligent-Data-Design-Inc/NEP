/**
 * @file
 * @internal The AB file functions.
 *
 * @author Ed Hartnett
 */

#include "config.h"
#include <errno.h>  /* netcdf functions sometimes return system errors */
#include <ncdimscale.h>
#include "nc.h"
#include "nc4internal.h"
#include "hdf5internal.h"
#include "grib2dispatch.h"
#include <strings.h>
#include <math.h>
#include <libgen.h>

/**
 * @internal Open a GRIB2 file.
 *
 * @param path The file name of the file.
 * @param mode The open mode flag.
 * @param basepe Ignored by this function.
 * @param chunksizehintp Ignored by this function.
 * @param parameters pointer to struct holding extra data (e.g. for
 * parallel I/O) layer. Ignored if NULL.
 * @param dispatch Pointer to the dispatch tgrib2le for this file.
 * @param ncid The ncid.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Invalid input.
 * @author Ed Hartnett
 */
int
NC_GRIB2_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
	      void *parameters, const NC_Dispatch *dispatch, int ncid)
{
   NC *nc;
   int retval;
   
   /* Check inputs. */
   assert(path);

   LOG((1, "%s: path %s mode %d params %x", __func__, path, mode,
        parameters));

    /* Find pointer to NC. */
    if ((retval = NC_check_id(ncid, &nc)))
        return retval;

    /* Check the mode for validity */
   /* if (mode & ILLEGAL_OPEN_FLAGS) */
   /*    return NC_EINVAL; */

   /* /\* Add necessary structs to hold netcdf-4 file data. *\/ */
   /* if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5))) */
   /*     return retval; */
   /* assert(h5 && h5->root_grp); */
   /* h5->no_write = NC_TRUE; */
   /* h5->root_grp->atts_read = 1; */

/*     /\* Allocate data to hold HDF4 specific file data. *\/ */
/*     if (!(hdf4_file = malloc(sizeof(NC_HDF4_FILE_INFO_T)))) */
/*         return NC_ENOMEM; */
/*     h5->format_file_info = hdf4_file; */
/*     hdf4_file->sdid = sdid; */

/*     /\* Read the global atts. *\/ */
/*     for (a = 0; a < num_gatts; a++) */
/*         if ((retval = hdf4_read_att(h5, NULL, a))) */
/*             break; */

/*     /\* Read each dataset. *\/ */
/*     if (!retval) */
/*         for (v = 0; v < num_datasets; v++) */
/*             if ((retval = hdf4_read_var(h5, v))) */
/*                 break; */

/*     /\* If there is an error, free resources. *\/ */
/*     if (retval) */
/*         free(hdf4_file); */

/* #ifdef LOGGING */
/*     /\* This will print out the names, types, lens, etc of the vars and */
/*        atts in the file, if the logging level is 2 or greater. *\/ */
/*     log_metadata_nc(h5); */
/* #endif */

   /* Open the file. */
   return retval;
}

/**
 * @internal Abort (close) the HDF4 file.
 *
 * @param ncid File ID.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR Error from HDF4 layer.
 * @author Ed Hartnett
 */
int
NC_GRIB2_abort(int ncid)
{
    return NC_GRIB2_close(ncid, NULL);
}

/**
 * @internal Close the GRIB2 file.
 *
 * @param ncid File ID.
 * @param ignore Just ignore me.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @author Ed Hartnett
 */
int
NC_GRIB2_close(int ncid, void *ignore)
{
   NC_GRP_INFO_T *grp;
   NC *nc;
   NC_HDF5_FILE_INFO_T *h5;
   GRIB2_FILE_INFO_T *grib2_file;
   int ret;

   LOG((1, "%s: ncid 0x%x", __func__, ncid));

   /* /\* Find our metadata for this file. *\/ */
   /* if ((ret = nc4_find_nc_grp_h5(ncid, &nc, &grp, &h5))) */
   /*    return ret; */
   /* assert(nc && h5 && h5->format_file_info); */

    /* /\* Clean up HDF4 specific allocations. *\/ */
    /* if ((retval = hdf4_rec_grp_del(h5->root_grp))) */
    /*     return retval; */

    /* /\* Close hdf4 file and free HDF4 file info. *\/ */
    /* hdf4_file = (NC_HDF4_FILE_INFO_T *)h5->format_file_info; */
    /* if (SDend(hdf4_file->sdid)) */
    /*     return NC_EHDFERR; */
    /* free(hdf4_file); */

    /* /\* Free the NC_FILE_INFO_T struct. *\/ */
    /* if ((retval = nc4_nc4f_list_del(h5))) */
    /*     return retval; */

   return NC_NOERR;
}
