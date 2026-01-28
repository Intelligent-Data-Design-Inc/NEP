# Issue #112 Refinement: Fortran Groups Example Implementation

## Executive Summary

Issue #112 requests creation of a Fortran equivalent (`f_groups.f90`) of the C groups example (`groups.c`) to demonstrate NetCDF-4 hierarchical group structures, nested groups, dimension visibility, and all five new integer types (ubyte, ushort, uint, int64, uint64) using the Fortran-90 NetCDF API. This is part of v1.5.2 Sprint 2 and builds on the C implementation from Sprint 1.

## Issue Quality Assessment

✅ **Issue is well-structured and complete:**
- Clear title and description
- Detailed task breakdown with checkboxes
- Implementation notes with code examples
- Definition of done criteria
- References to related work (Sprint 1 C example)
- Proper labels (enhancement, v1.5.2, sprint-2)

## Requirements & Acceptance Criteria

### Functional Requirements
- [ ] Create `examples/f_netcdf-4/f_groups.f90` with comprehensive Doxygen documentation
- [ ] Implement 3-level group hierarchy: Root → SubGroup1, Root → SubGroup2 → NestedGroup
- [ ] Define dimensions at root (x=3, y=4) and nested group (z=2)
- [ ] Create variables using all 5 new NetCDF-4 integer types:
  - Root: `ubyte_var` (nf90_ubyte, 2D: x, y)
  - SubGroup1: `ushort_var` (nf90_ushort, 2D: x, y)
  - SubGroup2: `uint_var` (nf90_uint, 2D: x, y)
  - NestedGroup: `int64_var` (nf90_int64, 2D: x, y)
  - NestedGroup: `uint64_var` (nf90_uint64, 3D: x, y, z)
- [ ] Write sequential test data matching C example (values 1-72)
- [ ] Validate file structure on read: groups, dimensions, variables, data
- [ ] Explicitly test dimension visibility across group boundaries

### Build System Requirements
- [ ] CMake integration: Add to `examples/f_netcdf-4/CMakeLists.txt`
- [ ] Autotools integration: Add to `examples/f_netcdf-4/Makefile.am`
- [ ] Conditional compilation when `ENABLE_FORTRAN=ON`
- [ ] Link against NetCDF-Fortran library
- [ ] Register as test in both build systems

### Testing Requirements
- [ ] Example runs without errors (exit code 0)
- [ ] Creates valid NetCDF-4 file with group structure
- [ ] All validation checks pass (groups, dimensions, variables, data)
- [ ] Works with both CMake and Autotools builds
- [ ] Does not build when Fortran disabled
- [ ] CI passes with new example

### Documentation Requirements
- [ ] Comprehensive Doxygen header matching C example style
- [ ] Learning objectives clearly stated
- [ ] Key concepts explained (group hierarchy, dimension visibility)
- [ ] Fortran API patterns documented
- [ ] Use cases for groups listed
- [ ] Prerequisites and related examples referenced

## Implementation Approach

The implementation will closely mirror the C `groups.c` example structure but use Fortran-90 NetCDF API conventions:

1. **File Structure**: Follow existing Fortran example patterns (`f_simple_nc4.f90`, `f_compression.f90`)
2. **API Mapping**: Use Fortran equivalents of C group functions:
   - `nc_def_grp()` → `nf90_def_grp()`
   - `nc_inq_grps()` → `nf90_inq_grps()`
   - `nc_inq_grp_ncid()` → `nf90_inq_grp_ncid()`
   - `nc_inq_grpname()` → `nf90_inq_grpname()`
   - `nc_inq_dimid()` → `nf90_inq_dimid()`
3. **Data Types**: Use Fortran integer kinds for new NetCDF-4 types:
   - `nf90_ubyte` → `integer(kind=1)` arrays
   - `nf90_ushort` → `integer(kind=2)` arrays
   - `nf90_uint` → `integer(kind=4)` arrays
   - `nf90_int64` → `integer(kind=8)` arrays
   - `nf90_uint64` → `integer(kind=8)` arrays
4. **Array Ordering**: Fortran column-major ordering (dimensions reversed from C)
5. **Error Handling**: Check all `nf90_*` return codes, use `nf90_strerror()`, stop with exit code 2 on errors
6. **Validation**: 5-phase structure matching C example (create, validate groups, test dimensions, validate metadata, validate data)

## Implementation Steps

