!> @file f_quickstart.f90
!! @brief Minimal introduction to NetCDF from Fortran - the simplest starting point
!!
!! This is the Fortran equivalent of quickstart.c, demonstrating the essential
!! NetCDF operations using the Fortran 90 NetCDF API (nf90_* functions). The
!! program creates a tiny 2D array (2x3) with 6 integer values, adds descriptive
!! attributes, writes it to a file, then reopens the file to verify everything
!! was stored correctly.
!!
!! **Learning Objectives:**
!! - Understand the basic NetCDF workflow from Fortran
!! - Learn how to use the nf90_* API functions
!! - Master Fortran column-major array ordering
!! - Implement error handling with check() subroutine
!! - Verify data integrity
!!
!! **Key Concepts:**
!! - **Dimensions**: Named axes that define array shapes (X=2, Y=3)
!! - **Variables**: Named data arrays with dimensions and types
!! - **Attributes**: Metadata describing the file or variables
!! - **Define Mode**: Phase where structure is defined (dimensions, variables, attributes)
!! - **Data Mode**: Phase where actual data is written/read
!! - **Column-Major**: Fortran arrays are stored column-first, opposite of C
!!
!! **Fortran vs C Differences:**
!! - **Array Declaration**: Fortran data(X, Y) vs C data[X][Y]
!! - **Array Indexing**: Fortran 1-based (1 to N) vs C 0-based (0 to N-1)
!! - **API Prefix**: Fortran nf90_* vs C nc_*
!! - **Module System**: Fortran "use netcdf" vs C "#include <netcdf.h>"
!! - **Error Handling**: Fortran check() subroutine vs C ERR() macro
!!
!! **Prerequisites:** Basic Fortran 90 programming knowledge
!!
!! **Related Examples:**
!! - quickstart.c - C equivalent of this example
!! - f_simple_2D.f90 - More detailed 2D array example
!!
!! **Compilation:**
!! @code
!! gfortran -o f_quickstart f_quickstart.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_quickstart
!! ncdump f_quickstart.nc
!! @endcode
!!
!! **Expected Output:**
!! Creates f_quickstart.nc containing:
!! - 2 dimensions: X(2), Y(3)
!! - 1 variable: data(X, Y) of type int
!! - 1 global attribute: description = "a quickstart example"
!! - 1 variable attribute: units = "m/s"
!! - Data: 6 sequential integers (1, 2, 3, 4, 5, 6)
!!
!! @author Edward Hartnett
!! @date 2026-01-29

