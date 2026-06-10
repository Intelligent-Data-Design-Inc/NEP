---
name: concordance
description: Create a LibreOffice Writer concordance file (.sdi, semicolon-delimited) for use with the AutoMark feature to automatically generate a book index. Use when given text, a chapter, or a list of terms that need to be indexed.
metadata:
  author: cascade
  version: "1.0"
  date: "2026-04-17"
---

# Concordance File Skill

A concordance file is a semicolon-delimited text file used by LibreOffice Writer's **Insert → Table of Contents and Index → AutoMark** feature to automatically find and mark index entries throughout a document.

## File Format

Each line is one index entry with **six semicolon-separated fields**:

```
Search term;Alternative entry;1st key;2nd key;Case sensitive;Word only
```

### Field Reference

| Field | Required | Description |
|---|---|---|
| Search term | Yes | Exact text to find in the document |
| Alternative entry | No | Text shown in the index (uses search term if empty) |
| 1st key | No | Primary category / top-level index heading |
| 2nd key | No | Secondary category for nested sub-entries |
| Case sensitive | No | `1` = match case, `0` or empty = ignore case |
| Word only | No | `1` = whole word only, `0` or empty = match substrings |

### Example Lines

```
NetCDF;NetCDF;File Formats;;0;1
HDF5;HDF5;File Formats;;0;1
dispatch table;dispatch table;Architecture;;0;1
nc_open;;API functions;;0;1
```

## How to Generate a Concordance CSV

Given source text (a chapter, section, or term list), follow these steps:

### Step 1 — Identify Index-Worthy Terms

Extract terms that are:
- **Proper nouns**: product names, library names, standards (e.g., NetCDF, HDF5, Zarr)
- **Technical terms**: functions, data structures, algorithms, protocols
- **Concepts**: architecture patterns, design principles, key ideas
- **Acronyms**: with and without expansion (index both forms)
- **People and organizations**: authors, standards bodies

Avoid indexing:
- Common words (the, and, is, for)
- Terms appearing only once and not significant
- Overly generic terms that would produce hundreds of entries

### Step 2 — Assign Categories (1st key)

Group related terms under a shared 1st key. Typical categories for technical books:

- `API functions` — function/method names like `nc_open`, `nc_close`
- `Data structures` — structs, classes, types
- `File formats` — NetCDF, HDF5, CSV, GeoTIFF
- `Concepts` — chunking, compression, dispatch
- `Tools` — command-line tools, utilities
- `Standards` — CF Conventions, OGC, ISO

For non-technical books, categories reflect subject matter:
- `People`, `Places`, `Events`, `Terms`, `Organizations`

### Step 3 — Decide Case and Word Matching

- Use `Word only=1` for most technical terms to avoid false matches (e.g., "var" matching "variable")
- Use `Case sensitive=1` for proper nouns, acronyms, and function names
- Use `Case sensitive=0` for concepts that may appear mid-sentence in lowercase

### Step 4 — Write the CSV

Output as plain text, one entry per line, no header row, semicolons as delimiters:

```
Search term;Alternative entry;1st key;2nd key;Case sensitive;Word only
```

Save with `.sdi` extension in UTF-8 or ANSI encoding.

## Complete Example

Given text about NetCDF file I/O, a concordance might look like:

```
NetCDF;NetCDF;File formats;;0;1
HDF5;HDF5;File formats;;0;1
Zarr;Zarr;File formats;;0;1
nc_open;nc_open();API functions;;1;1
nc_close;nc_close();API functions;;1;1
nc_get_vara;nc_get_vara();API functions;;1;1
dispatch table;dispatch table;Architecture;;0;1
NC_Dispatch;NC_Dispatch;Data structures;;1;1
chunking;chunking;Concepts;;0;1
compression;compression;Concepts;;0;1
OPeNDAP;OPeNDAP;Protocols;;1;1
DAP2;DAP2;Protocols;;1;1
DAP4;DAP4;Protocols;;1;1
```

## Output Format Rules

1. **No header row** in the output file — LibreOffice expects data on every line
2. **Semicolons** as delimiters (not commas)
3. **No quotes** around fields unless the field contains a semicolon
4. **One entry per line**
5. **Sort alphabetically** by search term for readability (optional but helpful)
6. **UTF-8 encoding** for documents with non-ASCII characters

## Tips

- A term can appear multiple times under different 1st keys if it belongs to multiple categories
- The Alternative entry can be used to normalize capitalization in the index (e.g., search `hdf5`, display `HDF5`)
- For acronyms, add two entries: one for the acronym, one for the expanded form, both pointing to the same 1st key
- Keep 2nd key sparse — only use it when a category genuinely has sub-groupings (e.g., 1st key `API functions`, 2nd key `read` vs `write`)
