# PDBx/mmCIF Format Reader

PDBx/mmCIF is the STAR/CIF-based macromolecular structure format that succeeded legacy PDB as the RCSB Protein Data Bank's primary distribution format. NEP's reader is read-only and covers atomic coordinate, unit-cell, and descriptive metadata categories.

**Transparent Access**: Read PDBx/mmCIF files with `nc_open()` after calling `NC_MMCIF_initialize()`.

**Category-to-NetCDF Mapping**:
- `_atom_site` (looped) → a single `atom` dimension (file order) and per-atom variables: `atom_site_Cartn_x/y/z` (`NC_DOUBLE`), `atom_site_id`, `atom_site_label_atom_id`, `atom_site_label_comp_id`, `atom_site_auth_asym_id`, `atom_site_auth_seq_id`, `atom_site_occupancy`, `atom_site_B_iso_or_equiv`, `atom_site_type_symbol`, and `atom_site_group_PDB` (`"ATOM"` or `"HETATM"`).
- `_atom_site.pdbx_PDB_model_num` distinct values → a `model` dimension (1 if the item is absent); coordinate variables are shaped `[model][atom]`.
- `_cell`/`_symmetry` → global attributes `cell_length_a/b/c`, `cell_angle_alpha/beta/gamma`, `symmetry_space_group_name_H-M`, `symmetry_Int_Tables_number`. Omitted entirely if the file has no `_cell`/`_symmetry` category.
- `_entry`/`_struct` → global attributes `entry_id`, `struct_title`.
- `?` (unknown) and `.` (not applicable) placeholder values both map to the destination variable's NetCDF fill value; no separate mask/flag variable is added.

**Known Limitations**: `_entity_poly_seq`-derived sequence data, `_atom_site_anisotrop` (ANISOU-equivalent) data, and `_struct_conn`/`_struct_conf`/`_struct_sheet_range` (bonds/secondary structure) are not currently handled. Multi-model (NMR ensemble) mmCIF is untested since all current test files are single-model X-ray structures.

**Use Cases**: Protein/macromolecular structure analysis, RCSB-sourced X-ray crystal structures.

**Enabling:**
```bash
cmake -B build -DNEP_ENABLE_MMCIF=ON   # CMake (default OFF)
```
**Dependencies**: None — a custom STAR/CIF tokenizer; no external parsing library is required.

**Resources**: [PDBx/mmCIF Dictionary](https://mmcif.wwpdb.org/) · [RCSB Protein Data Bank](https://www.rcsb.org/)

**Example:**
```c
#include "mmcifdispatch.h"
NC_MMCIF_initialize();   /* register UDF8; safe to call even if already registered */
nc_open("structure.cif", NC_UDF8, &ncid);
nc_inq_varid(ncid, "atom_site_Cartn_x", &varid);
nc_get_vara_double(ncid, varid, start, count, coords);
nc_close(ncid);
```

## Visualization

NEP includes Python visualization examples in `examples/viz/` that open PDBx/mmCIF files through the NetCDF UDF interface and write publication-ready PNGs.

- `plot_mmcif_1j7w.py` — 3D scatter of ATOM/HETATM Cartesian coordinates from `test/data/mmCIF/1J7W.cif` (deoxy haemoglobin beta-Y-Q mutant).
- `plot_mmcif_2w6v.py` — 3D scatter of ATOM/HETATM Cartesian coordinates from `test/data/mmCIF/2W6V.cif` (deoxy haemoglobin-xenon complex).
- `plot_mmcif_4hhb.py` — 3D scatter of ATOM/HETATM Cartesian coordinates from `test/data/mmCIF/4HHB.cif` (human deoxyhaemoglobin).

Enable the examples with:

```bash
cmake -S . -B build -DNEP_BUILD_EXAMPLES=ON -DNEP_ENABLE_VIZ_EXAMPLES=ON -DNEP_ENABLE_MMCIF=ON
```

Run only the mmCIF visualizations with `ctest --test-dir build -R viz_mmcif --output-on-failure`. Generated artifacts are `mmcif_1j7w_structure.png`, `mmcif_2w6v_structure.png`, and `mmcif_4hhb_structure.png` (each with a companion `_metadata.txt`) in the visualization build directory.
