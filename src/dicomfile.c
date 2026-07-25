/**
 * @file dicomfile.c
 * @brief DICOM User-Defined Format (UDF) dispatch layer.
 *
 * Implements the NEP DICOM reader, which maps DICOM image SOP Instances
 * to the netCDF-4 data model via libdicom. This Sprint 2 implementation
 * supports native (uncompressed) and encapsulated JPEG Baseline images,
 * including multi-frame objects. Other transfer syntaxes may be rejected.
 *
 * - Primary image: exposed as a `pixel_data` variable in the root group
 *   with dimensions derived from NumberOfFrames, Rows, Columns,
 *   SamplesPerPixel, and PlanarConfiguration.
 * - Pixel type is derived from BitsAllocated and PixelRepresentation.
 * - Patient/Study/Series/Image Pixel module tags are mapped to global
 *   and variable attributes.
 * - Data reading: NC_DICOM_get_vara() reads the requested frames via
 *   libdicom's dcm_filehandle_read_frame() and copies the hyperslab
 *   into the user buffer. JPEG frames are decompressed with libjpeg.
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

/* Include libdicom and libjpeg headers if available */
#ifdef HAVE_DICOM
#include <dicom/dicom.h>
#include <jpeglib.h>
#include <setjmp.h>
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
 * Integer-valued tags (US, SS, UL, SL, etc.) are read directly. String
 * numeric tags (IS, DS) are parsed into int64_t so that values such as
 * NumberOfFrames are available.
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
    DcmVR vr;

    tag = dcm_dict_tag_from_keyword(keyword);
    if (tag == 0)
        return 0;

    element = dcm_dataset_get(error, dataset, tag);
    if (element == NULL)
        return 0;

    vr = dcm_element_get_vr(element);
    if (vr == DCM_VR_IS || vr == DCM_VR_DS)
    {
        const char *str = NULL;
        char *endptr = NULL;
        double dval;
        long long ival;

        if (!dcm_element_get_value_string(error, element, 0, &str))
            return 0;
        if (!str)
            return 0;

        if (vr == DCM_VR_IS)
        {
            ival = strtoll(str, &endptr, 10);
            if (endptr == str || *endptr != '\0')
                return 0;
            *valuep = (int64_t)ival;
        }
        else
        {
            dval = strtod(str, &endptr);
            if (endptr == str || *endptr != '\0')
                return 0;
            *valuep = (int64_t)(dval >= 0.0 ? dval + 0.5 : dval - 0.5);
        }
        return 1;
    }

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

#ifdef HAVE_DICOM

/**
 * @internal libjpeg error handler that uses longjmp instead of exit().
 */
struct dicom_jpeg_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

/**
 * @internal libjpeg error_exit replacement.
 */
