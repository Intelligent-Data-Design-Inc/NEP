/**
 * @file dicomfile.c
 * @brief DICOM User-Defined Format (UDF) dispatch layer.
 *
 * Implements the NEP DICOM reader, which maps DICOM image SOP Instances
 * to the netCDF-4 data model via libdicom. This Sprint 1 implementation
 * supports native (uncompressed), single-frame, grayscale and RGB images
 * only; encapsulated (compressed) transfer syntaxes and multi-frame
 * objects are explicitly rejected.
 *
 * - Primary image: exposed as a `pixel_data` variable in the root group
 *   with dimensions derived from Rows, Columns, SamplesPerPixel, and
 *   PlanarConfiguration.
 * - Pixel type is derived from BitsAllocated and PixelRepresentation.
 * - Patient/Study/Series/Image Pixel module tags are mapped to global
 *   and variable attributes.
 * - Data reading: NC_DICOM_get_vara() reads the requested frame via
 *   libdicom's dcm_filehandle_read_frame() and copies the hyperslab
 *   into the user buffer.
 *
 * @author Edward Hartnett
 * @date 2026-07-25
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "nep_nc4.h"
#include "dicomdispatch.h"

/* Include libdicom header if available */
#ifdef HAVE_DICOM
#include <dicom/dicom.h>
#endif

extern int nc4_var_list_add(NC_GRP_INFO_T *grp, const char *name, int ndims,
                            NC_VAR_INFO_T **var);

/**
 * @internal Map DICOM BitsAllocated/PixelRepresentation to a netCDF type.
 *
 * @param bits_allocated DICOM BitsAllocated (0028,0100).
 * @param pixel_representation DICOM PixelRepresentation (0028,0103).
 * @param xtypep Pointer that gets the nc_type.
 * @param type_sizep Pointer that gets the type size in bytes.
 * @param type_name Buffer (at least NC_MAX_NAME+1 bytes) that gets the type
 * name string.
 * @param endiannessp Pointer that gets the endianness.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADTYPE Unknown bit depth.
 * @author Edward Hartnett
 */
#ifdef HAVE_DICOM
static int
dicom_bits_to_nc_type(int bits_allocated, int pixel_representation,
                      nc_type *xtypep, size_t *type_sizep, char *type_name,
                      int *endiannessp)
{
    nc_type xtype;
    size_t type_size;
    const char *name;
    int endianness = NC_ENDIAN_NATIVE;

    switch (bits_allocated)
    {
    case 8:
        if (pixel_representation == 0)
        {
            xtype = NC_UBYTE; type_size = 1; name = "ubyte";
        }
        else
        {
            xtype = NC_BYTE; type_size = 1; name = "byte";
        }
        break;
    case 16:
        if (pixel_representation == 0)
        {
            xtype = NC_USHORT; type_size = 2; name = "ushort";
        }
        else
        {
            xtype = NC_SHORT; type_size = 2; name = "short";
        }
        break;
    case 32:
        if (pixel_representation == 0)
        {
            xtype = NC_UINT; type_size = 4; name = "uint";
        }
        else
        {
            xtype = NC_INT; type_size = 4; name = "int";
        }
        break;
    case 64:
        if (pixel_representation == 0)
        {
            xtype = NC_UINT64; type_size = 8; name = "uint64";
        }
        else
        {
            xtype = NC_INT64; type_size = 8; name = "int64";
        }
        break;
    default:
        return NC_EBADTYPE;
    }

    if (xtypep) *xtypep = xtype;
    if (type_sizep) *type_sizep = type_size;
    if (type_name) strncpy(type_name, name, NC_MAX_NAME);
    if (endiannessp) *endiannessp = endianness;

    return NC_NOERR;
}

/**
 * @internal Set the type of a netCDF-4 variable.
 *
 * Mirror of the helper in fitsfile.c.
 */
