---
name: mmcif
description: Understanding the PDBx/mmCIF macromolecular structure file format (categories, items, loops, the underlying CIF/STAR syntax) and how to map its contents to netCDF dimensions, variables, and attributes for a read-only NEP UDF handler. Use together with the pdb-legacy skill when the legacy PDB format is also relevant.
metadata:
  author: netcdf-analysis
  version: "1.0"
  date: "2026-07-29"
---

# PDBx/mmCIF Skill

This skill covers the **PDBx/mmCIF** macromolecular structure format and
the design of a read-only mmCIF reader plugin for NEP that exposes atomic
coordinates and structural metadata through the NetCDF API.

## Overview

**mmCIF** (macromolecular Crystallographic Information File) is a
dictionary-based extension of the **CIF** (Crystallographic Information
File) syntax, itself built on the generic **STAR** (Self-defining Text
Archive and Retrieval) grammar. **PDBx/mmCIF** is the specific dictionary
(`mmcif_pdbx.dic`, maintained by wwPDB) used as the master archive format
for the Protein Data Bank since 2014, superseding the legacy fixed-column
PDB format (see the `pdb-legacy` skill).

Primary references:
- wwPDB mmCIF resources: https://mmcif.wwpdb.org/
- MMCIF User Guide: https://mmcif.wwpdb.org/docs/user-guide/guide.html
- PDBx/mmCIF Dictionary Resources (RCSB): https://mmcif.rcsb.org/docs/tutorials/mechanics/pdbx-mmcif-dict-struct.html
- Current dictionary index: https://mmcif.wwpdb.org/dictionaries/mmcif_pdbx_v50.dic/Index/index.html

For NEP, mmCIF is read-only. The reader opens an mmCIF text file, parses
its data block(s) into categories/items, and presents atomic coordinates
and metadata as NetCDF variables, dimensions, and attributes.

## File Syntax (STAR/CIF Grammar)

An mmCIF **data file** is plain ASCII/UTF-8 text consisting of one or more
**data blocks**:

```
data_1ABC
_entry.id   1ABC
#
_cell.length_a   58.39
_cell.length_b   86.70
...
#
loop_
_atom_site.group_PDB
_atom_site.id
_atom_site.type_symbol
_atom_site.label_atom_id
...
ATOM   1  N   ...
ATOM   2  CA  ...
#
```

Key syntax rules:

- A data block starts with the token `data_<name>` and runs to the next
  `data_` token or end of file. Data blocks cannot be nested.
- Comments start with `#` and run to end of line.
- **Key-value pairs**: `_category.item  value` — a single value on the
  same line (or a multi-line quoted/text-field value).
- **Loops** (`loop_`): declare a list of item names, one per line, all
  from the *same category*, followed by whitespace-delimited rows of
  values — one row per repetition of that category (i.e. a table).
  Loops are how `_atom_site`, `_entity`, `_struct_conn`, etc. store many
  rows efficiently.
- **Quoting**: values with embedded whitespace are single- or
  double-quoted (`'...'`, `"..."`). Multi-line text values are delimited
  by a semicolon at the start of a line (`;...;`), used for free text like
  `_struct.title` or `_entity.pdbx_description`.
- **Special value placeholders**:
  - `?` — value is missing/unknown.
  - `.` — no value is applicable / intentionally omitted.
- No nested loops are permitted; all items in one `loop_` belong to one
  category and have the same number of rows.
- **Save frames** (`save_...` / `save_`) appear only in *dictionary* files
  (defining categories/items), never in *data* files. A NEP data reader
  does not need to handle save frames.

## Data Model: Categories and Items

- A **category** is a named table (e.g. `atom_site`, `entity`, `cell`).
- An **item** (data name) is a column within a category, written as
  `_category_name.item_name` (category and item separated by `.`).
- Within a category, a subset of items are designated **key items**; no
  two rows may have duplicate values for the full set of key items
  (usually enforcing category-level uniqueness, e.g. `_atom_site.id`).
- **Parent-child relationships** link items across categories (foreign
  keys), e.g. `_atom_site.label_asym_id` refers to `_struct_asym.id`.
  These relationships express the relational structure of a macromolecule
  (entities, chains/instances, residues, atoms) without requiring nested
  data structures.