static void
dicom_jpeg_error_exit(j_common_ptr cinfo)
{
    struct dicom_jpeg_error_mgr *myerr =
        (struct dicom_jpeg_error_mgr *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

/**
 * @internal Decompress a JPEG Baseline encapsulated DICOM frame.
 *
 * @param dicom_file DICOM file state (rows, columns, samples, type size).
 * @param src JPEG encoded frame bytes.
 * @param src_len Length of JPEG input.
 * @param bufp Receives malloc'd decompressed frame buffer.
 * @param buf_lenp Receives length of decompressed buffer.
 *
 * @return NC_NOERR on success.
 * @return NC_ENOMEM or NC_EIO on failure.
 */
static int
dicom_decompress_jpeg(NC_DICOM_FILE_INFO_T *dicom_file,
                      const void *src, size_t src_len,
                      void **bufp, size_t *buf_lenp)
{
    struct jpeg_decompress_struct cinfo;
    struct dicom_jpeg_error_mgr jerr;
    JSAMPARRAY scanline_buffer = NULL;
    size_t samples_per_pixel;
    size_t row_stride;
    size_t out_size;
    unsigned char *out = NULL;
    unsigned char *out_ptr;
    int retval = NC_NOERR;

    *bufp = NULL;
    *buf_lenp = 0;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = dicom_jpeg_error_exit;

    if (setjmp(jerr.setjmp_buffer))
    {
        retval = NC_EIO;
        goto cleanup;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (const unsigned char *)src, (unsigned long)src_len);
    jpeg_read_header(&cinfo, TRUE);

    if (dicom_file->samples_per_pixel == 3)
        cinfo.out_color_space = JCS_RGB;
    else
        cinfo.out_color_space = JCS_GRAYSCALE;

    jpeg_start_decompress(&cinfo);

    samples_per_pixel = (size_t)cinfo.output_components;

    if (samples_per_pixel != (size_t)dicom_file->samples_per_pixel)
    {
        retval = NC_EINVAL;
        goto cleanup;
    }
    if ((size_t)cinfo.output_width != dicom_file->columns ||
        (size_t)cinfo.output_height != dicom_file->rows)
    {
        retval = NC_EINVAL;
        goto cleanup;
    }

    row_stride = (size_t)cinfo.output_width * samples_per_pixel *
        dicom_file->type_size;
    out_size = row_stride * (size_t)cinfo.output_height;

    if (!(out = malloc(out_size)))
    {
        retval = NC_ENOMEM;
        goto cleanup;
    }

    scanline_buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);
    if (!scanline_buffer)
    {
        retval = NC_ENOMEM;
        goto cleanup;
    }

    out_ptr = out;
    while (cinfo.output_scanline < cinfo.output_height)
    {
        jpeg_read_scanlines(&cinfo, scanline_buffer, 1);
        memcpy(out_ptr, scanline_buffer[0], row_stride);
        out_ptr += row_stride;
    }

    jpeg_finish_decompress(&cinfo);

cleanup:
    jpeg_destroy_decompress(&cinfo);
    if (retval != NC_NOERR)
    {
        free(out);
        return retval;
    }

    *bufp = out;
    *buf_lenp = out_size;
    return NC_NOERR;
}

/**
 * @internal Read or decompress one DICOM frame into a malloc'd buffer.
 *
 * The returned buffer must be freed by the caller.
 *
 * @param dicom_file DICOM file state.
 * @param frame_number One-based frame number (libdicom convention).
 * @param bufp Receives malloc'd frame buffer.
 * @param buf_lenp Receives length of buffer in bytes.
 *
 * @return NC_NOERR on success.
 */
static int
dicom_read_frame_buffer(NC_DICOM_FILE_INFO_T *dicom_file,
                        uint32_t frame_number,
                        void **bufp, size_t *buf_lenp)
{
    DcmError *error = NULL;
    DcmFrame *frame = NULL;
    const char *frame_data = NULL;
    uint32_t frame_length = 0;
    void *buf = NULL;
    size_t expected_len;
    size_t copy_len;
    int ret;

    *bufp = NULL;
    *buf_lenp = 0;

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
                                      frame_number);
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
        dcm_frame_destroy(frame);
        return NC_EIO;
    }

    if (dicom_file->encapsulated)
    {
        ret = dicom_decompress_jpeg(dicom_file, frame_data, frame_length,
                                    bufp, buf_lenp);
        dcm_frame_destroy(frame);
        return ret;
    }

    expected_len = dicom_file->type_size * dicom_file->rows *
        dicom_file->columns * (size_t)dicom_file->samples_per_pixel;
    copy_len = (size_t)frame_length < expected_len ? (size_t)frame_length :
        expected_len;

    if (!(buf = malloc(expected_len)))
    {
        dcm_frame_destroy(frame);
        return NC_ENOMEM;
    }
    memcpy(buf, frame_data, copy_len);
    if (copy_len < expected_len)
        memset((char *)buf + copy_len, 0, expected_len - copy_len);

    dcm_frame_destroy(frame);
    *bufp = buf;
    *buf_lenp = expected_len;
    return NC_NOERR;
}

#endif /* HAVE_DICOM */

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
 * @internal Determine whether the given Transfer Syntax UID is the JPEG
 * Baseline transfer syntax supported by this sprint.
 *
 * @param uid Transfer Syntax UID string.
 *
 * @return Non-zero if JPEG Baseline.
 */
