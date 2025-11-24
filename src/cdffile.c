/**
 * @file
 * @internal The CDF file functions. These provide a read-only
 * interface to CDF SD files.
 *
 * @author Edward Hartnett
 * @date 2025-11-23
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include "nc4internal.h"
#include "cdfdispatch.h"
#include <cdf.h>

#define NUM_TYPES 12 /**< Number of netCDF atomic types. */

extern int nc4_vararray_add(NC_GRP_INFO_T *grp, NC_VAR_INFO_T *var);

/** @internal These flags may not be set for open mode. */
static const int
ILLEGAL_OPEN_FLAGS = (NC_MMAP|NC_64BIT_OFFSET|NC_DISKLESS|NC_WRITE);

/** @internal NetCDF atomic type names. */
static const char*
nc_type_name_g[NUM_TYPES] = {"char", "byte", "short", "int", "float", "double",
                             "ubyte", "ushort", "uint", "int64", "uint64",
                             "string"};

/** @internal NetCDF atomic type sizes. */
static const size_t
nc_type_size_g[NUM_TYPES] = {sizeof(char), sizeof(char), sizeof(short),
                             sizeof(int), sizeof(float), sizeof(double),
                             sizeof(unsigned char), sizeof(unsigned short),
                             sizeof(unsigned int), sizeof(long long),
                             sizeof(unsigned long long), sizeof(char *)};

/**
 * @internal Recursively delete the data for a group (and everything
 * it contains) in our internal metadata store.
 *
 * @param grp Pointer to group info struct.
 *
 * @return ::NC_NOERR No error.
 * @author Edward Hartnett
 */
static int
cdf_rec_grp_del(NC_GRP_INFO_T *grp)
{
    int retval;
    
    /* Handle NULL pointer gracefully */
    if (!grp)
        return NC_NOERR;
    
    /* Use NetCDF-C internal function to recursively delete group metadata */
    if ((retval = nc4_rec_grp_del(grp)))
        return retval;
    
    return NC_NOERR;
}

/**
 * @internal Given an CDF type, set a pointer to netcdf type.
 *
 * See http://www.hdfgroup.org/training/HDFtraining/UsersGuide/Fundmtls.fm3.html
 * for more information re: CDF types.
 *
 * @param h5 Pointer to HDF5 file info struct.
 * @param cdf_typeid Type ID for cdf datatype.
 * @param xtype Pointer that gets netcdf type. Ignored if NULL.
 * @param endniannessp Pointer that gets endianness. Ignored if NULL.
 * @param type_sizep Pointer that gets type size. Ignored if NULL.
 * @param type_name Pointer that gets the type name. Ignored if NULL.
 *
 * @return ::NC_NOERR No error.
 * @author Ed Hartnett
 */
static int
cdf_type_info(NC_FILE_INFO_T *h5, int cdf_typeid, nc_type* xtypep,
               int *endiannessp, size_t *type_sizep, char *type_name)
{
    nc_type xtype = NC_NAT;
    int endianness = NC_ENDIAN_NATIVE;
    size_t type_size = 0;
    const char *name = NULL;

    assert(h5);
    
    /* Map CDF types to NetCDF types */
    switch(cdf_typeid)
    {
        case CDF_BYTE:
        case CDF_INT1:
            xtype = NC_BYTE;
            type_size = sizeof(signed char);
            name = "byte";
            break;
        case CDF_INT2:
            xtype = NC_SHORT;
            type_size = sizeof(short);
            name = "short";
            break;
        case CDF_INT4:
            xtype = NC_INT;
            type_size = sizeof(int);
            name = "int";
            break;
        case CDF_INT8:
            xtype = NC_INT64;
            type_size = sizeof(long long);
            name = "int64";
            break;
        case CDF_UINT1:
            xtype = NC_UBYTE;
            type_size = sizeof(unsigned char);
            name = "ubyte";
            break;
        case CDF_UINT2:
            xtype = NC_USHORT;
            type_size = sizeof(unsigned short);
            name = "ushort";
            break;
        case CDF_UINT4:
            xtype = NC_UINT;
            type_size = sizeof(unsigned int);
            name = "uint";
            break;
        case CDF_REAL4:
        case CDF_FLOAT:
            xtype = NC_FLOAT;
            type_size = sizeof(float);
            name = "float";
            break;
        case CDF_REAL8:
        case CDF_DOUBLE:
        case CDF_EPOCH:
        case CDF_EPOCH16:
        case CDF_TIME_TT2000:
            xtype = NC_DOUBLE;
            type_size = sizeof(double);
            name = "double";
            break;
        case CDF_CHAR:
        case CDF_UCHAR:
            xtype = NC_CHAR;
            type_size = sizeof(char);
            name = "char";
            break;
        default:
            return NC_EBADTYPE;
    }
    
    /* Set output parameters if provided */
    if (xtypep)
        *xtypep = xtype;
    if (endiannessp)
        *endiannessp = endianness;
    if (type_sizep)
        *type_sizep = type_size;
    if (type_name && name)
        strcpy(type_name, name);
    
    return NC_NOERR;
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
    int retval;
    
    /* Allocate and initialize type structure */
    if ((retval = nc4_type_new(type_size, type_name, xtype, &type)))
        return retval;
    
    /* Set type properties */
    type->endianness = endianness;
    type->size = type_size;
    type->nc_type_class = xtype;
    
    /* Return pointer to type */
    if (typep)
        *typep = type;
    
    return NC_NOERR;
}