- The **DDL2** dictionary definition language defines the categories and
  items themselves; NEP's reader only needs to consume *data* files
  conforming to the dictionary, not parse the dictionary itself.

## Key Categories for a Structure Reader

| Category | Legacy PDB equivalent | Purpose |
|---|---|---|
| `_entry` | `HEADER` (idCode) | Top-level entry identifier (`_entry.id`) |
| `_struct` | `HEADER`/`TITLE` | `_struct.title`, classification text |
| `_entity` | `COMPND` | One row per unique molecular entity (polymer, non-polymer, water) |
| `_entity_poly` | `COMPND`/`SEQRES` | Polymer type, sequence as one-letter code |
| `_entity_poly_seq` | `SEQRES` | Monomer sequence for each polymer entity (`entity_id`, `num`, `mon_id`) |
| `_entity_src_nat` / `_entity_src_gen` / `_pdbx_entity_src_syn` | `SOURCE` | Organism / source details |
| `_struct_asym` | (chain, implicit) | One row per distinct molecule instance (`id`, `entity_id`) |
| `_pdbx_poly_seq_scheme` | (chain/resSeq mapping) | Maps label numbering to author/PDB numbering |
| `_cell` | `CRYST1` (a,b,c,angles) | Unit cell parameters |
| `_symmetry` | `CRYST1` (sGroup) | Space group name / cell setting |
| `_atom_site` | `ATOM`/`HETATM` | Atomic coordinates (the main coordinate table) |
| `_atom_site_anisotrop` | `ANISOU` | Anisotropic displacement parameters |
| `_struct_conn` | `LINK`/`SSBOND` | Explicit bonds/connections between atoms |
| `_struct_conf` | `HELIX` | Secondary structure (helix) ranges |
| `_struct_sheet_range` | `SHEET` | Secondary structure (sheet) ranges |
| `_pdbx_struct_assembly` / `_pdbx_struct_oper_list` | `REMARK 350`/`MTRIXn` | Biological assembly generation operators |
| `_chem_comp` | `HETNAM`/`FORMUL` | Chemical component (residue/ligand) definitions |

## `_atom_site` — The Coordinate Category

This is the primary table for a NEP reader. Typical item set (order not
significant; items are matched by name in the `loop_` header):

```
loop_
_atom_site.group_PDB          # "ATOM" or "HETATM" (place-holder for legacy PDB tag)
_atom_site.id                 # unique integer atom identifier
_atom_site.type_symbol        # element symbol
_atom_site.label_atom_id      # atom name within its chemical component
_atom_site.label_alt_id       # alternate conformation identifier ('.' if none)
_atom_site.label_comp_id      # chemical component (residue) 3-letter code
_atom_site.label_asym_id      # internal chain/instance id -> _struct_asym.id
_atom_site.label_entity_id    # -> _entity.id
_atom_site.label_seq_id       # -> _entity_poly_seq.num (polymer position); '.' for non-polymers
_atom_site.pdbx_PDB_ins_code  # PDB insertion code
_atom_site.Cartn_x            # X coordinate (Å)
_atom_site.Cartn_y            # Y coordinate (Å)
_atom_site.Cartn_z            # Z coordinate (Å)
_atom_site.occupancy          # fractional occupancy
_atom_site.B_iso_or_equiv      # isotropic (or equivalent) B-factor
_atom_site.pdbx_formal_charge  # integer formal charge
_atom_site.auth_seq_id         # PDB-author residue number (legacy resSeq)
_atom_site.auth_comp_id        # PDB-author residue name (usually == label_comp_id)
_atom_site.auth_asym_id        # PDB-author chain id (legacy chainID)
_atom_site.auth_atom_id        # PDB-author atom name (usually == label_atom_id)
_atom_site.pdbx_PDB_model_num  # model number, for NMR ensembles / multi-model entries
```

Important distinction: `label_*` items are the internal, always-present,
uniquely-sequential identifiers used by the mmCIF data model itself;
`auth_*` items are the (sometimes irregular, sometimes reused) identifiers
chosen by the depositing authors and historically exposed in legacy PDB
files. **wwPDB recommends using `auth_seq_id`, `auth_comp_id`, and
`auth_asym_id` for anything intended to match published/legacy chain and
residue numbering.**

