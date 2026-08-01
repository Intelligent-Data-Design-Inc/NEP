# Legacy PDB Format Reader

Legacy PDB (Protein Data Bank) is the fixed-column text format historically used by the RCSB Protein Data Bank for macromolecular structures (proteins, nucleic acids). NEP's reader is read-only and covers coordinate, model, and unit-cell metadata records.

**Transparent Access**: Read legacy PDB files with `nc_open()` after calling `NC_PDB_initialize()`.

**Record-to-NetCDF Mapping**:
- `ATOM`/`HETATM` records → a single `atom` dimension (file order) and per-atom variables: `atom_site_Cartn_x/y/z`, `atom_site_id`, `atom_site_label_atom_id`, `atom_site_label_comp_id`, `atom_site_auth_asym_id`, `atom_site_auth_seq_id`, `atom_site_occupancy`, `atom_site_B_iso_or_equiv`, `atom_site_type_symbol`, and `atom_site_group_PDB` (`"ATOM"` or `"HETATM"`).
- `MODEL`/`ENDMDL` blocks → a `model` dimension (1 if no `MODEL` records are present); coordinate variables are shaped `[model][atom]`.
- `CRYST1` → global attributes `cell_length_a/b/c`, `cell_angle_alpha/beta/gamma`, `space_group_name_H-M`, `symmetry_Z`. Omitted entirely if the file has no `CRYST1` record.
- `HEADER`/`TITLE`/`COMPND`/`SOURCE` → global attributes `idCode`, `classification`, `depDate`, `title`, `compnd`, `source`.

**Known Limitations**: `SEQRES` sequence data, hybrid-36 encoded serial/residue numbers, and multi-character chain IDs are not currently handled.

**Use Cases**: Protein/macromolecular structure analysis, RCSB-sourced X-ray crystal structures, NMR ensembles with multiple models.

**Enabling:**
```bash
cmake -B build -DNEP_ENABLE_PDB=ON   # CMake (default ON)
```
**Dependencies**: None — a custom fixed-column line reader; no external parsing library is required.

**Resources**: [PDB Format Guide](https://www.wwpdb.org/documentation/file-format) · [RCSB Protein Data Bank](https://www.rcsb.org/)

**Example:**
```c
#include "pdbdispatch.h"
NC_PDB_initialize();   /* register UDF7; safe to call even if already registered */
nc_open("structure.pdb", NC_UDF7, &ncid);
nc_inq_varid(ncid, "atom_site_Cartn_x", &varid);
nc_get_vara_float(ncid, varid, start, count, coords);
nc_close(ncid);
```