static int
nc4_set_var_type(nc_type xtype, int endianness, size_t type_size,
                 char *type_name, NC_TYPE_INFO_T **typep)
{
    NC_TYPE_INFO_T *type;

    assert(typep);

    if (!(type = calloc(1, sizeof(NC_TYPE_INFO_T))))
        return NC_ENOMEM;
    if (!(type->hdr.name = strdup(type_name)))
    {
        free(type);
        return NC_ENOMEM;
    }
    type->hdr.sort = NCTYP;

    if (xtype == NC_FLOAT)
        type->nc_type_class = NC_FLOAT;
    else if (xtype == NC_DOUBLE)
        type->nc_type_class = NC_DOUBLE;
    else if (xtype == NC_CHAR)
        type->nc_type_class = NC_STRING;
    else
        type->nc_type_class = NC_INT;

    type->endianness = endianness;
    type->size = type_size;
    type->hdr.id = (size_t)xtype;

    *typep = type;
    return NC_NOERR;
}

/**
 * @internal Create a new variable and insert it into relevant lists.
 *
 * Mirror of the helper in fitsfile.c.
 */
static int
nc4_var_list_add_full(NC_GRP_INFO_T *grp, const char *name, int ndims,
                      nc_type xtype, int endianness, size_t type_size,
                      char *type_name, void *fill_value, int contiguous,
                      size_t *chunksizes, void *format_var_info,
                      NC_VAR_INFO_T **var)
{
    int d, retval;

    if ((retval = nc4_var_list_add(grp, name, ndims, var)))
        return retval;
    (*var)->created = NC_TRUE;
    (*var)->written_to = NC_TRUE;
    (*var)->format_var_info = format_var_info;
    (*var)->atts_read = 1;

    if ((retval = nc4_set_var_type(xtype, endianness, type_size, type_name,
                                   &(*var)->type_info)))
        return retval;

    (*var)->endianness = (*var)->type_info->endianness;
    (*var)->type_info->rc++;

    if (fill_value)
    {
        if (!((*var)->fill_value = malloc(type_size)))
            return NC_ENOMEM;
        memcpy((*var)->fill_value, fill_value, type_size);
    }

    (*var)->storage = contiguous ? NC_CONTIGUOUS : NC_CHUNKED;

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
 * @internal Look up a dimension length by its assigned dimid.
 */
static size_t
dicom_dimid_to_len(NC_GRP_INFO_T *grp, int dimid)
{
    size_t i;

    for (i = 0; i < ncindexsize(grp->dim); i++)
    {
        NC_DIM_INFO_T *dim = (NC_DIM_INFO_T *)ncindexith(grp->dim, i);
        if (dim && dim->hdr.id == dimid)
            return dim->len;
    }
    return 0;
}

/**
 * @internal Get a string-valued DICOM tag from a Data Set.
 *
 * The returned pointer is borrowed from the DcmElement and remains valid
 * only while the Data Set (and its containing DcmFilehandle) is alive.
 *
 * @param error libdicom error object.
 * @param dataset Data Set to search.
 * @param keyword DICOM keyword for the tag.
 * @param valuep Pointer that receives the borrowed string.
 *
 * @return Non-zero if found and successfully retrieved.
 */
static int
dicom_get_string_tag(DcmError **error, const DcmDataSet *dataset,
                     const char *keyword, const char **valuep)
{
    uint32_t tag;
    DcmElement *element;

    tag = dcm_dict_tag_from_keyword(keyword);
    if (tag == 0)
        return 0;

    element = dcm_dataset_get(error, dataset, tag);
    if (element == NULL)
        return 0;

    return dcm_element_get_value_string(error, element, 0, valuep);
}

/**
 * @internal Get a numeric DICOM tag from a Data Set.
 *
 * @param error libdicom error object.
 * @param dataset Data Set to search.
 * @param keyword DICOM keyword for the tag.
 * @param valuep Pointer that receives the integer value.
 *
 * @return Non-zero if found and successfully retrieved.
 */
static int
dicom_get_int_tag(DcmError **error, const DcmDataSet *dataset,
                  const char *keyword, int64_t *valuep)
{
    uint32_t tag;
    DcmElement *element;

    tag = dcm_dict_tag_from_keyword(keyword);
    if (tag == 0)
        return 0;

    element = dcm_dataset_get(error, dataset, tag);
    if (element == NULL)
        return 0;

    return dcm_element_get_value_integer(error, element, 0, valuep);
}

/**
 * @internal Add a string global attribute to a group.
 *
 * @param grp Group receiving the attribute.
 * @param name Attribute name.
 * @param value Attribute value (may be NULL).
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
dicom_add_att(NC_GRP_INFO_T *grp, const char *name, const char *value)
{
    NC_ATT_INFO_T *att = NULL;
    size_t vlen;
    char *data = NULL;
    int retval;

    if (!value)
        return NC_NOERR;

    vlen = strlen(value);
    if ((retval = nc4_att_list_add(grp->att, name, &att)))
        return retval;

    att->nc_typeid = NC_CHAR;
    att->len = vlen;
    if (vlen > 0)
    {
        if (!(data = malloc(vlen + 1)))
            return NC_ENOMEM;
        memcpy(data, value, vlen + 1);
    }
    att->data = data;
    att->dirty = NC_TRUE;

    return NC_NOERR;
}

/**
 * @internal Add an integer-valued string global attribute to a group.
 *
 * The integer is formatted with snprintf into a temporary buffer and
 * stored as an NC_CHAR attribute.
 *
 * @param grp Group receiving the attribute.
 * @param name Attribute name.
 * @param value Integer value.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 */
static int
dicom_add_int_att(NC_GRP_INFO_T *grp, const char *name, long long value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", value);
    return dicom_add_att(grp, name, buf);
}

/**
 * @internal Add a string variable attribute.
 */
static int
dicom_add_var_att(NC_VAR_INFO_T *var, const char *name, const char *value)
{
    NC_ATT_INFO_T *att = NULL;
    size_t vlen;
    char *data = NULL;
    int retval;

    if (!value)
        return NC_NOERR;

    vlen = strlen(value);
    if ((retval = nc4_att_list_add(var->att, name, &att)))
        return retval;

    att->nc_typeid = NC_CHAR;
    att->len = vlen;
    if (vlen > 0)
    {
        if (!(data = malloc(vlen + 1)))
            return NC_ENOMEM;
        memcpy(data, value, vlen + 1);
    }
    att->data = data;
    att->dirty = NC_TRUE;

    return NC_NOERR;
}

/**
 * @internal Add an integer variable attribute formatted as a string.
 */
static int
dicom_add_var_int_att(NC_VAR_INFO_T *var, const char *name, long long value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", value);
    return dicom_add_var_att(var, name, buf);
}

/**
 * @internal Determine whether the given Transfer Syntax UID is one of the
 * native (uncompressed) transfer syntaxes supported by this sprint.
 *
 * @param uid Transfer Syntax UID string.
 *
 * @return Non-zero if native uncompressed.
 */
static int
dicom_is_native_transfer_syntax(const char *uid)
{
    if (!uid)
        return 0;

    return (strcmp(uid, "1.2.840.10008.1.2") == 0 ||      /* Implicit VR LE */
            strcmp(uid, "1.2.840.10008.1.2.1") == 0 ||    /* Explicit VR LE */
            strcmp(uid, "1.2.840.10008.1.2.2") == 0);     /* Explicit VR BE */
}

/**
 * @internal Read the DICOM image metadata from the libdicom metadata subset
 * and populate the NetCDF-4 in-memory model with dimensions, the pixel data
 * variable, and attributes.
 *
 * @param h5 Pointer to the netCDF-4 file info struct.
 *
 * @return NC_NOERR No error.
 * @return NC_ENOMEM Out of memory.
 * @return NC_EINVAL Missing or invalid image metadata.
 * @return NC_EBADTYPE Unsupported BitsAllocated.
 * @author Edward Hartnett
 */
static int
dicom_read_image_metadata(NC_FILE_INFO_T *h5)
{
    NC_DICOM_FILE_INFO_T *dicom_file;
    NC_GRP_INFO_T *grp;
    DcmError *error = NULL;
    const DcmDataSet *file_meta = NULL;
    const char *transfer_syntax_uid = NULL;
    const char *photometric_interpretation = NULL;
    const char *patient_name = NULL;
    const char *patient_id = NULL;
    const char *study_uid = NULL;
    const char *series_uid = NULL;
    const char *sop_class_uid = NULL;
    const char *sop_instance_uid = NULL;
    const char *modality = NULL;
    int64_t rows = 0, columns = 0;
    int64_t samples_per_pixel = 1, bits_allocated = 0, bits_stored = 0;
    int64_t high_bit = 0, pixel_representation = 0, planar_configuration = 0;
    int64_t nframes = 1;
    nc_type xtype;
    size_t type_size;
    char type_name[NC_MAX_NAME + 1];
    int endianness = NC_ENDIAN_NATIVE;
    int dimids[NC_MAX_VAR_DIMS];
    int ndims = 0;
    int retval;
    NC_DICOM_VAR_INFO_T *var_info = NULL;
    NC_VAR_INFO_T *var = NULL;

    dicom_file = (NC_DICOM_FILE_INFO_T *)h5->format_file_info;
    grp = h5->root_grp;

    /* Transfer Syntax UID is in File Meta Information. */
    file_meta = dcm_filehandle_get_file_meta(&error, dicom_file->filehandle);
    if (file_meta == NULL)
    {
        if (error)
        {
            dcm_error_log(error);
            dcm_error_clear(&error);
        }
        return NC_EINVAL;
    }

    /* Transfer Syntax UID must be present and native uncompressed. */
    if (!dicom_get_string_tag(&error, file_meta,
                              "TransferSyntaxUID", &transfer_syntax_uid))
    {
        if (error)
        {
            dcm_error_log(error);
            dcm_error_clear(&error);
        }
        return NC_EINVAL;
    }

    if (!(dicom_file->transfer_syntax_uid = strdup(transfer_syntax_uid)))
        return NC_ENOMEM;

    if (!dicom_is_native_transfer_syntax(transfer_syntax_uid))
    {
        dicom_file->encapsulated = 1;
        return NC_EINVAL;  /* Compressed; not supported in Sprint 1 */
    }
    dicom_file->encapsulated = 0;

    /* Required image pixel module tags. */
    if (!dicom_get_int_tag(&error, dicom_file->metadata, "Rows", &rows) ||
        !dicom_get_int_tag(&error, dicom_file->metadata, "Columns", &columns) ||
        !dicom_get_int_tag(&error, dicom_file->metadata, "BitsAllocated",
                           &bits_allocated))
    {
        if (error)
        {
            dcm_error_log(error);
            dcm_error_clear(&error);
        }
        return NC_EINVAL;
    }

    /* Optional image pixel module tags with sensible defaults. */
    dicom_get_int_tag(&error, dicom_file->metadata, "SamplesPerPixel",
                      &samples_per_pixel);
    dicom_get_int_tag(&error, dicom_file->metadata, "BitsStored",
                      &bits_stored);
    dicom_get_int_tag(&error, dicom_file->metadata, "HighBit", &high_bit);
    dicom_get_int_tag(&error, dicom_file->metadata, "PixelRepresentation",
                      &pixel_representation);
    dicom_get_int_tag(&error, dicom_file->metadata, "PlanarConfiguration",
                      &planar_configuration);
    dicom_get_int_tag(&error, dicom_file->metadata, "NumberOfFrames", &nframes);

    if (samples_per_pixel < 1)
        samples_per_pixel = 1;
    if (bits_stored == 0)
        bits_stored = bits_allocated;
    if (high_bit == 0 && bits_stored > 0)
        high_bit = bits_stored - 1;

    /* Multi-frame is out of scope for Sprint 1. */
    if (nframes > 1)
        return NC_EINVAL;
    dicom_file->nframes = (int)nframes;

    dicom_file->rows = (size_t)rows;
    dicom_file->columns = (size_t)columns;
    dicom_file->samples_per_pixel = (int)samples_per_pixel;
    dicom_file->bits_allocated = (int)bits_allocated;
    dicom_file->bits_stored = (int)bits_stored;
    dicom_file->high_bit = (int)high_bit;
    dicom_file->pixel_representation = (int)pixel_representation;
    dicom_file->planar_configuration = (int)planar_configuration;
    dicom_file->color_dim_index = -1;

    if ((retval = dicom_bits_to_nc_type((int)bits_allocated,
                                        (int)pixel_representation,
                                        &xtype, &type_size, type_name,
                                        &endianness)))
        return retval;

    dicom_file->xtype = xtype;
    dicom_file->type_size = type_size;

    /* Create dimensions in the same order as the native pixel data layout:
     *   - Grayscale: [row, column]
     *   - RGB planar=0 (interleaved): [row, column, sample]
     *   - RGB planar=1 (planar): [sample, row, column]
     */
    {
        NC_DIM_INFO_T *dim;

        if ((retval = nc4_dim_list_add(grp, "row", dicom_file->rows, -1, &dim)))
            return retval;
        dimids[ndims++] = dim->hdr.id;

        if ((retval = nc4_dim_list_add(grp, "column", dicom_file->columns, -1,
                                       &dim)))
            return retval;
        dimids[ndims++] = dim->hdr.id;

        if (samples_per_pixel > 1)
        {
            if (planar_configuration == 1)
            {
                /* Insert sample dimension before rows for planar layout. */
                int i;
                int sample_dimid;

                if ((retval = nc4_dim_list_add(grp, "sample",
                                                (size_t)samples_per_pixel,
                                                -1, &dim)))
                    return retval;
                sample_dimid = dim->hdr.id;

                /* Shift row/column dimids down and place sample first. */
                for (i = ndims; i > 0; i--)
                    dimids[i] = dimids[i - 1];
                dimids[0] = sample_dimid;
                ndims++;
                dicom_file->color_dim_index = 0;
            }
            else
            {
                /* Append sample dimension for interleaved color-by-pixel. */
                if ((retval = nc4_dim_list_add(grp, "sample",
                                                (size_t)samples_per_pixel,
                                                -1, &dim)))
                    return retval;
                dimids[ndims++] = dim->hdr.id;
                dicom_file->color_dim_index = ndims - 1;
            }
        }
    }

    /* Create the pixel_data variable. */
    if (!(var_info = calloc(1, sizeof(NC_DICOM_VAR_INFO_T))))
        return NC_ENOMEM;
    var_info->frame_index = 1;

    /* Use contiguous storage for the in-memory view. */
    if ((retval = nc4_var_list_add_full(grp, "pixel_data", ndims, xtype,
                                        endianness, type_size, type_name,
                                        NULL, 1, NULL, var_info, &var)))
    {
        free(var_info);
        return retval;
    }

    for (int d = 0; d < ndims; d++)
        var->dimids[d] = dimids[d];

    /* Global attributes from Patient/Study/Series/Image modules.
     * Optional attributes may be missing; clear any libdicom error
     * before each lookup so stale errors do not accumulate. */
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "PatientName",
                         &patient_name);
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "PatientID",
                         &patient_id);
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "StudyInstanceUID",
                         &study_uid);
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "SeriesInstanceUID",
                         &series_uid);
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "SOPClassUID",
                         &sop_class_uid);
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "SOPInstanceUID",
                         &sop_instance_uid);
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "Modality", &modality);
    dcm_error_clear(&error);
    dicom_get_string_tag(&error, dicom_file->metadata, "PhotometricInterpretation",
                         &photometric_interpretation);

    if ((retval = dicom_add_att(grp, "TransferSyntaxUID",
                               transfer_syntax_uid)))
        return retval;
    if ((retval = dicom_add_att(grp, "PhotometricInterpretation",
                               photometric_interpretation)))
        return retval;
    if ((retval = dicom_add_att(grp, "Modality", modality)))
        return retval;
    if ((retval = dicom_add_att(grp, "PatientName", patient_name)))
        return retval;
    if ((retval = dicom_add_att(grp, "PatientID", patient_id)))
        return retval;
    if ((retval = dicom_add_att(grp, "StudyInstanceUID", study_uid)))
        return retval;
    if ((retval = dicom_add_att(grp, "SeriesInstanceUID", series_uid)))
        return retval;
    if ((retval = dicom_add_att(grp, "SOPClassUID", sop_class_uid)))
        return retval;
    if ((retval = dicom_add_att(grp, "SOPInstanceUID", sop_instance_uid)))
        return retval;

    /* Variable attributes on pixel_data. */
    if ((retval = dicom_add_var_int_att(var, "SamplesPerPixel",
                                        samples_per_pixel)))
        return retval;
    if ((retval = dicom_add_var_int_att(var, "BitsAllocated", bits_allocated)))
        return retval;
    if ((retval = dicom_add_var_int_att(var, "BitsStored", bits_stored)))
        return retval;
    if ((retval = dicom_add_var_int_att(var, "HighBit", high_bit)))
        return retval;
    if ((retval = dicom_add_var_int_att(var, "PixelRepresentation",
                                        pixel_representation)))
        return retval;
    if ((retval = dicom_add_var_int_att(var, "PlanarConfiguration",
                                        planar_configuration)))
        return retval;

    return NC_NOERR;
}