## `_cell` and `_symmetry` — Crystallographic Parameters

```
_cell.entry_id        1ABC
_cell.length_a        58.39
_cell.length_b        86.70
_cell.length_c        46.27
_cell.angle_alpha     90.00
_cell.angle_beta      90.00
_cell.angle_gamma     90.00
_cell.volume          234237
#
_symmetry.entry_id              1ABC
_symmetry.cell_setting          orthorhombic
_symmetry.Int_Tables_number     18
_symmetry.space_group_name_H-M  'P 21 21 2'
```

These are single-row (key-value) categories, not loops, in most PDB
entries — one row per entry.

## Mapping PDBx/mmCIF to the netCDF-4 Model

| mmCIF concept | netCDF-4 mapping |
|---|---|
| Data block (`data_<name>`) | Root group (name from block name, or ignored if only one block) |
| Single-row category (e.g. `_cell`, `_symmetry`, `_entry`, `_struct`) | Global attributes, prefixed by category (e.g. `cell_length_a`, `symmetry_space_group_name_H-M`) |
| `_atom_site` (looped category) | `atom` dimension (length = row count); one variable per item, each `[atom]`-shaped, or `[model][atom]` if `pdbx_PDB_model_num` has multiple distinct values |
| `_atom_site.Cartn_x/y/z` | Either three separate `[atom]` `NC_FLOAT`/`NC_DOUBLE` variables, or one combined `[atom][3]` variable `atom_site_Cartn` with a `cartesian_axis` dimension of length 3 |
| `_entity_poly_seq` (looped) | `residue` dimension per entity; `mon_id` string variable `[residue]` |
| `_struct_asym` (looped) | `chain` dimension; `id`/`entity_id` string/int variables `[chain]` |
| Text item value | `NC_STRING` (netCDF-4) or fixed-length `NC_CHAR` array variable |
| Numeric item value (real/integer regex-typed in the dictionary) | `NC_DOUBLE`/`NC_INT` per the dictionary's declared type, see below |
| `?` value | Fill value for the variable's type, or a per-row mask if precise missing-value tracking is required |
| `.` value | Not-applicable; treat the same as fill value unless the reader distinguishes the two via a companion mask variable |

### Determining netCDF Type from mmCIF Item Type

The PDBx/mmCIF dictionary declares an `_item_type.code` for every item
(e.g. `int`, `float`, `code`, `line`, `text`, `yyyy-mm-dd`). A pragmatic
reader does not need the full dictionary; the following simplified rules
work for the categories above:

| mmCIF value pattern | `nc_type` |
|---|---|
| Integer (`_atom_site.id`, `auth_seq_id`, `pdbx_formal_charge`) | `NC_INT` |
| Real/float (`Cartn_x/y/z`, `occupancy`, `B_iso_or_equiv`, `_cell.length_*`, `_cell.angle_*`) | `NC_DOUBLE` |
| Single character/code (`label_alt_id`, `pdbx_PDB_ins_code`, `type_symbol`) | `NC_CHAR` (fixed width) or `NC_STRING` |
| Free text (`_struct.title`, `_entity.pdbx_description`) | `NC_STRING` (netCDF-4) or a variable-length global attribute |

### Suggested Dimension Set

- `atom` — number of rows in `_atom_site` for a single model.
- `model` — number of distinct `_atom_site.pdbx_PDB_model_num` values
  (1 for a typical X-ray structure; >1 for NMR ensembles).
- `chain` — number of distinct `_struct_asym.id` (or `auth_asym_id`) values.
- `residue` (per entity, or a flattened global `residue` dimension) — rows
  in `_entity_poly_seq`.
- `cartesian_axis` — fixed length 3, if coordinates are combined into one
  variable.

### Implementation Notes

- Parse the file into an in-memory table per category first (a
  category-name -> list-of-item-names -> list-of-row-values structure),
  then apply the mapping table above. This mirrors how the PDS4 reader in
  NEP separates "parse label" from "build netCDF metadata" (see the
  `pds4` skill for a structurally similar two-phase approach, XML labels
  in that case).
