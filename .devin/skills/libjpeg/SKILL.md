---
name: libjpeg
description: Using libjpeg / libjpeg-turbo for 8-bit JPEG decompression from memory, with build-system detection and error handling suitable for NEP UDF readers.
metadata:
  author: netcdf-analysis
  version: "1.0"
  date: "2026-07-25"
---

# libjpeg Skill

This skill covers the IJG/libjpeg-turbo C API for decompressing 8-bit JPEG
images from an in-memory buffer. It is intended for NEP UDF handlers that need
to decode encapsulated pixel data (e.g., DICOM JPEG Baseline frames).

## Build System Detection

### CMake

```cmake
find_package(JPEG REQUIRED)
if(JPEG_FOUND)
    target_link_libraries(myudf PRIVATE ${JPEG_LIBRARIES})
    target_include_directories(myudf PRIVATE ${JPEG_INCLUDE_DIRS})
endif()
```

`JPEG_LIBRARIES` is typically `libjpeg.so` / `libjpeg.dll`.
`JPEG_INCLUDE_DIRS` contains `jpeglib.h`.

### Autotools

```bash
AC_CHECK_HEADERS([jpeglib.h], [jpeg_header=yes], [jpeg_header=no])
AC_CHECK_LIB([jpeg], [jpeg_std_error],
             [jpeg_lib=yes; JPEG_LIBS="-ljpeg"], [jpeg_lib=no])
AC_SUBST([JPEG_LIBS])
```

Link `$(JPEG_LIBS)` into the UDF library.

## Headers

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>
```

`<setjmp.h>` is required for the error-recovery technique shown below.

## Decompressing JPEG from Memory

The standard sequence is:

1. Allocate `struct jpeg_decompress_struct cinfo` and a custom error handler.
2. Override `error_exit` so the library does not call `exit()`.
3. `jpeg_create_decompress(&cinfo)`.
4. `jpeg_mem_src(&cinfo, buffer, buffer_len)`.
5. `jpeg_read_header(&cinfo, TRUE)`.
6. Optionally set `cinfo.out_color_space` to `JCS_GRAYSCALE` or `JCS_RGB`.
7. `jpeg_start_decompress(&cinfo)`.
8. Loop `jpeg_read_scanlines()`.
9. `jpeg_finish_decompress(&cinfo)`.
10. `jpeg_destroy_decompress(&cinfo)`.

### Error Handling

libjpeg's default error handler calls `exit()`. Always replace it:

```c
struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

typedef struct my_error_mgr *my_error_ptr;

static void
my_error_exit(j_common_ptr cinfo)
{
    my_error_ptr myerr = (my_error_ptr)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}
```

Usage:

```c
struct jpeg_decompress_struct cinfo;
struct my_error_mgr jerr;

if (setjmp(jerr.setjmp_buffer)) {
    jpeg_destroy_decompress(&cinfo);
    return NC_EIO;
}

cinfo.err = jpeg_std_error(&jerr.pub);
jerr.pub.error_exit = my_error_exit;

jpeg_create_decompress(&cinfo);
/* ... decompress ... */
```

`jpeg_destroy_decompress()` and `jpeg_abort()` are the only safe calls on a
JPEG object that has reported a fatal error.

### Full Memory-to-Memory Decompression Example

```c
static int
decompress_jpeg(const void *src, size_t src_len,
                int want_rgb,          /* 0 = grayscale, 1 = RGB */
                unsigned char **outp, size_t *out_lenp,
                size_t *widthp, size_t *heightp, int *componentsp)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY scanline;
    unsigned char *out = NULL;
    unsigned char *out_ptr;
    size_t row_stride;
    int retval = 0;

    *outp = NULL;
    *out_lenp = 0;

    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        free(out);
        return -1;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, (const unsigned char *)src, (unsigned long)src_len);
    jpeg_read_header(&cinfo, TRUE);

    if (want_rgb)
        cinfo.out_color_space = JCS_RGB;
    else
        cinfo.out_color_space = JCS_GRAYSCALE;

    jpeg_start_decompress(&cinfo);

    *widthp = cinfo.output_width;
    *heightp = cinfo.output_height;
    *componentsp = cinfo.output_components;

    row_stride = cinfo.output_width * cinfo.output_components * sizeof(JSAMPLE);
    *out_lenp = row_stride * cinfo.output_height;
    out = malloc(*out_lenp);
    if (!out) {
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }

    scanline = (*cinfo.mem->alloc_sarray)
        ((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    out_ptr = out;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, scanline, 1);
        memcpy(out_ptr, scanline[0], row_stride);
        out_ptr += row_stride;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    *outp = out;
    return 0;
}
```

## Important API Details

- `jpeg_mem_src()` requires libjpeg v8 or later, or libjpeg-turbo with
  `MEM_SRCDST_SUPPORTED`. It treats an empty input buffer as a fatal error.
- `jpeg_read_header()` fills `cinfo.image_width`, `cinfo.image_height`, and
  `cinfo.num_components` (the JPEG's native color space).
- `cinfo.output_width`, `cinfo.output_height`, and `cinfo.output_components`
  are computed after `jpeg_start_decompress()` and reflect any requested color
  conversion.
- `JSAMPLE` is `unsigned char` for 8-bit builds. For 12-bit builds the library
  is compiled separately and uses `J12SAMPLE`; NEP DICOM Sprint 2 targets 8-bit
  JPEG Baseline only.
- Use `cinfo.mem->alloc_sarray()` for the one-scanline work buffer so the
  library manages its lifetime. The returned buffer must *not* be freed by
  the caller.
- Always validate `output_width`, `output_height`, and `output_components`
  against expected values before copying into the output buffer.

## Color Space Notes

| Input colorspace | `out_color_space` | Output channels |
|------------------|-------------------|-----------------|
| Grayscale        | `JCS_GRAYSCALE`     | 1               |
| YCbCr / RGB      | `JCS_RGB`         | 3 (interleaved) |
| CMYK             | `JCS_CMYK`        | 4               |

For DICOM, `PhotometricInterpretation` of `MONOCHROME1` or `MONOCHROME2`
corresponds to `JCS_GRAYSCALE`. RGB images are normalized to `JCS_RGB` for the
NetCDF view.

## References

- libjpeg-turbo documentation: https://libjpeg-turbo.org/
- IJG libjpeg `libjpeg.txt`: `https://github.com/libjpeg-turbo/libjpeg-turbo/blob/main/doc/libjpeg.txt`
- `example.c` in the libjpeg-turbo source tree shows file-based decompression
  with custom error handling.