program f_quickstart
  use netcdf
  implicit none
  
  character(len=*), parameter :: FILE_NAME = "f_quickstart.nc"
  integer, parameter :: XDIM = 2, YDIM = 3
  
  integer :: ncid, x_dimid, y_dimid, data_varid
  integer :: dimids(2)
  integer :: retval
  
  integer :: data_out(XDIM, YDIM)
  integer :: data_in(XDIM, YDIM)
  
  integer :: i, j
  integer :: ndims_in, nvars_in, ngatts_in
  integer :: len_x, len_y
  character(len=100) :: desc_in, units_in
  integer :: errors
  
  ! ========== WRITE PHASE ==========
  print *, "Creating NetCDF file: ", FILE_NAME
  
  ! Create the NetCDF file (nf90_clobber overwrites existing file)
  retval = nf90_create(FILE_NAME, nf90_clobber, ncid)
  call check(retval, "creating file")
  
  ! Define dimensions: X=2, Y=3
  retval = nf90_def_dim(ncid, "X", XDIM, x_dimid)
  call check(retval, "defining X dimension")
  
  retval = nf90_def_dim(ncid, "Y", YDIM, y_dimid)
  call check(retval, "defining Y dimension")
  
  ! Define the variable with dimensions X and Y
  ! Note: Fortran column-major order means first dimension varies fastest
  dimids(1) = x_dimid
  dimids(2) = y_dimid
  retval = nf90_def_var(ncid, "data", nf90_int, dimids, data_varid)
  call check(retval, "defining variable")
  
  ! Add global attribute
  retval = nf90_put_att(ncid, nf90_global, "description", "a quickstart example")
  call check(retval, "adding global attribute")
  
  ! Add variable attribute
  retval = nf90_put_att(ncid, data_varid, "units", "m/s")
  call check(retval, "adding variable attribute")
  
  ! End define mode - ready to write data
  retval = nf90_enddef(ncid)
  call check(retval, "ending define mode")
  
  ! Generate data: sequential integers (1, 2, 3, 4, 5, 6)
  ! Fortran column-major: fill by column to match C row-major layout
  do j = 1, YDIM
    do i = 1, XDIM
      data_out(i, j) = (j-1) * XDIM + i
    end do
  end do
  
  ! Write the data to the file
  retval = nf90_put_var(ncid, data_varid, data_out)
  call check(retval, "writing data")
  
  ! Close the file
  retval = nf90_close(ncid)
  call check(retval, "closing file")
  
  print *, "*** SUCCESS writing file!"
  
  ! ========== READ PHASE ==========
  print *, ""
  print *, "Reopening file for validation..."
  
  ! Open the file for reading
  retval = nf90_open(FILE_NAME, nf90_nowrite, ncid)
  call check(retval, "reopening file")
  
  ! Verify metadata: check number of dimensions, variables, and attributes
  retval = nf90_inquire(ncid, ndims_in, nvars_in, ngatts_in)
  call check(retval, "inquiring file")
  
  print *, "File contains:", ndims_in, "dimensions,", nvars_in, "variables,", &
           ngatts_in, "global attributes"
  
  if (ndims_in /= 2 .or. nvars_in /= 1 .or. ngatts_in /= 1) then
    print *, "Error: Unexpected file structure"
    stop 1
  end if
  
  ! Verify dimension sizes
  retval = nf90_inquire_dimension(ncid, x_dimid, len=len_x)
  call check(retval, "inquiring X dimension")
  
  retval = nf90_inquire_dimension(ncid, y_dimid, len=len_y)
  call check(retval, "inquiring Y dimension")
  
  if (len_x /= XDIM .or. len_y /= YDIM) then
    print *, "Error: Expected dimensions X=", XDIM, ", Y=", YDIM, &
             ", found X=", len_x, ", Y=", len_y
    stop 1
  end if
  print *, "Verified: X=", len_x, ", Y=", len_y
  
  ! Verify global attribute
  retval = nf90_get_att(ncid, nf90_global, "description", desc_in)
  call check(retval, "reading global attribute")
  print *, "Verified: global attribute 'description' = '", trim(desc_in), "'"
  
  ! Verify variable attribute
  retval = nf90_get_att(ncid, data_varid, "units", units_in)
  call check(retval, "reading variable attribute")
  print *, "Verified: variable attribute 'units' = '", trim(units_in), "'"
  
  ! Read the data back
  retval = nf90_get_var(ncid, data_varid, data_in)
  call check(retval, "reading data")
  
  ! Verify data correctness
  errors = 0
  do j = 1, YDIM
    do i = 1, XDIM
      if (data_in(i, j) /= data_out(i, j)) then
        print *, "Error: data(", i, ",", j, ") = ", data_in(i, j), &
                 ", expected ", data_out(i, j)
        errors = errors + 1
      end if
    end do
  end do
  
  if (errors > 0) then
    print *, "*** FAILED:", errors, "data validation errors"
    stop 1
  end if
  
  print *, "Verified: all 6 data values correct (1, 2, 3, 4, 5, 6)"
  
  ! Close the file
  retval = nf90_close(ncid)
  call check(retval, "closing file after reading")
  
  print *, ""
  print *, "*** SUCCESS: All validation checks passed!"
  
contains
  !> @brief Error handling subroutine
  !! @param status NetCDF return status code
  !! @param context Descriptive message about the operation
  subroutine check(status, context)
    integer, intent(in) :: status
    character(len=*), intent(in) :: context
    
    if (status /= nf90_noerr) then
      print *, "Error ", trim(context), ": ", trim(nf90_strerror(status))
      stop 2
    end if
  end subroutine check
  
end program f_quickstart
