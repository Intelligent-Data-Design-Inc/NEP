/**
 * @file
 * @brief Shared implementation body for the per-precision JPEG Lossless
 * decode wrappers (dicomjpeglossless8.c, dicomjpeglossless12.c,
 * dicomjpeglossless16.c).
 *
 * This file is included, not compiled directly. Each wrapper defines
 * DICOM_LL_DECODE_FUNC_NAME to its own externally-visible function name and
 * includes this file after including the matching precision-specific
 * GDCM jpeglib.h, so that all IJG symbols referenced below resolve through
 * that precision's mangled namespace (e.g. gdcmjpeg12_jpeg_read_header).
 *
 * @author Edward Hartnett
 * @date 2026-07-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */

#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

/** @internal libjpeg error handler that uses longjmp instead of exit(). */
struct dicom_ll_error_mgr
{
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

/** @internal libjpeg error_exit replacement. */
static void
dicom_ll_error_exit(j_common_ptr cinfo)
{
    struct dicom_ll_error_mgr *myerr = (struct dicom_ll_error_mgr *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

/**
 * @internal Minimal memory-based source manager.
 *
 * These GDCM-bundled headers predate the standard IJG jpeg_mem_src() helper
 * (added in libjpeg 8), so a small custom source manager is required,
 * following the classic jdatasrc.c pattern: the entire compressed frame is
 * already in memory, so fill_input_buffer() only needs to run if the decoder
 * asks for more data than is available, in which case it is fed a synthetic
 * End-Of-Image marker.
 */
struct dicom_ll_mem_source_mgr
{
    struct jpeg_source_mgr pub;
};

/** @internal Source-manager init callback; the whole frame is already in
 * memory, so there is nothing to do.
 * @param cinfo libjpeg decompressor state (unused).
 */
static void
dicom_ll_init_source(j_decompress_ptr cinfo)
{
    (void)cinfo;
}

/** @internal Feed the decoder a synthetic End-Of-Image marker when it asks
 * for more data than the buffer holds; this is the classic jdatasrc.c
 * trick for truncated streams.
 * @param cinfo libjpeg decompressor state.
 * @return Always TRUE, as required by the source-manager contract.
 */
static boolean
dicom_ll_fill_input_buffer(j_decompress_ptr cinfo)
{
    static JOCTET eoi_buffer[2] = { 0xFF, JPEG_EOI };
    struct dicom_ll_mem_source_mgr *src =
        (struct dicom_ll_mem_source_mgr *)cinfo->src;

    src->pub.next_input_byte = eoi_buffer;
    src->pub.bytes_in_buffer = 2;
    return TRUE;
}

/** @internal Advance the read position past bytes the decoder skips,
 * refilling with the synthetic EOI marker when the skip runs off the end.
 * @param cinfo libjpeg decompressor state.
 * @param num_bytes Number of input bytes to discard.
 */
static void
dicom_ll_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
{
    struct dicom_ll_mem_source_mgr *src =
        (struct dicom_ll_mem_source_mgr *)cinfo->src;

    if (num_bytes <= 0)
        return;

    while ((size_t)num_bytes > src->pub.bytes_in_buffer)
    {
        num_bytes -= (long)src->pub.bytes_in_buffer;
        (void)dicom_ll_fill_input_buffer(cinfo);
    }
    src->pub.next_input_byte += (size_t)num_bytes;
    src->pub.bytes_in_buffer -= (size_t)num_bytes;
}

/** @internal Source-manager teardown callback; no resources to release
 * because the caller owns the compressed buffer.
 * @param cinfo libjpeg decompressor state (unused).
 */
static void
dicom_ll_term_source(j_decompress_ptr cinfo)
{
    (void)cinfo;
}

/** @internal Attach a memory source manager to the decompressor,
 * allocating it from the libjpeg permanent pool on first use.
 * @param cinfo libjpeg decompressor state.
 * @param buf Compressed JPEG Lossless frame.
 * @param len Length of @p buf in bytes.
 */
static void
dicom_ll_mem_src(j_decompress_ptr cinfo, const unsigned char *buf, size_t len)
{
    struct dicom_ll_mem_source_mgr *src;

    if (cinfo->src == NULL)
        cinfo->src = (struct jpeg_source_mgr *)
            (*cinfo->mem->alloc_small)((j_common_ptr)cinfo, JPOOL_PERMANENT,
                                       sizeof(struct dicom_ll_mem_source_mgr));

    src = (struct dicom_ll_mem_source_mgr *)cinfo->src;
    src->pub.init_source = dicom_ll_init_source;
    src->pub.fill_input_buffer = dicom_ll_fill_input_buffer;
    src->pub.skip_input_data = dicom_ll_skip_input_data;
    src->pub.resync_to_restart = jpeg_resync_to_restart;
    src->pub.term_source = dicom_ll_term_source;
    src->pub.bytes_in_buffer = len;
    src->pub.next_input_byte = (const JOCTET *)buf;
}

/** @internal Decompress one JPEG Lossless frame at this wrapper's bit
 * precision into a freshly allocated buffer.
 * @param src Compressed JPEG Lossless frame.
 * @param src_len Length of @p src in bytes.
 * @param width Expected image width in pixels.
 * @param height Expected image height in pixels.
 * @param components Expected number of components per pixel.
 * @param sample_size Bytes per sample for this precision's output.
 * @param bufp Destination for the decoded buffer; caller frees.
 * @param buf_lenp Destination for the decoded buffer length in bytes.
 * @return 0 on success, -1 on a libjpeg error, -2 on a dimension
 * mismatch, or -3 on an allocation failure.
 */
int
DICOM_LL_DECODE_FUNC_NAME(const void *src, size_t src_len,
                          size_t width, size_t height, size_t components,
                          size_t sample_size, void **bufp, size_t *buf_lenp)
{
    struct jpeg_decompress_struct cinfo;
    struct dicom_ll_error_mgr jerr;
    JSAMPARRAY scanline_buffer = NULL;
    unsigned char *out = NULL;
    unsigned char *out_ptr;
    size_t row_stride, out_size;
    int retval = 0;

    *bufp = NULL;
    *buf_lenp = 0;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = dicom_ll_error_exit;

    if (setjmp(jerr.setjmp_buffer))
    {
        retval = -1;
        goto cleanup;
    }

    jpeg_create_decompress(&cinfo);
    cinfo.src = NULL;
    dicom_ll_mem_src(&cinfo, (const unsigned char *)src, src_len);
    jpeg_read_header(&cinfo, TRUE);

    jpeg_start_decompress(&cinfo);

    if ((size_t)cinfo.output_width != width ||
        (size_t)cinfo.output_height != height ||
        (size_t)cinfo.output_components != components)
    {
        retval = -2;
        goto cleanup;
    }

    row_stride = width * components * sample_size;
    out_size = row_stride * height;

    if (!(out = malloc(out_size)))
    {
        retval = -3;
        goto cleanup;
    }

    scanline_buffer = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, (JDIMENSION)row_stride, 1);
    if (!scanline_buffer)
    {
        retval = -3;
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
    if (retval != 0)
    {
        free(out);
        return retval;
    }

    *bufp = out;
    *buf_lenp = out_size;
    return 0;
}