/**
 * @internal Read an attribute from a CDF file.
 *
 * @param h5 Pointer to the file metadata struct.
 * @param var Pointer to variable metadata struct or NULL for global
 * attributes.
 * @param a Index of attribute to read.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EHDFERR CDF error.
 * @return ::NC_EATTMETA Error reading CDF attribute.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
static int
cdf_read_att(NC_FILE_INFO_T *h5, NC_VAR_INFO_T *var, int a)
{
    NC_CDF_FILE_INFO_T *cdf_info;
    void *id;
    CDFstatus status;
    char attrName[CDF_ATTR_NAME_LEN256+1];
    long dataType, numElems;
    void *data = NULL;
    NC_ATT_INFO_T *att;
    nc_type xtype;
    int retval;
    
    /* Get CDF file ID */
    cdf_info = (NC_CDF_FILE_INFO_T *)h5->format_file_info;
    id = (CDFid)cdf_info->id;
    
    /* Get attribute info */
    status = CDFlib(SELECT_, CDF_, id,
                    SELECT_, ATTR_, (long)a,
                    GET_, ATTR_NAME_, attrName,
                    GET_, ATTR_NUMgENTRIES_, &numElems,
                    NULL_);
    if (status != CDF_OK)
        return NC_EATTMETA;
    
    /* Get data type from first entry */
    status = CDFlib(SELECT_, gENTRY_, 0L,
                    GET_, gENTRY_DATATYPE_, &dataType,
                    GET_, gENTRY_NUMELEMS_, &numElems,
                    NULL_);
    if (status != CDF_OK)
        return NC_EATTMETA;
    
    /* Map CDF type to NetCDF type */
    if ((retval = cdf_type_info(h5, dataType, &xtype, NULL, NULL, NULL)))
        return retval;
    
    /* Allocate space for attribute data */
    size_t type_size;
    cdf_type_info(h5, dataType, NULL, NULL, &type_size, NULL);
    if (!(data = malloc(numElems * type_size)))
        return NC_ENOMEM;
    
    /* Read attribute data */
    status = CDFlib(SELECT_, gENTRY_, 0L,
                    GET_, gENTRY_DATA_, data,
                    NULL_);
    if (status != CDF_OK)
    {
        free(data);
        return NC_EATTMETA;
    }
    
    /* Add attribute to metadata */
    NCindex *att_list = var ? var->att : h5->root_grp->att;
    if ((retval = nc4_att_list_add(att_list, attrName, &att)))
    {
        free(data);
        return retval;
    }
    
    /* Set attribute properties */
    att->nc_typeid = xtype;
    att->len = numElems;
    att->data = data;
    
    return NC_NOERR;
}

/**
 * @internal Read a CDF dimension. As new dimensions are found, add
 * them to the metadata list of dimensions.
 *
 * @param h5 Pointer to the file metadata struct.
 * @param var Pointer to variable metadata struct or NULL for global
 * attributes.
 * @param rec_dim_len Actual length of first dim for this SD.
 * @param d Dimension index for this SD.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EHDFERR CDF error.
 * @return ::NC_EDIMMETA Error reading CDF dimension info.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EMAXNAME Name too long.
 * @author Ed Hartnett
 */
static int
cdf_read_dim(NC_FILE_INFO_T *h5, NC_VAR_INFO_T *var, int rec_dim_len, int d)
{
    /* Function body commented out for v1.3.0 Sprint 3 - UDF skeleton only */
    /* Implementation will be added in Sprint 4 */
    assert(h5 && var);
    printf("rec_dim_len %d d %d\n", rec_dim_len, d);
    return NC_NOERR; /* Placeholder return */
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
    NC_VAR_INFO_T *new_var;
    NC_TYPE_INFO_T *type_info;
    int retval;

    assert((fill_value || !fill_value) && (chunksizes || !chunksizes));
    printf("contiguous %d\n", contiguous);
       
    /* Add variable using NetCDF-C internal function */
    if ((retval = nc4_var_list_add(grp, name, ndims, &new_var)))
        return retval;
    
    /* Set variable dimensions */
    if ((retval = nc4_var_set_ndims(new_var, ndims)))
        return retval;
    
    /* Set type information */
    if ((retval = nc4_set_var_type(xtype, endianness, type_size, type_name, &type_info)))
        return retval;
    new_var->type_info = type_info;
    new_var->type_info->rc++;
    
    /* Store format-specific info */
    new_var->format_var_info = format_var_info;
    
    /* Return variable pointer */
    if (var)
        *var = new_var;
    
    return NC_NOERR;
}

