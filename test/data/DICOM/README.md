# DICOM Test Data

This directory contains sample DICOM files used by the NEP DICOM UDF reader tests and visualization examples.

## Existing NEP Samples

- `tst_dicom_uncompressed.dcm` — tiny synthetic 4×6 8-bit grayscale image.
- `MRBRAIN.DCM` — 512×512 16-bit single-frame MR image.
- `0003.DCM` — 512×512 17-frame encapsulated JPEG Baseline XA image.

## Public-Domain Samples

The following additional samples are from the Open Microscopy Environment
(OME) DICOM sample collection, maintained by the OME team as freely usable test
images. They are in the public domain and are free of patient identifiers.

Transfer syntax values below reflect the actual embedded Transfer Syntax UID
of each file, verified directly via libdicom (v3.2.0 Sprint 2); some were
mislabeled when these samples were first added.

| File | Modality | Transfer Syntax | Size | Description | NEP Support |
|------|----------|-----------------|------|-------------|-------------|
| `CT-MONO2-16-brain.dcm` | CT | Explicit VR Little Endian | 525968 bytes | Brain CT slice. | Supported (native) |
| `MR-MONO2-16-head.dcm` | MR | Implicit VR Little Endian | 132876 bytes | Head MR slice. | Metadata supported; pixel-data read fails due to a libdicom limitation with this file's missing `NumberOfFrames` tag (unrelated to NEP) |
| `CT-MONO2-16-chest.dcm` | CT | JPEG Lossless, First-Order Prediction (`1.2.840.10008.1.2.4.70`) | 145136 bytes | Chest CT slice. | Supported (requires `libgdcm-dev`) |
| `MR-MONO2-12-shoulder.dcm` | MR | JPEG Lossless (`1.2.840.10008.1.2.4.57`) | 720528 bytes | Shoulder MR slice, 12-bit stored. | Supported (requires `libgdcm-dev`) |
| `CR-MONO1-10-chest.dcm` | CR | Unknown; file has no 128-byte preamble/`DICM` magic | 387976 bytes | Chest X-ray. | **Not supported** — rejected cleanly with `NC_EINVAL`; out of scope, see `docs/plan/v3.2.0-sprint2-dicom-sample-coverage.md` |

Source: <https://downloads.openmicroscopy.org/images/DICOM/samples/>

These files are de-identified, public-domain sample images provided for
software testing and development. They are not clinical datasets and contain no
real patient information.