/**
 * @internal Copy a DICOM frame buffer hyperslab into the user buffer.
 *
 * Both source and destination use the same row-major dimension order.
 * The source is indexed with start[] offsets; the destination is indexed
 * from zero over count[].
 *
 * @param src Source frame buffer.
 * @param dst Destination user buffer.
 * @param elem_size Size in bytes of one element.
 * @param ndims Number of dimensions.
 * @param dims Full dimension lengths.
 * @param start Start indices in source.
 * @param count Counts to copy along each dimension.
 *
 * @return NC_NOERR No error.
 */
static int
dicom_copy_slab(const void *src, void *dst, size_t elem_size,
                int ndims, const size_t dims[],
                const size_t start[], const size_t count[])
{
    size_t indices[NC_MAX_VAR_DIMS] = {0};
    size_t total = 1;
    size_t n;

    for (int d = 0; d < ndims; d++)
        total *= count[d];

    for (n = 0; n < total; n++)
    {
        size_t src_offset = 0;
        size_t dst_offset = 0;
        size_t src_stride = 1;
        size_t dst_stride = 1;

        for (int d = ndims - 1; d >= 0; d--)
        {
            size_t src_idx = start[d] + indices[d];
            size_t dst_idx = indices[d];

            src_offset += src_idx * src_stride;
            dst_offset += dst_idx * dst_stride;
            src_stride *= dims[d];
            dst_stride *= count[d];
        }

        memcpy((char *)dst + dst_offset * elem_size,
               (const char *)src + src_offset * elem_size,
               elem_size);

        /* Increment indices, innermost first. */
        for (int d = ndims - 1; d >= 0; d--)
        {
            indices[d]++;
            if (indices[d] < count[d])
                break;
            indices[d] = 0;
        }
    }

    return NC_NOERR;
}
#endif /* HAVE_DICOM */