/**
 * @internal Read a CDF variable, including its associated dimensions
 * and attributes.
 *
 * @param h5 Pointer to the file metadata struct.
 * @param v Index of variable to read.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EHDFERR CDF error.
 * @return ::NC_EDIMMETA Error reading CDF dimension info.
 * @return ::NC_EVARMETA Error reading CDF dataset or att.
 * @return ::NC_EATTMETA Error reading CDF attribute.
 * @return ::NC_ENOMEM Out of memory.
 * @return ::NC_EMAXNAME Name too long.
 * @author Ed Hartnett
 */
static int
cdf_read_var(NC_FILE_INFO_T *h5, int v)
{
    NC_CDF_FILE_INFO_T *cdf_info;
    NC_VAR_CDF_INFO_T *var_cdf_info;
    void *id;
    CDFstatus status;
    char varName[CDF_VAR_NAME_LEN256+1];
    long dataType, numDims, dimSizes[CDF_MAX_DIMS];
    long varNum = (long)v;
    NC_VAR_INFO_T *var;
    nc_type xtype;
    int endianness;
    size_t type_size;
    char type_name[NC_MAX_NAME+1];
    int retval, i;
    
    /* Get CDF file ID */
    cdf_info = (NC_CDF_FILE_INFO_T *)h5->format_file_info;
    id = (CDFid)cdf_info->id;
    
    /* Get variable info */
    status = CDFlib(SELECT_, CDF_, id,
                    SELECT_, zVAR_, varNum,
                    GET_, zVAR_NAME_, varName,
                    GET_, zVAR_DATATYPE_, &dataType,
                    GET_, zVAR_NUMDIMS_, &numDims,
                    GET_, zVAR_DIMSIZES_, dimSizes,
                    NULL_);
    if (status != CDF_OK)
        return NC_EVARMETA;
    
    /* Map CDF type to NetCDF type */
    if ((retval = cdf_type_info(h5, dataType, &xtype, &endianness, &type_size, type_name)))
        return retval;
    
    /* Create dimensions if needed */
    int dimids[NC_MAX_VAR_DIMS];
    /* for (i = 0; i < numDims; i++) */
    /* { */
    /*     NC_DIM_INFO_T *dim; */
    /*     char dimname[NC_MAX_NAME+1]; */
    /*     snprintf(dimname, NC_MAX_NAME, "dim_%d", i); */
        
    /*     /\* Check if dimension already exists *\/ */
    /*     retval = nc4_find_dim(h5->root_grp, dimname, &dim, NULL); */
    /*     if (retval == NC_EBADDIM) */
    /*     { */
    /*         /\* Create new dimension *\/ */
    /*         if ((retval = nc4_dim_list_add(h5->root_grp, dimname, (size_t)dimSizes[i], -1, &dim))) */
    /*             return retval; */
    /*     } */
    /*     dimids[i] = dim->hdr.id; */
    /* } */
    
    /* Allocate CDF-specific variable info */
    if (!(var_cdf_info = calloc(1, sizeof(NC_VAR_CDF_INFO_T))))
        return NC_ENOMEM;
    var_cdf_info->sdsid = varNum;
    var_cdf_info->cdf_data_type = dataType;
    
    /* Add variable to group */
    if ((retval = nc4_var_list_add_full(h5->root_grp, varName, numDims, xtype,
                                         endianness, type_size, type_name, NULL,
                                         1, NULL, var_cdf_info, &var)))
    {
        free(var_cdf_info);
        return retval;
    }
    
    /* Set variable dimensions */
    for (i = 0; i < numDims; i++)
        var->dimids[i] = dimids[i];
    
    /* Variable attributes will be read separately via attribute iteration */
    /* For now, skip variable-specific attributes in this simplified implementation */
    
    return NC_NOERR;
}

/**
 * @internal Open a CDF SD file for read-only access.
 *
 * @param path The file name of the file.
 * @param mode The open mode flag.
 * @param basepe Ignored by this function.
 * @param chunksizehintp Ignored by this function.
 * @param parameters pointer to struct holding extra data (e.g. for
 * parallel I/O) layer. Ignored if NULL. Ignored by this function.
 * @param dispatch Pointer to the dispatch table for this file.
 * @param nc_file Pointer to an instance of NC. The ncid has already
 * been assigned, and is in nc_file->ext_ncid.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EINVAL Invalid input.
 * @return ::NC_EHDFERR Error from CDF layer.
 * @return ::NC_ENOMEM Out of memory.
 * @author Ed Hartnett
 */
