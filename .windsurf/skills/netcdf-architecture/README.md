# NetCDF-C Architecture Skill

This Windsurf skill provides comprehensive knowledge of the NetCDF-C library architecture.

## What This Skill Provides

- **Dispatch table architecture** understanding
- **Format implementations** (NetCDF-3, HDF5, Zarr, DAP2, DAP4)
- **Data structures** used throughout the codebase
- **I/O layers** and storage backends
- **Component relationships** and data flow

## Files

- **SKILL.md** - Main skill file with architecture overview and quick reference
- **references/COMPONENTS.md** - Detailed component descriptions for each library
- **references/DATA-STRUCTURES.md** - Complete data structure documentation
- **references/DISPATCH-TABLES.md** - All dispatch table implementations

## When to Use

Use this skill when:
- Adding new features to NetCDF-C
- Debugging format-specific issues
- Understanding data flow through the library
- Implementing new dispatch tables or storage backends
- Working with metadata structures
- Investigating performance issues

## Skill Compliance

This skill follows the Windsurf Agent Skills specification:

- ✅ Valid name: `netcdf-architecture` (lowercase, hyphens only)
- ✅ Description: Comprehensive, includes keywords and use cases
- ✅ SKILL.md: 371 lines (under 500 line recommendation)
- ✅ Progressive disclosure: Main content in SKILL.md, details in references/
- ✅ Reference files: One level deep, focused topics
- ✅ Metadata: Author, version, date included

## Installation

To use this skill in Windsurf:

1. Copy the `netcdf-architecture/` directory to your Windsurf skills location
2. The skill will be automatically detected and loaded
3. AI assistants can now access NetCDF-C architecture knowledge

## Maintenance

Update this skill when:
- New dispatch tables are added
- Major architectural changes occur
- New storage backends are implemented
- Data structures are significantly modified

## Version

Current version: 1.0 (January 14, 2026)
