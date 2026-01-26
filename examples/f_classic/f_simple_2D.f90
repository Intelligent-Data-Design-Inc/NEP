!> @file f_simple_2D.f90
!! @brief Basic example demonstrating 2D array creation and reading in NetCDF (Fortran)
!!
!! This is the Fortran equivalent of simple_2D.c, demonstrating the fundamental
!! workflow for working with NetCDF files using the Fortran 90 NetCDF API. The
!! program creates a 2D integer array, writes it to a NetCDF file, then reopens
!! the file to verify both metadata and data correctness.
!!
!! **Learning Objectives:**
!! - Understand Fortran NetCDF API (nf90_* functions)
!! - Learn Fortran column-major vs C row-major array ordering
!! - Master error handling with nf90_noerr and nf90_strerror()
!! - Work with Fortran array indexing (1-based vs C's 0-based)
!! - Verify equivalence with C version (simple_2D.c)
!!
!! **Key Concepts:**
!! - **Fortran Column-Major**: Arrays stored column-first [i,j] vs C row-first [j][i]
!! - **Dimension Ordering**: Fortran reverses dimension order from C
!! - **1-Based Indexing**: Fortran arrays start at 1, C arrays start at 0
!! - **nf90 Module**: Fortran 90 NetCDF interface (use netcdf)
!! - **Error Handling**: Check retval against nf90_noerr
!!
!! **Fortran vs C Differences:**
!! - **Array Declaration**: Fortran data_out(NX, NY) vs C data_out[NY][NX]
!! - **Dimension Order**: Fortran dimids(1)=x, dimids(2)=y vs C dimids[0]=y, dimids[1]=x
!! - **Indexing**: Fortran 1-based (1 to N) vs C 0-based (0 to N-1)
!! - **API Prefix**: Fortran nf90_* vs C nc_*
!! - **Error Handling**: Fortran subroutine call vs C macro
!!
!! **Prerequisites:** 
!! - simple_2D.c - C equivalent for comparison
!!
!! **Related Examples:**
!! - simple_2D.c - C equivalent of this example
!! - f_coord_vars.f90 - Adds coordinate variables
!! - f_simple_nc4.f90 - NetCDF-4 specific features
!!
!! **Compilation:**
!! @code
!! gfortran -o f_simple_2D f_simple_2D.f90 -lnetcdff -lnetcdf
!! @endcode
!!
!! **Usage:**
!! @code
!! ./f_simple_2D
!! ncdump f_simple_2D.nc
!! @endcode
!!
!! **Expected Output:**
!! Creates f_simple_2D.nc containing:
!! - 2 dimensions: x(6), y(12)
!! - 1 variable: data(x, y) of type int
!! - Data: sequential integers from 0 to 71
!! - Output identical to simple_2D.c (verified via ncdump)
!!
!! @author Edward Hartnett, Intelligent Data Design, Inc.
!! @date 2026


program f_simple_2D
   use netcdf
   implicit none
   
   character(len=*), parameter :: FILE_NAME = "f_simple_2D.nc"
   integer, parameter :: NDIMS = 2
   integer, parameter :: NX = 6, NY = 12
   
   integer :: ncid, varid
   integer :: x_dimid, y_dimid
   integer :: dimids(NDIMS)
   integer :: retval
   
   integer :: data_out(NX, NY)
   integer :: data_in(NX, NY)
   
   integer :: i, j
   integer :: ndims_in, nvars_in
   integer :: len_x, len_y
   integer :: var_type
   integer :: errors
   integer :: expected
   
   ! ========== WRITE PHASE ==========
   print *, "Creating NetCDF file: ", FILE_NAME
   
   ! Initialize data with sequential integers (0, 1, 2, 3, ...)
   ! Note: Fortran is column-major, so we fill by column to match C row-major layout
   do j = 1, NY
      do i = 1, NX
         data_out(i, j) = (j-1) * NX + (i-1)
      end do
   end do
   
   ! Create the NetCDF file (NF90_CLOBBER overwrites existing file)
   retval = nf90_create(FILE_NAME, NF90_CLOBBER, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define dimensions
   retval = nf90_def_dim(ncid, "x", NX, x_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_def_dim(ncid, "y", NY, y_dimid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Define the variable (dimension order: x, y for Fortran column-major)
   dimids(1) = x_dimid
   dimids(2) = y_dimid
   retval = nf90_def_var(ncid, "data", NF90_INT, dimids, varid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! End define mode
   retval = nf90_enddef(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Write the data to the file
   retval = nf90_put_var(ncid, varid, data_out)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, "*** SUCCESS writing file!"
   
   ! ========== READ PHASE ==========
   print *, ""
   print *, "Reopening file for validation..."
   
   ! Open the file for reading
   retval = nf90_open(FILE_NAME, NF90_NOWRITE, ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify metadata: check number of dimensions and variables
   retval = nf90_inquire(ncid, ndims_in, nvars_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (ndims_in /= NDIMS) then
      print *, "Error: Expected ", NDIMS, " dimensions, found ", ndims_in
      stop 2
   end if
   print *, "Verified: ", ndims_in, " dimensions"
   
   if (nvars_in /= 1) then
      print *, "Error: Expected 1 variable, found ", nvars_in
      stop 2
   end if
   print *, "Verified: ", nvars_in, " variable"
   
   ! Verify dimension sizes
   retval = nf90_inquire_dimension(ncid, x_dimid, len=len_x)
   if (retval /= nf90_noerr) call handle_err(retval)
   retval = nf90_inquire_dimension(ncid, y_dimid, len=len_y)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (len_x /= NX) then
      print *, "Error: Expected x dimension = ", NX, ", found ", len_x
      stop 2
   end if
   if (len_y /= NY) then
      print *, "Error: Expected y dimension = ", NY, ", found ", len_y
      stop 2
   end if
   print *, "Verified: x dimension = ", len_x, ", y dimension = ", len_y
   
   ! Verify variable type
   retval = nf90_inquire_variable(ncid, varid, xtype=var_type)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   if (var_type /= NF90_INT) then
      print *, "Error: Expected variable type NF90_INT, found ", var_type
      stop 2
   end if
   print *, "Verified: variable type is NF90_INT"
   
   ! Read the data back
   retval = nf90_get_var(ncid, varid, data_in)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   ! Verify data correctness
   errors = 0
   do j = 1, NY
      do i = 1, NX
         expected = (j-1) * NX + (i-1)
         if (data_in(i, j) /= expected) then
            print *, "Error: data(", i, ",", j, ") = ", data_in(i, j), &
                     ", expected ", expected
            errors = errors + 1
         end if
      end do
   end do
   
   if (errors > 0) then
      print *, "*** FAILED: ", errors, " data validation errors"
      stop 2
   end if
   
   print *, "Verified: all ", NX * NY, " data values correct (0, 1, 2, ..., ", &
            NX * NY - 1, ")"
   
   ! Close the file
   retval = nf90_close(ncid)
   if (retval /= nf90_noerr) call handle_err(retval)
   
   print *, ""
   print *, "*** SUCCESS: All validation checks passed!"
   
contains
   subroutine handle_err(status)
      integer, intent(in) :: status
      print *, "Error: ", trim(nf90_strerror(status))
      stop 2
   end subroutine handle_err
   
end program f_simple_2D