int
NC_CDF_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
             void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_CDF_FILE_INFO_T *cdf_file;
    NC_GRP_INFO_T *grp;
    CDFid id;
    CDFstatus status;
    long numzVars, numrVars, numAttrs;
    long varNum, attrNum;
    int retval;

    assert(basepe || !basepe);
    assert(chunksizehintp || !chunksizehintp);
    assert(parameters || !parameters);
    assert(dispatch);
    
    /* Validate parameters */
    if (!path)
        return NC_EINVAL;
    
    /* Check for illegal open flags */
    if (mode & ILLEGAL_OPEN_FLAGS)
        return NC_EINVAL;
    
    /* Only support read-only mode */
    if (mode & NC_WRITE)
        return NC_EPERM;
    
    /* Find pointer to NC. */
    if ((retval = NC_check_id(ncid, &nc)))
        return retval;

    /* Open CDF file */
    status = CDFopenCDF(path, &id);
    if (status != CDF_OK)
        return NC_ENOTNC4;
    
    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return retval;
    assert(h5 && h5->root_grp);
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Get file info structure - root group should already be initialized by dispatcher */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
    {
        CDFcloseCDF(id);
        return retval;
    }
    
    /* Ensure we have the root group */
    if (!h5->root_grp)
    {
        CDFcloseCDF(id);
        return NC_EBADGRPID;
    }
    grp = h5->root_grp;
    
    /* Allocate CDF-specific file info */
    if (!(cdf_file = calloc(1, sizeof(NC_CDF_FILE_INFO_T))))
    {
        CDFcloseCDF(id);
        return NC_ENOMEM;
    }
    
    /* Store CDF file ID (cast from CDFid pointer to long) */
    cdf_file->id = id;
    h5->format_file_info = cdf_file;
    
    /* Query file metadata */
    status = CDFlib(SELECT_, CDF_, id,
                    GET_, CDF_NUMzVARS_, &numzVars,
                    GET_, CDF_NUMrVARS_, &numrVars,
                    GET_, CDF_NUMATTRS_, &numAttrs,
                    NULL_);
    if (status != CDF_OK)
    {
        free(cdf_file);
        CDFcloseCDF(id);
        return NC_EHDFERR;
    }
    
    /* Read global attributes */
    for (attrNum = 0; attrNum < numAttrs; attrNum++)
    {
        long attrScope;
        status = CDFlib(SELECT_, ATTR_, attrNum,
                        GET_, ATTR_SCOPE_, &attrScope,
                        NULL_);
        if (status == CDF_OK && attrScope == GLOBAL_SCOPE)
        {
            if ((retval = cdf_read_att(h5, NULL, attrNum)))
            {
                free(cdf_file);
                CDFcloseCDF(id);
                return retval;
            }
        }
    }
    
    /* Read zVariables (ignore rVariables for now) */
    for (varNum = 0; varNum < numzVars; varNum++)
    {
        if ((retval = cdf_read_var(h5, varNum)))
        {
            free(cdf_file);
            CDFcloseCDF(id);
            return retval;
        }
    }
    
    return NC_NOERR;
}

/**
 * @internal Abort (close) the CDF file.
 *
 * @param ncid File ID.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR Error from CDF layer.
 * @author Ed Hartnett
 */
int
NC_CDF_abort(int ncid)
{
    /* For read-only files, abort is same as close */
    return NC_CDF_close(ncid, NULL);
}

/**
 * @internal Close the CDF file.
 *
 * @param ncid File ID.
 * @param ignore Ignore this pointer.
 *
 * @return ::NC_NOERR No error.
 * @return ::NC_EBADID Bad ncid.
 * @return ::NC_EHDFERR Error from CDF layer.
 * @author Ed Hartnett
 */
int
NC_CDF_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_CDF_FILE_INFO_T *cdf_file;
    CDFstatus status;
    int retval;

    assert(ignore || !ignore);
    
    /* Get file info structure */
    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;
    
    /* Get CDF-specific info */
    cdf_file = (NC_CDF_FILE_INFO_T *)h5->format_file_info;
    if (!cdf_file)
        return NC_NOERR;
    
    /* Close CDF file */
    status = CDFcloseCDF((CDFid)cdf_file->id);
    
    /* Free metadata structures */
    if (grp)
        cdf_rec_grp_del(grp);
    
    /* Free CDF-specific info */
    free(cdf_file);
    h5->format_file_info = NULL;
    
    return (status == CDF_OK) ? NC_NOERR : NC_EHDFERR;
}
