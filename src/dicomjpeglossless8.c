/**
 * @file
 * @brief 8-bit precision JPEG Lossless decode wrapper.
 *
 * Compiled with GDCM's gdcmjpeg/8 headers on its include path (set via
 * per-source-file CMake properties, not the target's global include
 * directories, so the 8/12/16-bit variants do not collide). See
 * dicomjpeglosslessimpl.inc.c for the shared implementation.
 *
 * @author Edward Hartnett
 * @date 2026-07-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"

#ifdef HAVE_DICOM_LOSSLESS

/* The classic IJG jpeglib.h requires stdio.h (for FILE) and stddef.h (for
 * size_t) to be included first; see IJG's libjpeg.txt. */
#include <stdio.h>
#include <stddef.h>
#include <jpeglib.h>
#include "dicomjpeglossless.h"

#define DICOM_LL_DECODE_FUNC_NAME dicom_jpeg_lossless_decode8
#include "dicomjpeglosslessimpl.inc.c"
#undef DICOM_LL_DECODE_FUNC_NAME

#else /* !HAVE_DICOM_LOSSLESS */

#include "dicomjpeglossless.h"

int
dicom_jpeg_lossless_decode8(const void *src, size_t src_len,
                             size_t width, size_t height, size_t components,
                             size_t sample_size, void **bufp, size_t *buf_lenp)
{
    (void)src; (void)src_len; (void)width; (void)height; (void)components;
    (void)sample_size;
    *bufp = NULL;
    *buf_lenp = 0;
    return -1;
}

#endif /* HAVE_DICOM_LOSSLESS */