/**
 * @internal Open a DICOM file for read-only access.
 *
 * @param path Path to the DICOM file.
 * @param mode Open mode flags.
 * @param basepe Ignored.
 * @param chunksizehintp Ignored.
 * @param parameters Ignored.
 * @param dispatch Pointer to dispatch table.
 * @param ncid NetCDF ID assigned to this file.
 *
 * @return NC_NOERR No error.
 * @return NC_EINVAL Invalid parameters or mode flags.
 * @return NC_EPERM Write mode requested.
 * @return NC_ENOMEM Out of memory.
 * @author Edward Hartnett
 */
int
NC_DICOM_open(const char *path, int mode, int basepe, size_t *chunksizehintp,
              void *parameters, const NC_Dispatch *dispatch, int ncid)
{
    NC *nc;
    NC_FILE_INFO_T *h5;
    NC_DICOM_FILE_INFO_T *dicom_file;
    int retval;
#ifdef HAVE_DICOM
    DcmError *error = NULL;
#endif

    assert(basepe || !basepe);
    assert(chunksizehintp || !chunksizehintp);
    assert(parameters || !parameters);
    assert(dispatch);

    if (!path)
        return NC_EINVAL;

    /* Only read-only access is supported. */
    if (mode & NC_WRITE)
        return NC_EPERM;

    /* Find pointer to NC. */
    if ((retval = NC_check_id(ncid, &nc)))
        return retval;

    /* Add necessary structs to hold netcdf-4 file data. */
    if ((retval = nc4_file_list_add(ncid, path, mode, (void **)&h5)))
        return retval;
    assert(h5 && h5->root_grp);
    h5->no_write = NC_TRUE;
    h5->root_grp->atts_read = 1;

    /* Allocate DICOM-specific file info. */
    if (!(dicom_file = calloc(1, sizeof(NC_DICOM_FILE_INFO_T))))
        return NC_ENOMEM;

    if (!(dicom_file->path = strdup(path)))
    {
        free(dicom_file);
        return NC_ENOMEM;
    }

#ifdef HAVE_DICOM
    /* Open the DICOM file with libdicom. */
    dicom_file->filehandle = dcm_filehandle_create_from_file(&error, path);
    if (dicom_file->filehandle == NULL)
    {
        if (error)
        {
            dcm_error_log(error);
            dcm_error_clear(&error);
        }
        free(dicom_file->path);
        free(dicom_file);
        return NC_EINVAL;
    }

    /* Read the metadata subset (File Meta + Data Set metadata before PixelData). */
    dicom_file->metadata = dcm_filehandle_get_metadata_subset(&error,
                                                            dicom_file->filehandle);
    if (dicom_file->metadata == NULL)
    {
        if (error)
        {
            dcm_error_log(error);
            dcm_error_clear(&error);
        }
        dcm_filehandle_destroy(dicom_file->filehandle);
        free(dicom_file->path);
        free(dicom_file);
        return NC_EINVAL;
    }

    h5->format_file_info = dicom_file;

    /* Populate the netCDF-4 in-memory model from DICOM metadata. */
    if ((retval = dicom_read_image_metadata(h5)))
    {
        dcm_filehandle_destroy(dicom_file->filehandle);
        free(dicom_file->transfer_syntax_uid);
        free(dicom_file->path);
        free(dicom_file);
        h5->format_file_info = NULL;
        return retval;
    }
#else
    (void)nc;
    (void)dispatch;
    (void)ncid;
    dicom_file->filehandle = NULL;
    dicom_file->metadata = NULL;
    h5->format_file_info = dicom_file;
#endif

    return NC_NOERR;
}