### Step 1: Create f_groups.f90 (4-6 hours)
- Create file with comprehensive Doxygen documentation header
- Implement write phase:
  - File creation with `NF90_NETCDF4` flag
  - Group creation (SubGroup1, SubGroup2, NestedGroup)
  - Dimension definitions (x, y at root; z in NestedGroup)
  - Variable definitions using all 5 new integer types
  - Data initialization and writing (sequential values 1-72)
- Implement read/validation phase:
  - Group navigation and name validation
  - Dimension visibility testing across boundaries
  - Variable metadata validation
  - Data value verification
- Add summary output matching C example format

### Step 2: CMake Build Integration (1 hour)
- Edit `examples/f_netcdf-4/CMakeLists.txt`
- Add `f_groups` to `F_NETCDF4_EXAMPLES` list (line 34-40)
- Example will automatically build via foreach loop (lines 48-60)
- Add test registration (already handled by loop at line 58)
- Create expected CDL file: `examples/expected_output/f_groups_expected.cdl`
- Add CDL validation test similar to other examples (after line 77)

### Step 3: Autotools Build Integration (1 hour)
- Edit `examples/f_netcdf-4/Makefile.am`
- Add `f_groups` to `check_PROGRAMS` (line 8)
- Add `f_groups_SOURCES = f_groups.f90`
- LDADD already set to `-lnetcdff` (line 18)
- Add `test_f_groups.sh` to TESTS and EXTRA_DIST (lines 21-26)
- Create test wrapper script `test_f_groups.sh`

### Step 4: Testing and Validation (2 hours)
- Build with CMake: `cmake -DENABLE_FORTRAN=ON ..`
- Build with Autotools: `./configure --enable-fortran`
- Run example manually and verify output
- Generate expected CDL: `ncdump f_groups.nc > expected_output/f_groups_expected.cdl`
- Test CMake build with `ENABLE_FORTRAN=OFF` (should skip f_groups)
- Test Autotools build with `--disable-fortran` (should skip f_groups)
- Verify CI passes

### Step 5: Documentation (30 minutes)
- Ensure Doxygen comments are complete and accurate
- Verify learning objectives match C example
- Check that all Fortran API patterns are documented
- Validate cross-references to related examples

**Total Estimated Effort: 8-10 hours**

## Dependencies

**Depends on:**
- ✅ Sprint 1 C groups example (`groups.c`) - Already completed
- ✅ NetCDF-Fortran library integration in CI - Already completed (from v1.5.1)
- ✅ Existing Fortran example infrastructure - Already in place

**Blocks:**
- Future Fortran group-related examples
- v1.5.2 Sprint 2 completion

**No conflicts identified** - This is an additive feature with no impact on existing code.

## Testing Strategy

### Unit Testing
- Example program self-validates via 5 phases:
  1. File creation and group hierarchy
  2. Group navigation and name validation
  3. Dimension visibility across boundaries
  4. Variable metadata validation
  5. Data value verification (all 72 values)
- Exit code 0 on success, 2 on any failure
- Error messages printed for all failures

### Integration Testing
- CMake test: `add_test(NAME f_groups_run COMMAND f_groups)`
- Autotools test: `test_f_groups.sh` wrapper script
- CDL validation: Compare `ncdump` output with expected CDL
- Build system conditional tests (Fortran enabled/disabled)

### CI Testing
- GitHub Actions workflow already includes Fortran builds
- NetCDF-Fortran dependencies already configured
- New test will run automatically in CI pipeline

### Manual Validation
- Run `ncdump f_groups.nc` and inspect structure
- Compare with C example output: `ncdump groups.nc`
- Verify group hierarchy matches
- Verify all 5 integer types present
- Verify dimension visibility rules

## Risks & Mitigations

### Risk 1: Fortran Integer Kind Mapping
**Risk:** Fortran integer kinds may not map correctly to NetCDF-4 unsigned types
**Mitigation:** 
- Follow patterns from existing NetCDF-Fortran documentation
- Test with actual data writes and reads
- Verify with `ncdump` output showing correct types
- Reference: NetCDF-Fortran has explicit support for nf90_ubyte, nf90_ushort, nf90_uint, nf90_int64, nf90_uint64

### Risk 2: Group API Availability in NetCDF-Fortran
**Risk:** Group functions may not be available in all NetCDF-Fortran versions
**Mitigation:**
- CI uses NetCDF-Fortran 4.5.4 which supports groups
- Document minimum version requirement if needed
- Test on CI before merging

