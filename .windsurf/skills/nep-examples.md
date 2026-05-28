# Creating NEP Example Files

## Overview

NEP example files demonstrate library features and serve as integration tests. They are located in `examples/` with subdirectories for each format:
- `examples/classic/` - Classic NetCDF-3 examples
- `examples/netcdf-4/` - NetCDF-4/HDF5 examples
- `examples/f_classic/` - Fortran classic examples
- `examples/f_netcdf-4/` - Fortran NetCDF-4 examples

## Programming Rules

### 1. Consolidated Validation Checks

**Rule**: Check all metadata values in a single if-statement, not one by one.

**Rationale**: Reduces code verbosity and makes examples easier to read and maintain.

**Don't do this** (verbose, one check per attribute):
```c
/* Verify time attributes one by one */
if ((retval = nc_inq_attlen(ncid, time_varid, "units", &att_len)))
   ERR(retval);
if ((retval = nc_get_att_text(ncid, time_varid, "units", att_text)))
   ERR(retval);
att_text[att_len] = '\0';
if (strcmp(att_text, "hours since 2026-01-01") != 0) {
   printf("Error: time units = '%s', expected 'hours since 2026-01-01'\n", att_text);
   exit(ERRCODE);
}
printf("Verified: time units = '%s'\n", att_text);

if ((retval = nc_inq_attlen(ncid, time_varid, "standard_name", &att_len)))
   ERR(retval);
/* ... more lines for each attribute ... */
```

**Do this instead** (consolidated check):
```c
/* Verify all metadata in one place */
char units[256], standard_name[256], axis[256], calendar[256];
nc_get_att_text(ncid, time_varid, "units", units);
nc_get_att_text(ncid, time_varid, "standard_name", standard_name);
nc_get_att_text(ncid, time_varid, "axis", axis);
nc_get_att_text(ncid, time_varid, "calendar", calendar);

if (strcmp(units, "hours since 2026-01-01") != 0 ||
    strcmp(standard_name, "time") != 0 ||
    strcmp(axis, "T") != 0 ||
    strcmp(calendar, "standard") != 0) {
    printf("Error: time coordinate attributes incorrect\n");
    exit(ERRCODE);
}
printf("Verified: all time coordinate attributes correct\n");
```

### 2. Error Handling Pattern

**Rule**: Use a consistent error macro for API calls, but batch validation checks.

```c
#define ERRCODE 2
#define ERR(e) {printf("Error: %s\n", nc_strerror(e)); exit(ERRCODE);}

/* API calls use ERR macro */
if ((retval = nc_create(FILE_NAME, NC_CLOBBER, &ncid)))
   ERR(retval);

/* Validation checks use consolidated if-statement */
if (ndims_in != NDIMS || nvars_in != NVARS) {
    printf("Error: expected %d dims/%d vars, got %d/%d\n", 
           NDIMS, NVARS, ndims_in, nvars_in);
    exit(ERRCODE);
}
```

### 3. File Structure

All examples should follow this structure:

1. **File header** - Doxygen documentation with:
   - `@file` - filename
   - `@brief` - one-line description
   - Detailed description explaining the feature demonstrated
   - **Learning Objectives** section
   - **Key Concepts** section
   - **Prerequisites** - related examples
   - **Related Examples** - Fortran equivalents, similar examples
   - `@author` and `@date`

2. **Includes and defines** - Standard headers, netcdf.h, constants

3. **Main function** with two phases:
   - **Write phase**: Create file, define metadata, write data
   - **Read/validate phase**: Reopen file, verify with consolidated checks

4. **Success/failure output** - Clear pass/fail message at end

### 4. Documentation Requirements

**Doxygen tags to include**:
```c
/**
 * @file example_name.c
 * @brief Brief description of what this example demonstrates
 *
 * Detailed paragraph explaining the feature, its purpose, and 
 * what the user will learn.
 *
 * **Learning Objectives:**
 * - Objective 1
 * - Objective 2
 *
 * **Key Concepts:**
 * - Concept 1: explanation
 * - Concept 2: explanation
 *
 * **CF Convention Attributes Used:** (if applicable)
 * - `attribute_name`: explanation
 *
 * **Prerequisites:**
 * - prerequisite_example.c
 *
 * **Related Examples:**
 * - f_example.f90 - Fortran equivalent
 *
 * **Compilation:**
 * @code
 * gcc -o example_name example_name.c -lnetcdf
 * @endcode
 *
 * **Usage:**
 * @code
 * ./example_name
 * ncdump example.nc
 * @endcode
 *
 * **Expected Output:**
 * Description of what the example creates.
 *
 * @author Edward Hartnett, Intelligent Data Design, Inc.
 * @date 2026
 */
```

### 5. Data Validation Pattern

Read data and validate with consolidated checks:

```c
/* Read data */
if ((retval = nc_get_var_float(ncid, varid, data_in)))
   ERR(retval);

/* Validate with single loop and error counter */
int errors = 0;
for (int i = 0; i < NUM_ELEMENTS && !errors; i++) {
    if (data_in[i] != expected_data[i]) {
        printf("Error: data[%d] = %f, expected %f\n", 
               i, data_in[i], expected_data[i]);
        errors++;
    }
}

if (errors) {
    printf("*** FAILED: %d data validation errors\n", errors);
    exit(ERRCODE);
}
printf("Verified: all %d data values correct\n", NUM_ELEMENTS);
```

### 6. Example File Naming

- C examples: `descriptive_name.c`
- Fortran examples: `f_descriptive_name.f90`
- Output files: `descriptive_name.nc`
- Group related examples by format in subdirectories

## Checklist for New Examples

- [ ] File header with complete Doxygen documentation
- [ ] Uses `ERR` macro for API error handling
- [ ] Consolidated metadata validation (not one-by-one)
- [ ] Write phase clearly marked with comment
- [ ] Read/validate phase clearly marked with comment
- [ ] Success/failure message at end
- [ ] Added to appropriate `CMakeLists.txt` and `Makefile.am`
- [ ] Expected output CDL file in `examples/expected_output/`
- [ ] Fortran equivalent (if applicable)
- [ ] Cross-referenced in prerequisite/related examples
