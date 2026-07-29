# PDB Test Data

This directory contains legacy PDB-format files used by `test/tst_pdb_udf.c`
and `ftest/ftst_pdb_udf.F90`.

## Files

### `1J7W.pdb`
- Source: RCSB Protein Data Bank (https://www.rcsb.org/structure/1J7W)
- License: Public domain
- Type: X-ray crystal structure, single model

### `4HHB.pdb`
- Source: RCSB Protein Data Bank (https://www.rcsb.org/structure/4HHB)
- License: Public domain
- Type: X-ray crystal structure, single model

### `1GAB.pdb`
- Source: RCSB Protein Data Bank (https://www.rcsb.org/structure/1GAB)
- License: Public domain
- Type: NMR ensemble, 20 `MODEL`/`ENDMDL` blocks
- Notes: Used to verify multi-model coordinate reads in the NEP PDB UDF
  reader. Downloaded via `https://files.rcsb.org/download/1GAB.pdb`.

### `no_cryst1.pdb`
- Source: Generated for NEP testing
- License: N/A (synthetic test fixture)
- Type: Single-model synthetic structure, 5 alanine atoms, no `CRYST1`
- Notes: Used to verify that the PDB reader opens files without a `CRYST1`
  record and omits the optional `cell_*` and `space_group_name_H-M`
  global attributes.