static int
dicom_is_jpeg_baseline_transfer_syntax(const char *uid)
{
    if (!uid)
        return 0;

    return (strcmp(uid, "1.2.840.10008.1.2.4.50") == 0);   /* JPEG Baseline */
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

    dicom_file->encapsulated = !dicom_is_native_transfer_syntax(transfer_syntax_uid);

    /* Sprint 2 only supports native syntaxes and JPEG Baseline. */
    if (dicom_file->encapsulated &&
        !dicom_is_jpeg_baseline_transfer_syntax(transfer_syntax_uid))
        return NC_EINVAL;

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
    if (nframes < 1)
        nframes = 1;
    if (bits_stored == 0)
        bits_stored = bits_allocated;
    if (high_bit == 0 && bits_stored > 0)
        high_bit = bits_stored - 1;

    dicom_file->nframes = (int)nframes;

    dicom_file->rows = (size_t)rows;
    dicom_file->columns = (size_t)columns;
    dicom_file->samples_per_pixel = (int)samples_per_pixel;
    dicom_file->bits_allocated = (int)bits_allocated;
    dicom_file->bits_stored = (int)bits_stored;
    dicom_file->high_bit = (int)high_bit;
    dicom_file->pixel_representation = (int)pixel_representation;
    dicom_file->planar_configuration = (int)planar_configuration;
    dicom_file->frame_dim_index = 0;
    dicom_file->row_dim_index = 1;
    dicom_file->col_dim_index = 2;
    dicom_file->sample_dim_index = (samples_per_pixel > 1) ? 3 : -1;
    dicom_file->color_dim_index = dicom_file->sample_dim_index;

    if ((retval = dicom_bits_to_nc_type((int)bits_allocated,
                                        (int)pixel_representation,
                                        &xtype, &type_size, type_name,
                                        &endianness)))
        return retval;

    dicom_file->xtype = xtype;
    dicom_file->type_size = type_size;

    /* Create dimensions in a uniform NetCDF layout:
     *   [frame][row][column] for grayscale, and
     *   [frame][row][column][sample] for color.
     * Native planar data is re-ordered during reads.
     */
    {
        NC_DIM_INFO_T *dim;

        if ((retval = nc4_dim_list_add(grp, "frame", (size_t)nframes, -1,
                                        &dim)))
            return retval;
        dimids[ndims++] = dim->hdr.id;

        if ((retval = nc4_dim_list_add(grp, "row", dicom_file->rows, -1, &dim)))
            return retval;
        dimids[ndims++] = dim->hdr.id;

        if ((retval = nc4_dim_list_add(grp, "column", dicom_file->columns, -1,
                                       &dim)))
            return retval;
        dimids[ndims++] = dim->hdr.id;

        if (samples_per_pixel > 1)
        {
            if ((retval = nc4_dim_list_add(grp, "sample",
                                            (size_t)samples_per_pixel,
                                            -1, &dim)))
                return retval;
            dimids[ndims++] = dim->hdr.id;
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
    if ((retval = dicom_add_var_int_att(var, "NumberOfFrames", nframes)))
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
 * @internal Return true for integer netCDF atomic types.
 */
static int
dicom_type_is_int(nc_type xtype)
{
    return (xtype == NC_BYTE || xtype == NC_UBYTE ||
            xtype == NC_SHORT || xtype == NC_USHORT ||
            xtype == NC_INT || xtype == NC_UINT ||
            xtype == NC_INT64 || xtype == NC_UINT64);
}

/**
 * @internal Return true for unsigned integer netCDF atomic types.
 */
static int
dicom_type_is_unsigned(nc_type xtype)
{
    return (xtype == NC_UBYTE || xtype == NC_USHORT ||
            xtype == NC_UINT || xtype == NC_UINT64);
}

/**
 * @internal Convert a single DICOM pixel value from the file type to the
 * requested memory type.  The conversion is done via the widest integer
 * types so that widening/narrowing between signed and unsigned integer
 * types behaves the same as a C cast.
 */
static int
dicom_convert_pixel(void *dst, size_t dst_size, nc_type dst_type,
                    const void *src, size_t src_size, nc_type src_type)
{
    unsigned long long uval = 0;
    long long sval = 0;
    int src_is_unsigned = dicom_type_is_unsigned(src_type);
    int dst_is_unsigned = dicom_type_is_unsigned(dst_type);

    if (!dicom_type_is_int(src_type) || !dicom_type_is_int(dst_type))
        return NC_EBADTYPE;

    /* Read the source value as the widest signed/unsigned integer. */
    if (src_is_unsigned)
    {
        switch (src_size)
        {
        case 1: uval = *(const unsigned char *)src; break;
        case 2: uval = *(const unsigned short *)src; break;
        case 4: uval = *(const unsigned int *)src; break;
        case 8: uval = *(const unsigned long long *)src; break;
        default: return NC_EBADTYPE;
        }
    }
    else
    {
        switch (src_size)
        {
        case 1: sval = *(const signed char *)src; break;
        case 2: sval = *(const short *)src; break;
        case 4: sval = *(const int *)src; break;
        case 8: sval = *(const long long *)src; break;
        default: return NC_EBADTYPE;
        }
    }

    /* Write the destination using C cast semantics. */
    if (src_is_unsigned && !dst_is_unsigned)
    {
        long long v = (long long)uval;
        switch (dst_size)
        {
        case 1: *(signed char *)dst = (signed char)v; break;
        case 2: *(short *)dst = (short)v; break;
        case 4: *(int *)dst = (int)v; break;
        case 8: *(long long *)dst = v; break;
        default: return NC_EBADTYPE;
        }
    }
    else if (!src_is_unsigned && dst_is_unsigned)
    {
        unsigned long long v = (unsigned long long)sval;
        switch (dst_size)
        {
        case 1: *(unsigned char *)dst = (unsigned char)v; break;
        case 2: *(unsigned short *)dst = (unsigned short)v; break;
        case 4: *(unsigned int *)dst = (unsigned int)v; break;
        case 8: *(unsigned long long *)dst = v; break;
        default: return NC_EBADTYPE;
        }
    }
    else if (src_is_unsigned)
    {
        unsigned long long v = uval;
        switch (dst_size)
        {
        case 1: *(unsigned char *)dst = (unsigned char)v; break;
        case 2: *(unsigned short *)dst = (unsigned short)v; break;
        case 4: *(unsigned int *)dst = (unsigned int)v; break;
        case 8: *(unsigned long long *)dst = v; break;
        default: return NC_EBADTYPE;
        }
    }
    else
    {
        long long v = sval;
        switch (dst_size)
        {
        case 1: *(signed char *)dst = (signed char)v; break;
        case 2: *(short *)dst = (short)v; break;
        case 4: *(int *)dst = (int)v; break;
        case 8: *(long long *)dst = v; break;
        default: return NC_EBADTYPE;
        }
    }

    return NC_NOERR;
}

/**
 * @internal Read a hyperslab of data from the DICOM pixel_data variable.
 *
 * Reads the requested frames via libdicom (and libjpeg for encapsulated
 * JPEG Baseline frames) and copies the requested subset into the user
 * buffer. The NetCDF variable uses a uniform [frame][row][column][sample]
 * layout; native source data is re-ordered if PlanarConfiguration requires.
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
    size_t dims[NC_MAX_VAR_DIMS];
    int ndims;
    int retval;
    int frame_dim, row_dim, col_dim, sample_dim;
    size_t frame_start, row_start, col_start, sample_start;
    size_t nframes_req, row_count, col_count, sample_count;
    size_t f, r, c, s;
    nc_type src_type;
    size_t src_size, dst_size;

    if (!start || !count || !value)
        return NC_EINVAL;

    if ((retval = nc4_find_grp_h5_var(ncid, varid, &h5, &grp, &var)))
        return retval;
    if (!var || !var->format_var_info)
        return NC_EBADID;

    dicom_file = (NC_DICOM_FILE_INFO_T *)h5->format_file_info;

    src_type = (nc_type)var->type_info->hdr.id;
    src_size = dicom_file->type_size;

    /* Use variable's own type if caller did not specify. */
    if (memtype == NC_NAT)
        memtype = src_type;

    /* Reject unsupported memory types, but allow integer conversions. */
    if (nc_inq_type(ncid, memtype, NULL, &dst_size))
        return NC_EBADTYPE;
    if (memtype != src_type &&
        (!dicom_type_is_int(src_type) || !dicom_type_is_int(memtype)))
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

    frame_dim = dicom_file->frame_dim_index;
    row_dim = dicom_file->row_dim_index;
    col_dim = dicom_file->col_dim_index;
    sample_dim = dicom_file->sample_dim_index;

    if (frame_dim >= ndims || row_dim >= ndims || col_dim >= ndims)
        return NC_EINVAL;
    if (sample_dim >= 0 && sample_dim >= ndims)
        return NC_EINVAL;

    frame_start = start[frame_dim];
    nframes_req = count[frame_dim];
    row_start = start[row_dim];
    row_count = count[row_dim];
    col_start = start[col_dim];
    col_count = count[col_dim];
    sample_start = (sample_dim >= 0) ? start[sample_dim] : 0;
    sample_count = (sample_dim >= 0) ? count[sample_dim] : 1;

    /* Read each requested frame and copy the requested sub-slab. */
    for (f = 0; f < nframes_req; f++)
    {
        uint32_t frame_number = (uint32_t)(frame_start + f + 1);
        void *frame_buf = NULL;
        size_t frame_buf_len = 0;

        if ((retval = dicom_read_frame_buffer(dicom_file, frame_number,
                                                &frame_buf, &frame_buf_len)))
            return retval;

        for (r = 0; r < row_count; r++)
        {
            size_t src_r = row_start + r;
            for (c = 0; c < col_count; c++)
            {
                size_t src_c = col_start + c;
                for (s = 0; s < sample_count; s++)
                {
                    size_t src_s = sample_start + s;
                    size_t src_idx;
                    size_t dst_idx;

                    if (dicom_file->planar_configuration == 1 &&
                        dicom_file->samples_per_pixel > 1)
                        src_idx = (src_s * dicom_file->rows + src_r) *
                            dicom_file->columns + src_c;
                    else
                        src_idx = (src_r * dicom_file->columns + src_c) *
                            (size_t)dicom_file->samples_per_pixel + src_s;

                    dst_idx = ((f * row_count + r) * col_count + c) *
                        sample_count + s;

                    if (memtype == src_type)
                    {
                        memcpy((char *)value + dst_idx * src_size,
                               (char *)frame_buf + src_idx * src_size,
                               src_size);
                    }
                    else
                    {
                        retval = dicom_convert_pixel(
                            (char *)value + dst_idx * dst_size,
                            dst_size, memtype,
                            (char *)frame_buf + src_idx * src_size,
                            src_size, src_type);
                        if (retval)
                        {
                            free(frame_buf);
                            return retval;
                        }
                    }
                }
            }
        }

        free(frame_buf);
    }

    return NC_NOERR;
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