/**
 * @internal Close a DICOM file.
 *
 * @param ncid NetCDF ID.
 * @param ignore Ignored.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid.
 * @author Edward Hartnett
 */
int
NC_DICOM_close(int ncid, void *ignore)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_DICOM_FILE_INFO_T *dicom_file;
    int retval;

    assert(ignore || !ignore);

    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    dicom_file = (NC_DICOM_FILE_INFO_T *)h5->format_file_info;
    if (!dicom_file)
        return NC_NOERR;

#ifdef HAVE_DICOM
    if (dicom_file->filehandle)
        dcm_filehandle_destroy(dicom_file->filehandle);
#endif

    free(dicom_file->transfer_syntax_uid);
    free(dicom_file->path);
    free(dicom_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Abort opening a DICOM file.
 *
 * @param ncid NetCDF ID.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_DICOM_abort(int ncid)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_DICOM_FILE_INFO_T *dicom_file;
    int retval;

    if ((retval = nc4_find_grp_h5(ncid, &grp, &h5)))
        return retval;

    dicom_file = (NC_DICOM_FILE_INFO_T *)h5->format_file_info;
    if (!dicom_file)
        return NC_NOERR;

#ifdef HAVE_DICOM
    if (dicom_file->filehandle)
        dcm_filehandle_destroy(dicom_file->filehandle);
#endif

    free(dicom_file->transfer_syntax_uid);
    free(dicom_file->path);
    free(dicom_file);
    h5->format_file_info = NULL;

    return NC_NOERR;
}