- Because `_atom_site` rows for a multi-model file repeat the same atoms
  once per model (differing only by `pdbx_PDB_model_num` and
  coordinates), detect models by grouping rows by that item rather than
  assuming one row per atom.
- `label_asym_id` values are always assigned starting from `A` and can
  differ from `auth_asym_id` (the legacy/publication chain ID) — expose
  both as separate variables rather than merging them.
- Case sensitivity: mmCIF category and item names are case-insensitive
  per the CIF grammar, but the convention (and NEP's reader) should treat
  them case-insensitively on input while emitting normalized lower-case
  netCDF names.

## Magic Number / Detection

mmCIF data files are plain text; they conventionally begin with the
`data_` token (e.g. `data_1ABC`) as the first non-comment,
non-whitespace token. Unlike PDS4's `<?xml` this is not a byte-exact
magic number shared with unrelated formats, but detection should:

1. Skip leading blank lines and `#`-comment lines.
2. Confirm the first token starts with `data_`.
3. Optionally confirm at least one expected PDBx category name
   (`_atom_site.` or `_entry.`) appears within the first several KB, to
   avoid false positives against generic (non-PDBx) CIF files such as
   small-molecule crystallography CIFs from the core `cif_core.dic`
   dictionary.

```c
/** PDBx/mmCIF magic: "data_" token at the start of the file (after
 *  optional blank/comment lines) */
#define NEP_MAGIC_MMCIF "data_"

/** PDBx/mmCIF format uses UDF8 slot */
#define NEP_UDF_MMCIF NC_UDF8

#define NEP_FORMAT_NAME_MMCIF "mmCIF"
```

## NEP-Specific Guidance

### UDF Slot

Based on current NEP allocations (see `docs/formats.md`):

| Slot | Format |
|------|--------|
| UDF0 | GeoTIFF BigTIFF |
| UDF1 | GeoTIFF standard TIFF |
| UDF2 | GRIB2 |
| UDF3 | FITS |
| UDF4 | NASA CDF |
| UDF5 | NASA/ESA PDS4 |
| UDF6 | DICOM |
| UDF7 | Legacy PDB (see `pdb-legacy` skill) |
| **UDF8** | **PDBx/mmCIF** |
| UDF9 | Reserved |

### Initialization

Follow the PDS4/FITS dispatch pattern used elsewhere in NEP
(`src/pds4dispatch.c`, `src/fitsdispatch.c`):

```c
static const NC_Dispatch MMCIF_dispatcher = {
    NC_FORMATX_UDF8,
    NC_DISPATCH_VERSION,
    NC_RO_create,
    NC_MMCIF_open,
    /* ... remaining read-only dispatch functions ... */
};

NC_Dispatch *
NC_MMCIF_initialize(void)
{
    MMCIF_dispatch_table = &MMCIF_dispatcher;
    nc_def_user_format(NEP_UDF_MMCIF,
                        (NC_Dispatch *)MMCIF_dispatch_table,
                        NEP_MAGIC_MMCIF);
    return (NC_Dispatch *)&MMCIF_dispatcher;
}
```

### Build Options

| Build system | Option |
|--------------|--------|
| CMake | `-DNEP_ENABLE_MMCIF=ON/OFF` (default OFF) |
| Autotools | `--enable-mmcif` / `--disable-mmcif` |

### Parsing Library Options

1. **Custom STAR/CIF tokenizer** (recommended for a minimal read-only
   reader): the grammar needed to read PDBx/mmCIF *data* files (as
   opposed to *dictionary* files with save frames) is small — key-value
   pairs, `loop_` blocks, quoted/semicolon text values, and `#` comments.
   This avoids a new external dependency, matching NEP's PDS4 approach of
   using a general-purpose parser (there, libxml2) plus custom
   category-to-schema mapping logic.
2. **CIFParse-obj / gemmi**: full-featured C++ mmCIF libraries used by
   wwPDB tooling and gemmi respectively; heavier dependencies, likely
   unnecessary for a read-only coordinate/metadata extraction reader.
   - gemmi: https://github.com/project-gemmi/gemmi (C++/Python, permissive
     license, actively maintained, has a documented mmCIF parser).
   - CIFParse-obj: https://github.com/rcsb/py-rcsb_db or the wwPDB C++
     `cifparse-obj`/`cifpp` tooling — closer to the canonical
     implementation but a larger dependency footprint.

