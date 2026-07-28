/**
 * @file
 * @brief JPEG data-precision detection for JPEG Lossless frames.
 *
 * This is a plain byte scan of the raw JPEG stream for a Start-Of-Frame
 * marker and does not depend on any JPEG decoding library, so it is
 * always available whenever DICOM support is built, independent of
 * whether the JPEG Lossless codec (HAVE_DICOM_LOSSLESS) is available.
 *
 * @author Edward Hartnett
 * @date 2026-07-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#include "config.h"
#include "dicomjpeglossless.h"

int
dicom_jpeg_lossless_precision(const void *src, size_t src_len)
{
    const unsigned char *data = (const unsigned char *)src;
    size_t i;

    if (!data || src_len < 5)
        return -1;

    /* Scan for a Start-Of-Frame marker (0xFFC0-0xFFCF), skipping the
     * non-SOF markers in that range (DHT=0xC4, JPG=0xC8, DAC=0xCC). The
     * data precision is the byte immediately following the 2-byte segment
     * length field. */
    for (i = 0; i + 4 < src_len; i++)
    {
        if (data[i] == 0xFF && data[i + 1] >= 0xC0 && data[i + 1] <= 0xCF &&
            data[i + 1] != 0xC4 && data[i + 1] != 0xC8 && data[i + 1] != 0xCC)
            return (int)data[i + 4];
    }

    return -1;
}