/**
 * @internal Inquire the format of a DICOM file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_DICOM_inq_format(int ncid, int *formatp)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMAT_NETCDF4;
    return NC_NOERR;
}

/**
 * @internal Inquire the extended format of a DICOM file.
 *
 * @param ncid NetCDF ID.
 * @param formatp Pointer that gets format code.
 * @param modep Pointer that gets mode flags.
 *
 * @return NC_NOERR No error.
 * @author Edward Hartnett
 */
int
NC_DICOM_inq_format_extended(int ncid, int *formatp, int *modep)
{
    (void)ncid;
    if (formatp)
        *formatp = NC_FORMATX_NC_DICOM;
    if (modep)
        *modep = NC_NOWRITE;
    return NC_NOERR;
}

#ifdef HAVE_DICOM
/**
 * @internal Read a hyperslab of data from the DICOM pixel_data variable.
 *
 * Reads the single frame via libdicom and copies the requested subset
 * into the user buffer. The frame buffer and the NetCDF variable share
 * the same row-major dimension order.
 *
 * @param ncid NetCDF ID.
 * @param varid Variable ID.
 * @param start Start indices (0-based).
 * @param count Counts.
 * @param value Output buffer.
 * @param memtype Requested memory type.
 *
 * @return NC_NOERR No error.
 * @return NC_EBADID Bad ncid or varid.
 * @return NC_EINVAL Invalid parameters.
 * @return NC_EIO Read error.
 * @author Edward Hartnett
 */