### Tests

- Use a small, freely redistributable mmCIF entry for testing (e.g. the
  same structure used for the `pdb-legacy` skill's test, in `.cif` form —
  RCSB provides both formats for most entries, e.g. `1CRN.cif`).
- Test cases:
  - Open an mmCIF file through `nc_open`.
  - Verify `atom` dimension length matches the row count of `_atom_site`.
  - Read `atom_site_Cartn_x/y/z` for a known atom and compare to the
    corresponding loop row values in the source file.
  - Verify global attributes derived from `_cell`/`_symmetry` match the
    file's single-row values.
  - Verify multi-model detection using an NMR ensemble entry.
  - Verify a small-molecule (non-PDBx) CIF file is correctly rejected
    (`NC_ENOTNC`) by the PDBx-specific category check.

## Important Caveats

1. **PDBx/mmCIF vs. generic CIF**: a "magic" check on `data_` alone
   matches any CIF file, including small-molecule/core CIF files that use
   an entirely different item vocabulary. Confirm PDBx-specific categories
   are present before accepting the file.
2. **`label_*` vs `auth_*` identifiers**: do not conflate internal
   sequential identifiers with author/legacy-PDB identifiers; expose both
   where present since downstream consumers may need either.
3. **`?` vs `.` placeholders**: both indicate "no value" but have
   different semantics (unknown vs. not applicable). A reader that maps
   both to the same fill value loses this distinction; document the
   simplification if adopted.
4. **Multi-line text values**: `_struct.title` and similar free-text items
   may span many lines inside a `;...;` block; the tokenizer must treat
   the entire block as one value, not split it into loop rows.
5. **No nested loops**: a mmCIF file cannot loop items from two different
   categories together; each `loop_` block is single-category. A reader
   that tries to merge multiple loop blocks by row position (rather than
   by shared key items) will misalign data if categories have differing
   row counts.
6. **Large entries**: some PDB entries (large viral capsids, ribosomes)
   have `_atom_site` tables with hundreds of thousands of rows; prefer a
   single-pass streaming tokenizer over loading the whole file into a DOM
   if memory is a concern.

## References

- wwPDB mmCIF resources: https://mmcif.wwpdb.org/
- MMCIF User Guide: https://mmcif.wwpdb.org/docs/user-guide/guide.html
- PDBx/mmCIF Dictionary Resources (RCSB tutorial): https://mmcif.rcsb.org/docs/tutorials/mechanics/pdbx-mmcif-dict-struct.html
- PDBx/mmCIF Glossary: https://mmcif.rcsb.org/docs/tutorials/glossary/pdbx-mmcif-glossary.html
- Data Items Describing Atomic Positions: https://mmcif.wwpdb.org/docs/tutorials/content/atomic-description.html
- `_atom_site` category reference: https://mmcif.wwpdb.org/dictionaries/mmcif_pdbx_v50.dic/Categories/atom_site.html
- `_cell` category reference: https://mmcif.wwpdb.org/dictionaries/mmcif_std.dic/Categories/cell.html
- `_symmetry` category reference: https://mmcif.wwpdb.org/dictionaries/mmcif_std.dic/Categories/symmetry.html
- gemmi (C++/Python mmCIF library): https://github.com/project-gemmi/gemmi

## When to Use This Skill

Use this skill when:
- Adding a PDBx/mmCIF UDF handler to NEP.
- Mapping mmCIF categories and items to NetCDF dimensions, variables, and
  attributes.
- Choosing between a custom STAR/CIF tokenizer, gemmi, or CIFParse-obj for
  the implementation.
- Designing tests and build-system options for the mmCIF reader.
- Debugging CIF/STAR syntax edge cases (loops, quoting, multi-line text,
  `?`/`.` placeholders) or label/auth identifier confusion.
- Deciding how the mmCIF reader relates to a legacy PDB reader (see the
  `pdb-legacy` skill) sharing the same target netCDF schema.
