/**
 * @file
 * @brief Internal API for decoding encapsulated JPEG Lossless (Process 14)
 * DICOM pixel data.
 *
 * JPEG Lossless (Transfer Syntax UIDs 1.2.840.10008.1.2.4.57 and
 * 1.2.840.10008.1.2.4.70) is not supported by mainline libjpeg/libjpeg-turbo,
 * which only implements baseline/sequential DCT-based JPEG. Decoding uses
 * GDCM's bundled IJG libjpeg 6b codec, patched with the classic lossless
 * (SOF3) extension. GDCM ships three separately-compiled, symbol-mangled
 * variants of this codec (8/12/16-bit sample precision); the correct variant
 * is selected at runtime based on the JPEG frame's own data precision, which
 * is read directly from the compressed stream's SOF3 marker.
 *
 * @author Edward Hartnett
 * @date 2026-07-28
 * @copyright Intelligent Data Design, Inc. All rights reserved.
 */
#ifndef _DICOMJPEGLOSSLESS_H
#define _DICOMJPEGLOSSLESS_H

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @internal Determine the JPEG data precision (bits per sample) encoded in
 * a JPEG Lossless (SOF3) frame's start-of-frame marker.
 *
 * @param src JPEG encoded frame bytes.
 * @param src_len Length of JPEG input in bytes.
 *
 * @return The data precision (typically 8, 12, or 16), or -1 if no SOF3
 * marker was found.
 */
int dicom_jpeg_lossless_precision(const void *src, size_t src_len);

/**
 * @internal Decompress a JPEG Lossless (8-bit precision) encapsulated
 * DICOM frame.
 *
 * @param src JPEG encoded frame bytes.
 * @param src_len Length of JPEG input in bytes.
 * @param width Expected image width (columns).
 * @param height Expected image height (rows).
 * @param components Expected number of components (samples per pixel).
 * @param sample_size Expected size in bytes of one decoded sample.
 * @param bufp Receives a malloc'd decompressed frame buffer.
 * @param buf_lenp Receives the length of the decompressed buffer.
 *
 * @return 0 on success, non-zero on failure.
 */
int dicom_jpeg_lossless_decode8(const void *src, size_t src_len,
                                 size_t width, size_t height,
                                 size_t components, size_t sample_size,
                                 void **bufp, size_t *buf_lenp);

/**
 * @internal Decompress a JPEG Lossless (9-12-bit precision) encapsulated
 * DICOM frame. See dicom_jpeg_lossless_decode8() for parameter details.
 */
int dicom_jpeg_lossless_decode12(const void *src, size_t src_len,
                                  size_t width, size_t height,
                                  size_t components, size_t sample_size,
                                  void **bufp, size_t *buf_lenp);

/**
 * @internal Decompress a JPEG Lossless (13-16-bit precision) encapsulated
 * DICOM frame. See dicom_jpeg_lossless_decode8() for parameter details.
 */
int dicom_jpeg_lossless_decode16(const void *src, size_t src_len,
                                  size_t width, size_t height,
                                  size_t components, size_t sample_size,
                                  void **bufp, size_t *buf_lenp);

#if defined(__cplusplus)
}
#endif

#endif /* _DICOMJPEGLOSSLESS_H */
