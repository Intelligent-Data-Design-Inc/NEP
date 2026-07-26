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

| File | Modality | Transfer Syntax | Size | Description |
|------|----------|-----------------|------|-------------|
| `CT-MONO2-16-chest.dcm` | CT | Explicit VR Little Endian | 145136 bytes | Chest CT slice. |
| `CT-MONO2-16-brain.dcm` | CT | Explicit VR Little Endian | 525968 bytes | Brain CT slice. |
| `MR-MONO2-16-head.dcm` | MR | Explicit VR Little Endian | 132876 bytes | Head MR slice. |
| `CR-MONO1-10-chest.dcm` | CR | Implicit VR Little Endian | 387976 bytes | Chest X-ray. |
| `MR-MONO2-12-shoulder.dcm` | MR | Explicit VR Little Endian | 720528 bytes | Shoulder MR slice. |

Source: <https://downloads.openmicroscopy.org/images/DICOM/samples/>

These files are de-identified, public-domain sample images provided for
software testing and development. They are not clinical datasets and contain no
real patient information.