int
NC_DICOM_get_vara(int ncid, int varid, const size_t *start,
                  const size_t *count, void *value, nc_type memtype)
{
    NC_FILE_INFO_T *h5;
    NC_GRP_INFO_T *grp;
    NC_VAR_INFO_T *var;
    NC_DICOM_FILE_INFO_T *dicom_file;
    NC_DICOM_VAR_INFO_T *dicom_var;
    DcmError *error = NULL;
    DcmFrame *frame = NULL;
    const char *frame_data = NULL;
    uint32_t frame_length = 0;
    size_t dims[NC_MAX_VAR_DIMS];
    int ndims;
    int retval;

    if (!start || !count || !value)
        return NC_EINVAL;

    if ((retval = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return retval;
    if (!var || !var->format_var_info)
        return NC_EBADID;

    dicom_file = (NC_DICOM_FILE_INFO_T *)h5->format_file_info;
    dicom_var = (NC_DICOM_VAR_INFO_T *)var->format_var_info;

    if (dicom_file->encapsulated)
        return NC_EINVAL;  /* Compressed pixel data not supported yet */

    /* Use variable's own type if caller did not specify. */
    if (memtype == NC_NAT)
        memtype = (nc_type)var->type_info->hdr.id;

    if (memtype != (nc_type)var->type_info->hdr.id)
        return NC_EBADTYPE;

    ndims = var->ndims;
    for (int d = 0; d < ndims; d++)
    {
        dims[d] = dicom_dimid_to_len(grp, var->dimids[d]);
        if (dims[d] == 0)
            return NC_EINVAL;
        if (start[d] + count[d] > dims[d])
            return NC_EINVAL;
    }

    /* Prepare to read frames and read the requested frame. */
    if (!dcm_filehandle_prepare_read_frame(&error, dicom_file->filehandle))
    {
        if (error)
        {
            dcm_error_log(error);
            dcm_error_clear(&error);
        }
        return NC_EIO;
    }

    frame = dcm_filehandle_read_frame(&error, dicom_file->filehandle,
                                      (uint32_t)dicom_var->frame_index);
    if (frame == NULL)
    {
        if (error)
        {
            dcm_error_log(error);
            dcm_error_clear(&error);
        }
        return NC_EIO;
    }

    frame_data = dcm_frame_get_value(frame);
    frame_length = dcm_frame_get_length(frame);
    if (!frame_data || frame_length == 0)
    {
        /* libdicom owns frame memory; destroy frame before returning. */
        dcm_frame_destroy(frame);
        return NC_EIO;
    }

    /* Defensive: ensure the frame is large enough for the full image. */
    {
        size_t expected = dicom_file->type_size;
        for (int d = 0; d < ndims; d++)
            expected *= dims[d];

        if (frame_length < expected)
        {
            dcm_frame_destroy(frame);
            return NC_EIO;
        }
    }

    /* Copy the requested hyperslab into the user buffer. */
    retval = dicom_copy_slab(frame_data, value, dicom_file->type_size,
                               ndims, dims, start, count);

    dcm_frame_destroy(frame);
    return retval;
}
#else /* !HAVE_DICOM */
int
NC_DICOM_get_vara(int ncid, int varid, const size_t *start, const size_t *count,
                  void *value, nc_type memtype)
{
    (void)ncid; (void)varid; (void)start; (void)count; (void)value; (void)memtype;
    return NC_ENOTBUILT;
}
#endif /* HAVE_DICOM */