### Risk 3: Array Ordering Confusion
**Risk:** Fortran column-major vs C row-major ordering may cause confusion
**Mitigation:**
- Document dimension ordering clearly in comments
- Use same dimension order as other Fortran examples
- Verify data values match expected sequential pattern

### Risk 4: Build System Integration
**Risk:** Conditional compilation may not work correctly
**Mitigation:**
- Follow exact patterns from existing Fortran examples
- Test both enabled and disabled configurations
- Verify CI passes with both configurations

## Technical Notes

### Fortran Group API Functions
```fortran
! Group creation
integer function nf90_def_grp(parent_ncid, name, new_ncid)

! Group navigation
integer function nf90_inq_grp_ncid(ncid, name, grp_ncid)
integer function nf90_inq_grps(ncid, numgrps, ncids)
integer function nf90_inq_grpname(ncid, name)
integer function nf90_inq_grpname_len(ncid, len)

! Dimension visibility
integer function nf90_inq_dimid(ncid, name, dimid)
! Walks parent chain: child → parent → root
```

### Fortran Integer Type Constants
```fortran
! NetCDF-4 new integer types (from netcdf.mod)
nf90_ubyte   ! Unsigned 8-bit (0-255)
nf90_ushort  ! Unsigned 16-bit (0-65535)
nf90_uint    ! Unsigned 32-bit (0-4294967295)
nf90_int64   ! Signed 64-bit
nf90_uint64  ! Unsigned 64-bit
```

### Data Array Declarations
```fortran
! Fortran arrays (column-major, dimensions reversed from C)
integer(kind=1), dimension(NX, NY) :: ubyte_data
integer(kind=2), dimension(NX, NY) :: ushort_data
integer(kind=4), dimension(NX, NY) :: uint_data
integer(kind=8), dimension(NX, NY) :: int64_data
integer(kind=8), dimension(NX, NY, NZ) :: uint64_data
```

### Error Handling Pattern
```fortran
retval = nf90_def_grp(ncid, "SubGroup1", grp1_id)
if (retval /= nf90_noerr) call handle_err(retval)

! In contains section:
subroutine handle_err(status)
   integer, intent(in) :: status
   print *, "Error: ", trim(nf90_strerror(status))
   stop 2
end subroutine handle_err
```

## Definition of Done Checklist

- [ ] `f_groups.f90` created with complete Doxygen documentation
- [ ] Program compiles in CMake build when `ENABLE_FORTRAN=ON`
- [ ] Program compiles in Autotools build when `--enable-fortran`
- [ ] Program does NOT build when Fortran disabled
- [ ] Example runs as test in both build systems
- [ ] Example passes with exit code 0
- [ ] NetCDF-4 file created with correct group structure
- [ ] All 5 new integer types demonstrated
- [ ] Dimension visibility validated across group boundaries
- [ ] Manual `ncdump` verification shows structure matches C example
- [ ] Expected CDL file created and validation test added
- [ ] CI passes with new Fortran example
- [ ] Code follows existing Fortran example patterns
- [ ] No compiler warnings

## References

- **C Example:** `examples/netcdf-4/groups.c` (Sprint 1)
- **Fortran Patterns:** `examples/f_netcdf-4/f_simple_nc4.f90`, `f_compression.f90`, `f_multi_unlimited.f90`
- **CMake Integration:** `examples/f_netcdf-4/CMakeLists.txt`
- **Autotools Integration:** `examples/f_netcdf-4/Makefile.am`
- **NetCDF-Fortran Groups API:** https://docs.unidata.ucar.edu/netcdf-fortran/current/
- **NetCDF-Fortran Types:** https://docs.unidata.ucar.edu/netcdf-fortran/current/f90_The-NetCDF-Fortran-90-API.html
- **v1.5.1 Sprint 1:** Fortran build integration patterns established

## Next Steps

1. **Ready for Implementation:** Issue is well-defined and ready for `/implement` workflow
2. **No Clarifications Needed:** All requirements are clear from the C example and existing patterns
3. **Recommended Approach:** 
   - Start with Step 1 (create f_groups.f90)
   - Use `groups.c` as reference for structure and validation logic
   - Follow `f_simple_nc4.f90` for Fortran API patterns
   - Test incrementally (write phase, then read/validation phase)
4. **Estimated Timeline:** 1-2 days for implementation and testing
